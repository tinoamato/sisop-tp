#!/bin/bash
# Limpieza manual de recursos IPC (por si quedó colgado tras un kill -9)
set -e

echo "Eliminando semáforos y memoria compartida..."
ipcrm -M 0x$(echo -n "/tp1_shm" | od -An -tx1 | tr -d ' ')
ipcrm -S 0x$(echo -n "/tp1_sem_empty" | od -An -tx1 | tr -d ' ')
ipcrm -S 0x$(echo -n "/tp1_sem_full" | od -An -tx1 | tr -d ' ')
ipcrm -S 0x$(echo -n "/tp1_sem_buf_mutex" | od -An -tx1 | tr -d ' ')
ipcrm -S 0x$(echo -n "/tp1_sem_id_mutex" | od -An -tx1 | tr -d ' ')
echo "✔️ Recursos IPC eliminados."