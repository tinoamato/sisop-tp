#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>
#include <time.h>

#include "ipc.h"

#ifdef __linux__
#include <sys/prctl.h>   // PR_SET_PDEATHSIG
#endif

static int g_generadores = 0;
static int g_total = 0;

/* ========== Señales ========== */
static void on_sigint(int sig) {
    (void)sig;
    fprintf(stderr, "\n[MAIN] SIGINT recibido. Notificando shutdown y limpiando recursos...\n");
    ipc_notify_shutdown();
    ipc_cleanup();
    _exit(130);
}

/* ========== GENERADOR ========== */

static volatile sig_atomic_t kid_terminate = 0;
static void kid_sigterm(int s){ (void)s; kid_terminate = 1; }

void run_generator(int id) {
#ifdef __linux__
    prctl(PR_SET_PDEATHSIG, SIGTERM);
#endif
    signal(SIGTERM, kid_sigterm);
    signal(SIGINT,  kid_sigterm);

    srand(getpid());

    while (!kid_terminate) {
        if (g_shm && g_shm->shutdown) break;

        if (getppid() == 1) {
            fprintf(stderr, "[GEN %d] Padre muerto, salgo.\n", id);
            break;
        }

        int start = 0, count = 0;
        if (sem_wait_cancellable(g_sem_id_mutex, 500) == -1) {
            if (errno == ETIMEDOUT) continue;
            perror("[GEN] sem_wait id_mutex");
            break;
        }
        if (g_shm->remaining == 0 || g_shm->shutdown) {
            sem_post(g_sem_id_mutex);
            break;
        }
        start = (int)g_shm->next_id;
        count = (g_shm->remaining >= 10) ? 10 : (int)g_shm->remaining;
        g_shm->next_id += (uint64_t)count;
        g_shm->remaining -= (uint64_t)count;
        sem_post(g_sem_id_mutex);

        for (int i = 0; i < count && !kid_terminate; i++) {
            if (g_shm->shutdown) break;
            if (getppid() == 1) break;

            Registro r;
            r.id = (uint64_t)(start + i);
            r.sensor_id = id;                       // ID del generador
            r.temperatura = 15 + rand() % 16;       // 15 a 30
            r.humedad = 30 + rand() % 70;           // 30 a 99

            if (sem_wait_cancellable(g_sem_empty, 1000) == -1) {
                if (errno == ETIMEDOUT) {
                    if (kid_terminate || (g_shm && g_shm->shutdown)) break;
                    i--;
                    continue;
                } else {
                    perror("[GEN] sem_wait empty");
                    break;
                }
            }

            if (sem_wait_cancellable(g_sem_buf_mutex, 500) == -1) {
                if (errno == ETIMEDOUT) {
                    sem_post(g_sem_empty);
                    i--;
                    continue;
                } else {
                    perror("[GEN] sem_wait buf_mutex");
                    sem_post(g_sem_empty);
                    break;
                }
            }

            g_shm->buffer[g_shm->tail] = r;
            g_shm->tail = (g_shm->tail + 1) % SLOTS;

            sem_post(g_sem_buf_mutex);
            sem_post(g_sem_full);

            usleep(500000); // 0.5 segundos
        }
    }

    printf("[GEN %d] Terminé.\n", id);
    _exit(0);
}

/* ========== COORDINADOR ========== */

void run_coordinator(void) {
    FILE *f = fopen("salida.csv", "a");
    if (!f) {
        perror("No se pudo abrir salida.csv");
        return;
    }

    int processed = 0;

    while (processed < g_total) {
        if (g_shm && g_shm->shutdown) break;

        if (sem_wait_cancellable(g_sem_full, 1000) == -1) {
            if (errno == ETIMEDOUT) {
                if (g_shm && g_shm->remaining == 0) {
                    if (g_shm->head == g_shm->tail) break;
                }
                continue;
            } else {
                perror("[COORD] sem_wait full");
                break;
            }
        }

        if (sem_wait_cancellable(g_sem_buf_mutex, 500) == -1) {
            if (errno == ETIMEDOUT) {
                sem_post(g_sem_full);
                continue;
            } else {
                perror("[COORD] sem_wait buf_mutex");
                sem_post(g_sem_full);
                break;
            }
        }

        Registro r = g_shm->buffer[g_shm->head];
        g_shm->head = (g_shm->head + 1) % SLOTS;

        sem_post(g_sem_buf_mutex);
        sem_post(g_sem_empty);

        fprintf(f, "%llu,%d,%d,%d\n",
                (unsigned long long)r.id, r.sensor_id, r.temperatura, r.humedad);
        processed++;
    }

    fclose(f);

    ipc_notify_shutdown();
    printf("[COORD] Terminé. Total escritos: %d\n", processed);

    if (processed < g_total) {
        fprintf(stderr,
            "[COORD] Advertencia: solo se escribieron %d de %d registros (posible caída de hijos).\n",
            processed, g_total);
    }
}

/* ========== MAIN ========== */

static void usage(const char *progname) {
    printf("Uso: %s -g <generadores> -n <total_registros>\n", progname);
    exit(EXIT_FAILURE);
}

int main(int argc, char *argv[]) {
    int opt;
    while ((opt = getopt(argc, argv, "g:n:")) != -1) {
        switch (opt) {
            case 'g': g_generadores = atoi(optarg); break;
            case 'n': g_total = atoi(optarg); break;
            default: usage(argv[0]);
        }
    }
    if (g_generadores <= 0 || g_total <= 0) usage(argv[0]);

    signal(SIGINT, on_sigint);
    atexit(ipc_cleanup);

    FILE *f = fopen("salida.csv", "w");
    if (!f) { perror("No se pudo abrir salida.csv"); return EXIT_FAILURE; }
    fprintf(f, "ID,SensorID,Temperatura,Humedad\n");
    fclose(f);

    if (ipc_init((uint64_t)g_total) != 0) {
        fprintf(stderr, "[MAIN] Error inicializando IPC.\n");
        return EXIT_FAILURE;
    }

    printf("[MAIN] Generadores: %d, Total: %d\n", g_generadores, g_total);
    ipc_print_state();

    for (int i = 0; i < g_generadores; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            run_generator(i+1);
        }
    }

    run_coordinator();

    for (int i = 0; i < g_generadores; i++) {
        wait(NULL);
    }

    return 0;
}