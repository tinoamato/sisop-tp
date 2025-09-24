#ifndef IPC_H
#define IPC_H

#include <stdint.h>
#include <semaphore.h>

// Nombres POSIX (deben comenzar con '/')
#define SHM_NAME      "/tp1_shm"
#define SEM_EMPTY     "/tp1_sem_empty"
#define SEM_FULL      "/tp1_sem_full"
#define SEM_BUF_MUTEX "/tp1_sem_buf_mutex"
#define SEM_ID_MUTEX  "/tp1_sem_id_mutex"

// Tamaño del buffer (ajustable)
#define SLOTS 64

// Registro de ejemplo (lo iremos ajustando luego)
typedef struct {
    uint64_t id;
    int campo1;
    int campo2;
} Registro;

typedef struct {
    // Control
    uint64_t next_id;
    uint64_t remaining;
    uint32_t head;
    uint32_t tail;
    uint32_t shutdown;      // bandera de cierre ordenado

    // Buffer
    Registro buffer[SLOTS];
} SharedData;

// Puntero global a la SHM (mapeo)
extern SharedData *g_shm;

// Semáforos globales
extern sem_t *g_sem_empty;
extern sem_t *g_sem_full;
extern sem_t *g_sem_buf_mutex;
extern sem_t *g_sem_id_mutex;

/**
 * Crea/Inicializa SHM y semáforos.
 * Devuelve 0 si ok, -1 si error.
 */
int ipc_init(uint64_t total_registros);

/**
 * Cierra/desmapea y hace unlink de SHM y semáforos.
 * Es segura para llamar múltiples veces.
 */
void ipc_cleanup(void);

/** Imprime estado mínimo (debug). */
void ipc_print_state(void);

#endif // IPC_H