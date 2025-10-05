// Declaraciones de funciones relacionadas al escalado de imágenes.
// Prototipo a exponer: void scaleImageConcurrently(ImageInfo* info, int newWidth, int newHeight);

#ifndef ESCALAR_H
#define ESCALAR_H

#include "image.h"

// Estructura para pasar argumentos a cada hilo de escalado
typedef struct {
    ImagenInfo* originalImage;
    ImagenInfo* resultImage;
    int startRow;
    int endRow;
    float scaleFactorX;
    float scaleFactorY;
} ScaleArgs;

// Función principal que llama a los hilos
void scaleImageConcurrently(ImagenInfo* info, int newWidth, int newHeight);

#endif
