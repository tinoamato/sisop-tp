#define _XOPEN_SOURCE 700
#include "ipc.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>      // O_CREAT, O_EXCL, O_RDWR
#include <sys/mman.h>   // shm_open, mmap
#include <sys/stat.h>   // ftruncate
#include <unistd.h>     // ftruncate, close, usleep
#include <errno.h>
#include <time.h>

// Globals
SharedData *g_shm = NULL;
sem_t *g_sem_empty = NULL;
sem_t *g_sem_full  = NULL;
sem_t *g_sem_buf_mutex = NULL;
sem_t *g_sem_id_mutex  = NULL;

static int create_or_open_sems(int create) {
    unsigned int one = 1, zero = 0, slots = SLOTS;
    mode_t mode = 0666;

    g_sem_empty = sem_open(SEM_EMPTY, create ? (O_CREAT|O_EXCL) : 0, mode, slots);
    if (g_sem_empty == SEM_FAILED && create && errno == EEXIST) g_sem_empty = sem_open(SEM_EMPTY, 0);
    if (g_sem_empty == SEM_FAILED) { perror("sem_open(EMPTY)"); return -1; }

    g_sem_full = sem_open(SEM_FULL, create ? (O_CREAT|O_EXCL) : 0, mode, zero);
    if (g_sem_full == SEM_FAILED && create && errno == EEXIST) g_sem_full = sem_open(SEM_FULL, 0);
    if (g_sem_full == SEM_FAILED) { perror("sem_open(FULL)"); return -1; }

    g_sem_buf_mutex = sem_open(SEM_BUF_MUTEX, create ? (O_CREAT|O_EXCL) : 0, mode, one);
    if (g_sem_buf_mutex == SEM_FAILED && create && errno == EEXIST) g_sem_buf_mutex = sem_open(SEM_BUF_MUTEX, 0);
    if (g_sem_buf_mutex == SEM_FAILED) { perror("sem_open(BUF_MUTEX)"); return -1; }

    g_sem_id_mutex = sem_open(SEM_ID_MUTEX, create ? (O_CREAT|O_EXCL) : 0, mode, one);
    if (g_sem_id_mutex == SEM_FAILED && create && errno == EEXIST) g_sem_id_mutex = sem_open(SEM_ID_MUTEX, 0);
    if (g_sem_id_mutex == SEM_FAILED) { perror("sem_open(ID_MUTEX)"); return -1; }

    return 0;
}

int ipc_init(uint64_t total_registros) {
    int fd = -1;
    int created = 0;

    // 1) SHM
    fd = shm_open(SHM_NAME, O_RDWR | O_CREAT | O_EXCL, 0666);
    if (fd >= 0) {
        created = 1;
    } else {
        if (errno == EEXIST) {
            fd = shm_open(SHM_NAME, O_RDWR, 0666);
            if (fd < 0) { perror("shm_open existing"); return -1; }
        } else {
            perror("shm_open");
            return -1;
        }
    }

    if (created) {
        if (ftruncate(fd, sizeof(SharedData)) < 0) {
            perror("ftruncate");
            close(fd);
            shm_unlink(SHM_NAME);
            return -1;
        }
    }

    void *addr = mmap(NULL, sizeof(SharedData),
                      PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (addr == MAP_FAILED) {
        perror("mmap");
        close(fd);
        if (created) shm_unlink(SHM_NAME);
        return -1;
    }
    close(fd);
    g_shm = (SharedData *)addr;

    if (created) {
        memset(g_shm, 0, sizeof(*g_shm));
        g_shm->next_id   = 1;
        g_shm->remaining = total_registros;
        g_shm->head = g_shm->tail = 0;
        g_shm->shutdown = 0;
    }

    if (create_or_open_sems(created) < 0) {
        ipc_cleanup();
        return -1;
    }

    return 0;
}

void ipc_notify_shutdown(void) {
    if (g_shm) g_shm->shutdown = 1;
    // Desbloquear posibles waits (productores/consumidores)
    for (int i = 0; i < SLOTS * 2; ++i) {
        if (g_sem_empty) sem_post(g_sem_empty);
        if (g_sem_full)  sem_post(g_sem_full);
    }
}

void ipc_cleanup(void) {
    // Cerrar semáforos
    if (g_sem_empty && g_sem_empty != SEM_FAILED) { sem_close(g_sem_empty); g_sem_empty = NULL; }
    if (g_sem_full  && g_sem_full  != SEM_FAILED) { sem_close(g_sem_full ); g_sem_full  = NULL; }
    if (g_sem_buf_mutex && g_sem_buf_mutex != SEM_FAILED) { sem_close(g_sem_buf_mutex); g_sem_buf_mutex = NULL; }
    if (g_sem_id_mutex  && g_sem_id_mutex  != SEM_FAILED) { sem_close(g_sem_id_mutex ); g_sem_id_mutex  = NULL; }

    // Unlink (por si somos dueños en esta corrida)
    sem_unlink(SEM_EMPTY);
    sem_unlink(SEM_FULL);
    sem_unlink(SEM_BUF_MUTEX);
    sem_unlink(SEM_ID_MUTEX);

    // Desmapear y unlink de SHM
    if (g_shm) {
        munmap(g_shm, sizeof(SharedData));
        g_shm = NULL;
    }
    shm_unlink(SHM_NAME);
}

void ipc_print_state(void) {
    if (!g_shm) {
        printf("[IPC] SHM no mapeada.\n");
        return;
    }
    printf("[IPC] SHM OK — next_id=%llu remaining=%llu head=%u tail=%u slots=%d\n",
           (unsigned long long)g_shm->next_id,
           (unsigned long long)g_shm->remaining,
           g_shm->head, g_shm->tail, SLOTS);
}

/* ---------- Espera portable con timeout ----------- */

int sem_wait_cancellable(sem_t *sem, int timeout_ms) {
#if defined(__APPLE__)
    // macOS históricamente no tuvo sem_timedwait estable.
    // Implementamos poll con sem_trywait + nanosleep.
    const int step_ms = 5;
    int waited = 0;
    while (1) {
        if (sem_trywait(sem) == 0) return 0;
        if (errno != EAGAIN) return -1; // error real

        if (timeout_ms >= 0 && waited >= timeout_ms) {
            errno = ETIMEDOUT;
            return -1;
        }
        struct timespec req = {0};
        req.tv_nsec = step_ms * 1000000L;
        nanosleep(&req, NULL);
        waited += step_ms;
    }
#else
    // Linux: usar sem_timedwait real
    if (timeout_ms < 0) {
        return sem_wait(sem); // bloqueo sin timeout
    }
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) < 0) return -1;
    ts.tv_sec  += timeout_ms / 1000;
    ts.tv_nsec += (timeout_ms % 1000) * 1000000L;
    if (ts.tv_nsec >= 1000000000L) { ts.tv_sec++; ts.tv_nsec -= 1000000000L; }

    return sem_timedwait(sem, &ts);
#endif
}