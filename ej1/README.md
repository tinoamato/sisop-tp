TP1 – Sistemas Operativos (Ejercicio 1)
---------------------------------------

Descripción
-----------
Este programa implementa un sistema concurrente con:
- Un proceso coordinador (padre).
- Varios generadores (hijos) que producen datos de sensores.

Cada generador simula un sensor ambiental que produce lecturas de:
- Temperatura (15–30 °C).
- Humedad (30–99 %).
- SensorID (número de generador).

Los datos se guardan en un archivo CSV (`salida.csv`) con el siguiente formato:

ID,SensorID,Temperatura,Humedad
1,1,22,65
2,2,28,70
...

Se garantiza que:
- Los IDs van del 1 al N, sin huecos ni duplicados.
- Los procesos corren en paralelo y escriben concurrentemente.
- El orden de los registros en el archivo puede variar (porque es concurrente).

---------------------------------------

Compilación
-----------
En la raíz del proyecto:

  make

Esto genera el binario `tp1`.

Para limpiar:

  make clean

---------------------------------------

Ejecución
---------
Ejemplo recomendado (3 generadores, 500 registros):

  ./tp1 -g 3 -n 500

Esto genera un archivo `salida.csv` con 500 lecturas de sensores.
Con 0.5 segundos de retardo por registro, este caso dura varios minutos,
lo cual es útil para monitoreo en Linux (ps, htop, vmstat).

---------------------------------------

Pruebas automáticas
-------------------
Hay dos scripts de prueba:

1. Pruebas rápidas (smoke tests):

  ./scripts/run_tests.sh

Ejecuta combinaciones pequeñas (10–50 registros) y valida el CSV.

2. Pruebas largas (para monitoreo/documentación):

  ./scripts/run_long_tests.sh

Ejecuta con 200 y 500 registros, lo que permite observar la concurrencia.

Ambos scripts usan `scripts/validate.awk`, que verifica que los IDs estén
correlativos y sin duplicados, sin importar el orden en el archivo.

---------------------------------------

Validación manual
-----------------
Podés validar cualquier salida con:

  awk -f scripts/validate.awk salida.csv

Salida esperada:

  Validación OK: IDs correlativos y sin duplicados (orden indiferente).

---------------------------------------

Robustez
--------
- Ctrl+C (SIGINT): el coordinador limpia recursos y avisa shutdown.
- Caída de un generador: el resto continúa, el coordinador puede terminar
  con menos registros (se loguea advertencia).
- Caída del coordinador:
  - En Linux: gracias a `prctl`, los hijos reciben SIGTERM y finalizan.
  - En macOS: los hijos detectan si el padre murió (`getppid()==1`).
- SIGKILL (-9): no puede atraparse, por lo que puede quedar memoria compartida
  y semáforos colgados. En ese caso usar:

  ./scripts/cleanup_ipc.sh

---------------------------------------

Monitoreo (para el informe)
---------------------------
Estas pruebas deben documentarse en Linux:

1. Procesos en ejecución:
   ps -ef | grep tp1
   o bien con `htop`.

2. Recursos IPC:
   ls /dev/shm

3. Estadísticas de concurrencia:
   vmstat 1 5

Recomendado usar el caso largo:

   ./tp1 -g 3 -n 500

para obtener capturas claras de procesos concurrentes.

---------------------------------------

Informe / Documentación
------------------------
El informe debe incluir:

- Descripción del programa (simulación de sensores concurrentes).
- Ejemplos de ejecución con distintos parámetros.
- Validación con AWK mostrando "OK".
- Capturas de monitoreo en Linux (ps, htop, vmstat, /dev/shm).
- Pruebas de robustez (Ctrl+C, caída de hijos, etc.).
- Conclusiones sobre concurrencia y coordinación de procesos.

---------------------------------------

Autores
-------
- Grupo 04
- Materia: Sistemas Operativos - 3649
- Universidad Nacional de La Matanza
- Segundo Cuatrimestre 2025 - Comisión 01-1900