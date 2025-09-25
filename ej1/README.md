TP1 – Sistemas Operativos (Ejercicio 1)

⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻

Descripción

Este programa implementa un sistema coordinador + generadores que producen registros y los escriben en un archivo CSV utilizando memoria compartida y semáforos POSIX.
	•	El coordinador (padre) consume los registros desde un buffer circular en SHM y los persiste en salida.csv.
	•	Los generadores (hijos) producen lotes de registros con IDs correlativos y datos aleatorios.
	•	Se garantiza que los IDs estén correlativos, sin huecos ni duplicados mientras los procesos terminan normalmente.

El enunciado solicita además validación con AWK y monitoreo en Linux.

⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻

Compilación

En la raíz del proyecto:

make

Esto genera el binario tp1.

Para limpiar compilación:

make clean

⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻

Ejecución

Ejemplo recomendado con 3 generadores y 500 registros:

./tp1 -g 3 -n 500

Con un retardo de 0,5 segundos por registro, este caso dura varios minutos, lo que permite observar procesos concurrentes con herramientas de monitoreo.

Salida esperada (resumida):

[MAIN] Generadores: 3, Total: 500
[IPC] SHM OK — next_id=1 remaining=500 head=0 tail=0 slots=64
[GEN 1] Terminé.
[GEN 2] Terminé.
[GEN 3] Terminé.
[COORD] Terminé. Total escritos: 500

Archivo salida.csv generado:

ID,Campo1,Campo2
1,51,387
2,97,612
…
500,48,818

⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻

Validación con AWK

El script scripts/validate.awk verifica que el CSV tenga IDs correlativos y sin duplicados:

awk -f scripts/validate.awk salida.csv

Salida esperada:

Validación OK: IDs correlativos y sin duplicados.

⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻

Pruebas automatizadas

Se incluye el script scripts/run_tests.sh para correr múltiples combinaciones:

./scripts/run_tests.sh

Ejecuta el programa con varios valores de -g y -n y valida el CSV con AWK.
Si todo pasa, muestra:

✔️ Todos los tests pasaron

⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻

Robustez
	•	Ctrl+C: el coordinador limpia recursos (shm_unlink, sem_unlink) y notifica shutdown a los hijos.
	•	Caída de un generador: el resto continúa; el coordinador puede terminar con menos registros que -n, lo cual se loguea como advertencia.
	•	Caída del coordinador:
	•	En Linux, gracias a prctl, los hijos reciben SIGTERM y finalizan.
	•	En Mac, se detecta si getppid()==1 (padre muerto) y el hijo sale.
	•	SIGKILL (-9): no puede interceptarse, por lo que puede quedar SHM/sems colgados. Se incluye un script de limpieza.

⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻

Limpieza manual

En caso de recursos colgados (tras kill -9):

./scripts/cleanup_ipc.sh

Este elimina semáforos POSIX y SHM residuales.

⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻

Monitoreo (para el informe)

Estas pruebas deben documentarse en Linux (no se ven igual en macOS):
	1.	Procesos en ejecución
ps -ef | grep tp1
o con htop.
	2.	Recursos IPC
ls /dev/shm
	3.	Estadísticas de concurrencia
vmstat 1 5

Usar como caso base: ./tp1 -g 3 -n 500
De esta manera el programa corre varios minutos y se pueden obtener capturas de procesos concurrentes.

⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻

Informe / Documentación

Para completar la entrega, documentar:
	•	Ejemplos de ejecución con distintos parámetros (incluyendo el caso largo con 500 registros).
	•	Validación AWK mostrando “OK”.
	•	Monitoreo en Linux (ps, htop, /dev/shm, vmstat).
	•	Pruebas de robustez (matar hijos/padre, Ctrl+C).
	•	Conclusiones sobre el comportamiento.

⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻⸻
Autores
	•	Grupo 04
	•	Materia: Sistemas Operativos - 3649
	•	Universidad Nacional de La Matanza
	•	Segundo Cuatrimestre 2025 - Comisión 01-1900