#!/bin/bash
set -e

make

for G in 1 2 3 5; do
  for N in 10 20 50; do
    echo "===> Test con $G generadores y $N registros"
    ./tp1 -g $G -n $N
    awk -f scripts/validate.awk salida.csv || exit 1
  done
done

echo "✔️ Todos los tests pasaron"