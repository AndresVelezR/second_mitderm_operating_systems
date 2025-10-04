#ifndef THREADING_H
#define THREADING_H

#include <sys/time.h>

// QUÉ: Límites para número de hilos.
// CÓMO: Constantes que definen rango válido.
// POR QUÉ: Evita valores absurdos (muy pocos o demasiados hilos).
#define MIN_HILOS 1
#define MAX_HILOS 16

// QUÉ: Variable global para número de hilos configurable.
// CÓMO: Se modifica desde el menú, se usa en todas las funciones paralelas.
// POR QUÉ: Permite demostrar escalabilidad y cumple con requisito de configurabilidad.
extern int NUM_HILOS_GLOBAL;

// QUÉ: Calcular tiempo real transcurrido en segundos.
// CÓMO: Usa gettimeofday (tiempo de reloj de pared, no CPU time).
// POR QUÉ: clock() suma tiempo de todos los hilos, no muestra paralelización real.
double obtenerTiempoReal(struct timeval inicio, struct timeval fin);

#endif // THREADING_H
