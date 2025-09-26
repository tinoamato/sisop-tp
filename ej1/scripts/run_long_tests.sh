#!/bin/bash
set -e

make

for G in 2 3; do
  for N in 200 500; do
    echo "===> Test largo con $G generadores y $N registros"
    ./tp1 -g $G -n $N
    awk -f scripts/validate.awk salida.csv || exit 1
  done
done

echo "✔️ Todos los tests largos pasaron"