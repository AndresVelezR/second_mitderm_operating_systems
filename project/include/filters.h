#ifndef FILTERS_H
#define FILTERS_H

#include "image.h"

// QUÉ: Ajustar brillo de la imagen usando múltiples hilos con monitoreo.
// CÓMO: Divide las filas entre hilos, registra tiempos y muestra estadísticas.
// POR QUÉ: Demuestra paralelización con evidencia visual clara.
void ajustarBrilloConcurrente(ImagenInfo* info, int delta);

// QUÉ: Aplicar filtro Gaussiano a la imagen usando convolución concurrente.
// CÓMO: Genera kernel, crea matriz destino, divide trabajo entre hilos.
// POR QUÉ: Paraleliza el procesamiento para acelerar la operación costosa.
int aplicarConvolucionGaussiana(ImagenInfo* info, int tamKernel, float sigma);

// QUÉ: Aplicar detector de bordes Sobel a la imagen.
// CÓMO: Convierte a grayscale, calcula Gx y Gy, computa magnitud del gradiente.
// POR QUÉ: Detecta bordes calculando cambios bruscos de intensidad en todas direcciones.
int aplicarSobel(ImagenInfo* info);

#endif // FILTERS_H
