#include "threading.h"

// QUÉ: Variable global para número de hilos configurable.
// CÓMO: Se modifica desde el menú, se usa en todas las funciones paralelas.
// POR QUÉ: Permite demostrar escalabilidad y cumple con requisito de configurabilidad.
int NUM_HILOS_GLOBAL = 4; // Valor por defecto: 4 hilos

// QUÉ: Calcular tiempo real transcurrido en segundos.
// CÓMO: Usa gettimeofday (tiempo de reloj de pared, no CPU time).
// POR QUÉ: clock() suma tiempo de todos los hilos, no muestra paralelización real.
double obtenerTiempoReal(struct timeval inicio, struct timeval fin) {
    long segundos = fin.tv_sec - inicio.tv_sec;
    long microsegundos = fin.tv_usec - inicio.tv_usec;
    return segundos + microsegundos / 1000000.0;
}
