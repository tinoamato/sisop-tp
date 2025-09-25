#!/bin/bash
set -e

echo "[cleanup] Intentando eliminar semáforos POSIX del TP en /dev/shm ..."
if [ -d /dev/shm ]; then
  sudo rm -f /dev/shm/sem.tp1_sem_* 2>/dev/null || true
  sudo rm -f /dev/shm/sem.*tp1*     2>/dev/null || true
fi

echo "[cleanup] Intentando unlink de SHM (si queda colgado) ..."
# No hay una herramienta estándar para POSIX shm unlink por nombre
# pero al reejecutar tu programa, éste hace shm_unlink(SHM_NAME).

echo "[cleanup] Listo. Si persisten recursos, reiniciar o inspeccionar /dev/shm manualmente."