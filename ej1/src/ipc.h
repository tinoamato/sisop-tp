#ifndef IPC_H
#define IPC_H

#include <stdint.h>
#include <semaphore.h>

// Nombres POSIX (empiezan con '/')
#define SHM_NAME      "/tp1_shm"
#define SEM_EMPTY     "/tp1_sem_empty"
#define SEM_FULL      "/tp1_sem_full"
#define SEM_BUF_MUTEX "/tp1_sem_buf_mutex"
#define SEM_ID_MUTEX  "/tp1_sem_id_mutex"

// Tamaño del buffer
#define SLOTS 64

// Registro de ejemplo
typedef struct {
    uint64_t id;        // ID correlativo único
    int sensor_id;      // de qué generador proviene
    int temperatura;    // 15 a 30
    int humedad;        // 30 a 99
} Registro;

typedef struct {
    // Control
    uint64_t next_id;
    uint64_t remaining;
    uint32_t head;
    uint32_t tail;
    uint32_t shutdown;  // 1 => apagar ordenadamente

    // Buffer
    Registro buffer[SLOTS];
} SharedData;

// Punteros/semáforos globales (mapeados al iniciar)
extern SharedData *g_shm;
extern sem_t *g_sem_empty;
extern sem_t *g_sem_full;
extern sem_t *g_sem_buf_mutex;
extern sem_t *g_sem_id_mutex;

// Inicializa SHM + semáforos (0 ok, -1 error)
int ipc_init(uint64_t total_registros);

// Limpia/desmapea y hace unlink (idempotente)
void ipc_cleanup(void);

// Debug
void ipc_print_state(void);

// Señalizar shutdown y desbloquear waiters
void ipc_notify_shutdown(void);

/**
 * Espera "cancelable" con timeout en ms (portable).
 * Devuelve 0 si tomó el semáforo, -1 en error/timeout (consultar errno):
 *  - errno == ETIMEDOUT  => timeout
 *  - otro errno          => error real
 */
int sem_wait_cancellable(sem_t *sem, int timeout_ms);

#endif // IPC_H