#include "image.h"
#include <stdio.h>
#include <stdlib.h>

// QUÉ: Liberar memoria asignada para la imagen.
// CÓMO: Libera cada fila y canal de la matriz 3D, luego el arreglo de filas y
// reinicia la estructura.
// POR QUÉ: Evita fugas de memoria, esencial en C para manejar recursos manualmente.
void liberarImagen(ImagenInfo* info) {
    if (info->pixeles) {
        for (int y = 0; y < info->alto; y++) {
            for (int x = 0; x < info->ancho; x++) {
                free(info->pixeles[y][x]); // Liberar canales por píxel
            }
            free(info->pixeles[y]); // Liberar fila
        }
        free(info->pixeles); // Liberar arreglo de filas
        info->pixeles = NULL;
    }
    info->ancho = 0;
    info->alto = 0;
    info->canales = 0;
}

// QUÉ: Verificar si hay una imagen cargada en memoria.
// CÓMO: Comprueba que el puntero de píxeles no sea NULL.
// POR QUÉ: Evita código repetitivo y centraliza la validación.
int imagenCargada(const ImagenInfo* info) {
    if (!info || !info->pixeles) {
        fprintf(stderr, "ERROR: No hay imagen cargada. Carga una imagen primero (opción 1).\n");
        return 0;
    }
    return 1;
}

// QUÉ: Convertir imagen RGB a escala de grises.
// CÓMO: Usa ponderación perceptual (0.299R + 0.587G + 0.114B).
// POR QUÉ: Sobel requiere imagen de un solo canal para calcular gradientes.
int convertirAGrayscale(ImagenInfo* info) {
    if (!imagenCargada(info)) {
        return 0;
    }
    if (info->canales == 1) {
        printf("La imagen ya está en escala de grises.\n");
        return 1; // Ya es grayscale
    }
    if (info->canales != 3) {
        fprintf(stderr, "Error: formato de imagen no soportado (canales=%d).\n", info->canales);
        return 0;
    }

    // QUÉ: Crear nueva matriz para grayscale (1 canal).
    unsigned char*** pixelesGray = (unsigned char***)malloc(info->alto * sizeof(unsigned char**));
    if (!pixelesGray) {
        fprintf(stderr, "Error de memoria al asignar grayscale\n");
        return 0;
    }

    int memoriaOk = 1;
    for (int y = 0; y < info->alto && memoriaOk; y++) {
        pixelesGray[y] = (unsigned char**)malloc(info->ancho * sizeof(unsigned char*));
        if (!pixelesGray[y]) {
            memoriaOk = 0;
            for (int yy = 0; yy < y; yy++) {
                for (int x = 0; x < info->ancho; x++) {
                    free(pixelesGray[yy][x]);
                }
                free(pixelesGray[yy]);
            }
            free(pixelesGray);
            return 0;
        }

        for (int x = 0; x < info->ancho && memoriaOk; x++) {
            pixelesGray[y][x] = (unsigned char*)malloc(1 * sizeof(unsigned char));
            if (!pixelesGray[y][x]) {
                memoriaOk = 0;
                for (int xx = 0; xx < x; xx++) {
                    free(pixelesGray[y][xx]);
                }
                for (int yy = 0; yy < y; yy++) {
                    for (int xx = 0; xx < info->ancho; xx++) {
                        free(pixelesGray[yy][xx]);
                    }
                    free(pixelesGray[yy]);
                }
                free(pixelesGray[y]);
                free(pixelesGray);
                return 0;
            }

            // QUÉ: Calcular valor grayscale usando ponderación ITU-R BT.601.
            // CÓMO: Gray = 0.299*R + 0.587*G + 0.114*B
            // POR QUÉ: Refleja la sensibilidad perceptual del ojo humano.
            float r = (float)info->pixeles[y][x][0];
            float g = (float)info->pixeles[y][x][1];
            float b = (float)info->pixeles[y][x][2];
            float gray = 0.299f * r + 0.587f * g + 0.114f * b;
            pixelesGray[y][x][0] = (unsigned char)(gray + 0.5f); // Redondeo
        }
    }

    // QUÉ: Reemplazar imagen original con grayscale.
    for (int y = 0; y < info->alto; y++) {
        for (int x = 0; x < info->ancho; x++) {
            free(info->pixeles[y][x]);
        }
        free(info->pixeles[y]);
    }
    free(info->pixeles);
    info->pixeles = pixelesGray;
    info->canales = 1;

    printf("Imagen convertida a escala de grises.\n");
    return 1;
}
