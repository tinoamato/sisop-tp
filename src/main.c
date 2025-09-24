#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <signal.h>
#include <sys/wait.h>

#include "ipc.h"

static int g_generadores = 0;
static int g_total = 0;

static void usage(const char *progname) {
    printf("Uso: %s -g <generadores> -n <total_registros>\n", progname);
    exit(EXIT_FAILURE);
}

static void on_sigint(int sig) {
    (void)sig;
    fprintf(stderr, "\n[MAIN] SIGINT recibido. Limpiando recursos...\n");
    ipc_cleanup();
    _exit(130);
}

// -------------------- GENERADOR --------------------
void run_generator(int id) {
    srand(getpid()); // semilla aleatoria distinta por proceso

    while (1) {
        // Pedir lote de 10 IDs
        int start, count;

        sem_wait(g_sem_id_mutex);
        if (g_shm->remaining == 0) {
            sem_post(g_sem_id_mutex);
            break; // ya no hay más trabajo
        }
        start = g_shm->next_id;
        count = (g_shm->remaining >= 10) ? 10 : g_shm->remaining;
        g_shm->next_id += count;
        g_shm->remaining -= count;
        sem_post(g_sem_id_mutex);

        // Generar registros y publicarlos de a uno
        for (int i = 0; i < count; i++) {
            Registro r;
            r.id = start + i;
            r.campo1 = rand() % 100;   // valor aleatorio
            r.campo2 = rand() % 1000;  // valor aleatorio

            sem_wait(g_sem_empty);
            sem_wait(g_sem_buf_mutex);

            g_shm->buffer[g_shm->tail] = r;
            g_shm->tail = (g_shm->tail + 1) % SLOTS;

            sem_post(g_sem_buf_mutex);
            sem_post(g_sem_full);
        }
    }

    printf("[GEN %d] Terminé.\n", id);
    _exit(0);
}

// -------------------- COORDINADOR --------------------
void run_coordinator(void) {
    FILE *f = fopen("salida.csv", "a"); // abrir en append
    if (!f) {
        perror("No se pudo abrir salida.csv");
        return;
    }

    int processed = 0;
    while (1) {
        if (processed >= g_total) break;

        sem_wait(g_sem_full);
        sem_wait(g_sem_buf_mutex);

        Registro r = g_shm->buffer[g_shm->head];
        g_shm->head = (g_shm->head + 1) % SLOTS;

        sem_post(g_sem_buf_mutex);
        sem_post(g_sem_empty);

        fprintf(f, "%llu,%d,%d\n",
                (unsigned long long)r.id,
                r.campo1,
                r.campo2);
        processed++;
    }

    fclose(f);
    printf("[COORD] Terminé. Total escritos: %d\n", processed);
}

// -------------------- MAIN --------------------
int main(int argc, char *argv[]) {
    int opt;
    while ((opt = getopt(argc, argv, "g:n:")) != -1) {
        switch (opt) {
            case 'g': g_generadores = atoi(optarg); break;
            case 'n': g_total = atoi(optarg); break;
            default: usage(argv[0]);
        }
    }

    if (g_generadores <= 0 || g_total <= 0) {
        usage(argv[0]);
    }

    signal(SIGINT, on_sigint);
    atexit(ipc_cleanup);

    // CSV inicial con cabecera
    FILE *f = fopen("salida.csv", "w");
    if (!f) { perror("No se pudo abrir salida.csv"); return EXIT_FAILURE; }
    fprintf(f, "ID,Campo1,Campo2\n");
    fclose(f);

    // Inicializar IPC
    if (ipc_init((uint64_t)g_total) != 0) {
        fprintf(stderr, "[MAIN] Error inicializando IPC.\n");
        return EXIT_FAILURE;
    }

    printf("[MAIN] Generadores: %d, Total: %d\n", g_generadores, g_total);

    // Fork de N generadores
    for (int i = 0; i < g_generadores; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            run_generator(i+1);
        }
    }

    // Proceso padre → coordinador
    run_coordinator();

    // Esperar hijos
    for (int i = 0; i < g_generadores; i++) {
        wait(NULL);
    }

    return 0;
}