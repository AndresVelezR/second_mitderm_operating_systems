#include "filters.h"
#include "image.h"
#include "threading.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <sys/time.h>

// QUÉ: Estructura para pasar datos al hilo de Sobel.
// CÓMO: Contiene imagen original, gradientes Gx y Gy, y rango de trabajo.
// POR QUÉ: Permite calcular gradientes en paralelo por rangos de filas.
typedef struct {
    unsigned char*** pixelesOrigen;
    float** gradienteX;
    float** gradienteY;
    int inicio;
    int fin;
    int ancho;
    int alto;
} SobelArgs;

// QUÉ: Calcular gradientes Sobel en un rango de filas (para hilos).
// CÓMO: Aplica kernels Gx y Gy con convolución, guarda en matrices float.
// POR QUÉ: Permite paralelizar el cálculo de gradientes.
static void* calcularSobelHilo(void* args) {
    SobelArgs* sArgs = (SobelArgs*)args;

    // QUÉ: Definir kernels Sobel para Gx (horizontal) y Gy (vertical).
    // CÓMO: Gx detecta bordes verticales, Gy detecta bordes horizontales.
    // POR QUÉ: Son operadores de derivada optimizados con suavizado.
    int Gx[3][3] = {
        {-1, 0, 1},
        {-2, 0, 2},
        {-1, 0, 1}
    };
    int Gy[3][3] = {
        {-1, -2, -1},
        { 0,  0,  0},
        { 1,  2,  1}
    };

    for (int y = sArgs->inicio; y < sArgs->fin; y++) {
        for (int x = 0; x < sArgs->ancho; x++) {
            float sumX = 0.0f;
            float sumY = 0.0f;

            // QUÉ: Aplicar convolución con kernels Sobel.
            // CÓMO: Multiplica vecindario 3x3 por cada kernel.
            // POR QUÉ: Calcula aproximación de derivadas parciales.
            for (int ky = 0; ky < 3; ky++) {
                for (int kx = 0; kx < 3; kx++) {
                    int iy = y + ky - 1;
                    int ix = x + kx - 1;

                    // QUÉ: Manejo de bordes con replicación.
                    if (iy < 0) iy = 0;
                    if (iy >= sArgs->alto) iy = sArgs->alto - 1;
                    if (ix < 0) ix = 0;
                    if (ix >= sArgs->ancho) ix = sArgs->ancho - 1;

                    float pixel = (float)sArgs->pixelesOrigen[iy][ix][0];
                    sumX += pixel * Gx[ky][kx];
                    sumY += pixel * Gy[ky][kx];
                }
            }

            sArgs->gradienteX[y][x] = sumX;
            sArgs->gradienteY[y][x] = sumY;
        }
    }
    return NULL;
}

