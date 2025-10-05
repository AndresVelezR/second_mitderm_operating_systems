// Implementación pendiente de scaleImageConcurrently.
// Dividir el trabajo entre 2-4 hilos, cada uno procesa un rango de filas de la imagen destino.
// Utilizar interpolación bilineal y reemplazar la imagen original por la redimensionada.

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include "scaling.h"

// Función que calcula interpolación bilineal
static unsigned char interpolatePixel(ImagenInfo* img, float x, float y, int channel) {
    int x0 = (int)floor(x);
    int y0 = (int)floor(y);
    int x1 = x0 + 1;
    int y1 = y0 + 1;

    // Clamp to image boundaries
    if (x1 >= img->ancho)  x1 = img->ancho - 1;
    if (y1 >= img->alto) y1 = img->alto - 1;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;

    float dx = x - x0;
    float dy = y - y0;

    // Get pixel values
    float TL = img->pixeles[y0][x0][channel];
    float TR = img->pixeles[y0][x1][channel];
    float BL = img->pixeles[y1][x0][channel];
    float BR = img->pixeles[y1][x1][channel];

    // Bilinear interpolation
    float top = TL * (1 - dx) + TR * dx;
    float bottom = BL * (1 - dx) + BR * dx;
    float value = top * (1 - dy) + bottom * dy;

    // Clamp final result
    if (value < 0) value = 0;
    if (value > 255) value = 255;

    return (unsigned char)value;
}

// Función que ejecutará cada hilo
void* scaleThread(void* args) {
    ScaleArgs* threadArgs = (ScaleArgs*)args;

    ImagenInfo* src = threadArgs->originalImage;
    ImagenInfo* dst = threadArgs->resultImage;

    for (int y = threadArgs->startRow; y < threadArgs->endRow; y++) {
        for (int x = 0; x < dst->ancho; x++) {
            float srcX = x * threadArgs->scaleFactorX;
            float srcY = y * threadArgs->scaleFactorY;

            for (int c = 0; c < src->canales; c++) {
                dst->pixeles[y][x][c] = interpolatePixel(src, srcX, srcY, c);
            }
        }
    }

    pthread_exit(NULL);
}

// Función principal de escalado concurrente
void scaleImageConcurrently(ImagenInfo* info, int newancho, int newalto) {
    if (!info->pixeles) {
        printf("No hay imagen cargada.\n");
        return;
    }

    ImagenInfo resized;
    resized.ancho = newancho;
    resized.alto = newalto;
    resized.canales = info->canales;

    // Reservar memoria para la nueva matriz
    resized.pixeles = (unsigned char***)malloc(resized.alto * sizeof(unsigned char**));
    for (int y = 0; y < resized.alto; y++) {
        resized.pixeles[y] = (unsigned char**)malloc(resized.ancho * sizeof(unsigned char*));
        for (int x = 0; x < resized.ancho; x++) {
            resized.pixeles[y][x] = (unsigned char*)malloc(resized.canales * sizeof(unsigned char));
        }
    }

    // Preparar concurrencia
    int threadCount = 4;
    pthread_t threads[threadCount];
    ScaleArgs args[threadCount];

    int rowsPerThread = (int)ceil((double)resized.alto / threadCount);
    float scaleFactorX = (float)info->ancho / newancho;
    float scaleFactorY = (float)info->alto / newalto;

    for (int i = 0; i < threadCount; i++) {
        args[i].originalImage = info;
        args[i].resultImage = &resized;
        args[i].startRow = i * rowsPerThread;
        args[i].endRow = ((i + 1) * rowsPerThread < resized.alto) ? (i + 1) * rowsPerThread : resized.alto;
        args[i].scaleFactorX = scaleFactorX;
        args[i].scaleFactorY = scaleFactorY;
        pthread_create(&threads[i], NULL, scaleThread, &args[i]);
    }

    for (int i = 0; i < threadCount; i++) {
        pthread_join(threads[i], NULL);
    }

    // Reemplazar imagen original
    liberarImagen(info);
    *info = resized;

    printf("Imagen escalada concurrentemente a %dx%d con %d hilos.\n", newancho, newalto, threadCount);
}
