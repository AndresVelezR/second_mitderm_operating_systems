#ifndef BENCHMARK_H
#define BENCHMARK_H

#include "image.h"

// QUÉ: Ejecutar benchmark de paralelización con diferentes números de hilos.
// CÓMO: Prueba operaciones con 1, 2, 4, 8 hilos y muestra tabla comparativa.
// POR QUÉ: Demuestra mejora de rendimiento con paralelización.
void ejecutarBenchmark(ImagenInfo* imagen);

// QUÉ: Mostrar información del sistema y configuración actual.
// CÓMO: Imprime detalles de la imagen y configuración de hilos.
// POR QUÉ: Ayuda al usuario a entender el estado del programa.
void mostrarInformacion(const ImagenInfo* info);

#endif // BENCHMARK_H