// QUÉ: Aplicar detector de bordes Sobel a la imagen.
// CÓMO: Convierte a grayscale, calcula Gx y Gy, computa magnitud del gradiente.
// POR QUÉ: Detecta bordes calculando cambios bruscos de intensidad en todas direcciones.
int aplicarSobel(ImagenInfo* info) {
    if (!imagenCargada(info)) {
        return 0;
    }

    struct timeval tiempo_inicio, tiempo_fin;
    gettimeofday(&tiempo_inicio, NULL);

    // QUÉ: Convertir a grayscale si es necesario.
    if (info->canales == 3) {
        if (!convertirAGrayscale(info)) {
            return 0;
        }
    }

    // QUÉ: Asignar matrices para gradientes Gx y Gy.
    // CÓMO: Usa float para precisión en cálculos intermedios.
    // POR QUÉ: Evita pérdida de precisión antes de calcular magnitud.
    float** gradienteX = (float**)malloc(info->alto * sizeof(float*));
    float** gradienteY = (float**)malloc(info->alto * sizeof(float*));
    if (!gradienteX || !gradienteY) {
        fprintf(stderr, "Error de memoria al asignar gradientes\n");
        if (gradienteX) free(gradienteX);
        if (gradienteY) free(gradienteY);
        return 0;
    }

    int memoriaOk = 1;
    for (int y = 0; y < info->alto && memoriaOk; y++) {
        gradienteX[y] = (float*)malloc(info->ancho * sizeof(float));
        gradienteY[y] = (float*)malloc(info->ancho * sizeof(float));
        if (!gradienteX[y] || !gradienteY[y]) {
            memoriaOk = 0;
            fprintf(stderr, "Error de memoria al asignar fila de gradientes\n");
            for (int yy = 0; yy <= y; yy++) {
                if (gradienteX[yy]) free(gradienteX[yy]);
                if (gradienteY[yy]) free(gradienteY[yy]);
            }
            free(gradienteX);
            free(gradienteY);
            return 0;
        }
    }

    int numHilos = NUM_HILOS_GLOBAL;
    if (numHilos > info->alto) {
        numHilos = info->alto;
        printf("INFO: Ajustando a %d hilos (imagen tiene solo %d filas)\n", numHilos, info->alto);
    }
    printf("INFO: Procesando Sobel con %d hilos en imagen de %dx%d...\n",
           numHilos, info->ancho, info->alto);

    pthread_t hilos[numHilos];
    SobelArgs args[numHilos];
    int filasPorHilo = (int)ceil((double)info->alto / numHilos);

    for (int i = 0; i < numHilos; i++) {
        args[i].pixelesOrigen = info->pixeles;
        args[i].gradienteX = gradienteX;
        args[i].gradienteY = gradienteY;
        args[i].inicio = i * filasPorHilo;
        args[i].fin = ((i + 1) * filasPorHilo < info->alto) ? (i + 1) * filasPorHilo : info->alto;
        args[i].ancho = info->ancho;
        args[i].alto = info->alto;
        printf("Lanzando hilos...\n");
        if (pthread_create(&hilos[i], NULL, calcularSobelHilo, &args[i]) != 0) {
            fprintf(stderr, "Error al crear hilo %d\n", i);
            for (int j = 0; j < i; j++) {
                pthread_join(hilos[j], NULL);
            }
            printf("\nTodos los hilos completados.\n");
            for (int y = 0; y < info->alto; y++) {
                free(gradienteX[y]);
                free(gradienteY[y]);
            }
            free(gradienteX);
            free(gradienteY);
            return 0;
        }
    }

    // Esperar hilos
    for (int i = 0; i < numHilos; i++) {
        pthread_join(hilos[i], NULL);
    }
    printf("\nTodos los hilos completados.\n");
    // QUÉ: Calcular magnitud del gradiente y actualizar imagen.
    // CÓMO: |∇I| = sqrt(Gx² + Gy²), clamp a [0, 255].
    // POR QUÉ: La magnitud indica la intensidad del borde.
    for (int y = 0; y < info->alto; y++) {
        for (int x = 0; x < info->ancho; x++) {
            float gx = gradienteX[y][x];
            float gy = gradienteY[y][x];
            float magnitud = sqrtf(gx * gx + gy * gy);

            // QUÉ: Clamp a [0, 255].
            int valor = (int)(magnitud + 0.5f);
            if (valor > 255) valor = 255;
            if (valor < 0) valor = 0;

            info->pixeles[y][x][0] = (unsigned char)valor;
        }
    }

    // Liberar gradientes
    for (int y = 0; y < info->alto; y++) {
        free(gradienteX[y]);
        free(gradienteY[y]);
    }
    free(gradienteX);
    free(gradienteY);

    gettimeofday(&tiempo_fin, NULL);
    double tiempo_total = obtenerTiempoReal(tiempo_inicio, tiempo_fin);

    printf("\n╔══════════════════════════════════════════════════════╗\n");
    printf("║              RESULTADOS SOBEL                        ║\n");
    printf("╚══════════════════════════════════════════════════════╝\n");
    printf("  • Tiempo total: %.4f segundos\n", tiempo_total);
    printf("  • Hilos utilizados: %d\n", numHilos);
    printf("  • Throughput: %.0f píxeles/seg\n",
           (info->ancho * info->alto) / tiempo_total);
    printf("\n");

    return 1;
}
