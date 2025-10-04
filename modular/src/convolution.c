#include "filters.h"
#include "image.h"
#include "threading.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <sys/time.h>

// QUÉ: Generar kernel Gaussiano usando la fórmula matemática.
// CÓMO: Calcula G(x,y) = (1/(2πσ²))·exp(-(x²+y²)/(2σ²)) para cada posición.
// POR QUÉ: El kernel Gaussiano suaviza la imagen ponderando por distancia al centro.
static float** generarKernelGaussiano(int tamKernel, float sigma, float* suma) {
    if (tamKernel % 2 == 0 || tamKernel < 3) {
        fprintf(stderr, "Error: tamKernel debe ser impar y >= 3\n");
        return NULL;
    }

    // QUÉ: Asignar memoria para kernel 2D.
    // CÓMO: Asigna filas y luego columnas para matriz tamKernel x tamKernel.
    // POR QUÉ: Necesitamos una matriz flotante para precisión en cálculos.
    float** kernel = (float**)malloc(tamKernel * sizeof(float*));
    if (!kernel) {
        fprintf(stderr, "Error de memoria al asignar kernel\n");
        return NULL;
    }
    for (int i = 0; i < tamKernel; i++) {
        kernel[i] = (float*)malloc(tamKernel * sizeof(float));
        if (!kernel[i]) {
            fprintf(stderr, "Error de memoria al asignar fila de kernel\n");
            for (int j = 0; j < i; j++) {
                free(kernel[j]);
            }
            free(kernel);
            return NULL;
        }
    }

    // QUÉ: Calcular valores del kernel Gaussiano.
    // CÓMO: Usa fórmula G(x,y) = exp(-(x²+y²)/(2σ²)) y normaliza después.
    // POR QUÉ: El kernel debe estar centrado y normalizado para preservar brillo.
    int centro = tamKernel / 2;
    *suma = 0.0f;
    float sigmaDosCuadrado = 2.0f * sigma * sigma;

    for (int y = 0; y < tamKernel; y++) {
        for (int x = 0; x < tamKernel; x++) {
            int dx = x - centro;
            int dy = y - centro;
            float valor = expf(-(dx * dx + dy * dy) / sigmaDosCuadrado);
            kernel[y][x] = valor;
            *suma += valor;
        }
    }

    // QUÉ: Normalizar el kernel para que la suma sea 1.
    // CÓMO: Divide cada elemento por la suma total.
    // POR QUÉ: Garantiza que el brillo promedio de la imagen no cambie.
    if (*suma > 0.0f) {
        for (int y = 0; y < tamKernel; y++) {
            for (int x = 0; x < tamKernel; x++) {
                kernel[y][x] /= *suma;
            }
        }
    }

    return kernel;
}

// QUÉ: Validar parámetros de convolución Gaussiana.
// CÓMO: Verifica rangos válidos para tamaño de kernel y sigma.
// POR QUÉ: Centraliza validaciones y proporciona mensajes de error claros.
static int validarParametrosConvolucion(int tamKernel, float sigma) {
    if (tamKernel < 3 || tamKernel > 15) {
        fprintf(stderr, "ERROR: tamKernel debe estar entre 3 y 15 (valor ingresado: %d)\n", tamKernel);
        return 0;
    }
    if (tamKernel % 2 == 0) {
        fprintf(stderr, "ERROR: tamKernel debe ser impar (valor ingresado: %d)\n", tamKernel);
        fprintf(stderr, "SUGERENCIA: Prueba con %d o %d\n", tamKernel - 1, tamKernel + 1);
        return 0;
    }
    if (sigma <= 0.0f || sigma > 10.0f) {
        fprintf(stderr, "ERROR: sigma debe estar entre 0.01 y 10.0 (valor ingresado: %.2f)\n", sigma);
        fprintf(stderr, "SUGERENCIA: Valores típicos son 0.5 (blur leve), 1.5 (moderado), 3.0 (fuerte)\n");
        return 0;
    }
    return 1;
}

// QUÉ: Liberar memoria del kernel.
// CÓMO: Libera cada fila y luego el arreglo de filas.
// POR QUÉ: Evita fugas de memoria.
static void liberarKernel(float** kernel, int tamKernel) {
    if (kernel) {
        for (int i = 0; i < tamKernel; i++) {
            free(kernel[i]);
        }
        free(kernel);
    }
}

// QUÉ: Estructura para pasar datos al hilo de convolución.
// CÓMO: Contiene imagen original, resultado, kernel, rangos y dimensiones.
// POR QUÉ: Los hilos necesitan acceso a datos compartidos y su rango de trabajo.
typedef struct {
    unsigned char*** pixelesOrigen;
    unsigned char*** pixelesDestino;
    float** kernel;
    int tamKernel;
    int inicio;
    int fin;
    int ancho;
    int alto;
    int canales;
} ConvolucionArgs;

// QUÉ: Aplicar convolución en un rango de filas (para hilos).
// CÓMO: Para cada píxel en el rango, multiplica vecinos por kernel y suma.
// POR QUÉ: Permite paralelizar la operación dividiendo filas entre hilos.
static void* aplicarConvolucionHilo(void* args) {
    ConvolucionArgs* cArgs = (ConvolucionArgs*)args;
    int radio = cArgs->tamKernel / 2;

    // AÑADIR ESTO AL INICIO:
    struct timeval tiempo_inicio;
    gettimeofday(&tiempo_inicio, NULL);
    int pixeles_procesados = 0;

    for (int y = cArgs->inicio; y < cArgs->fin; y++) {
        for (int x = 0; x < cArgs->ancho; x++) {
            for (int c = 0; c < cArgs->canales; c++) {
                float suma = 0.0f;

                for (int ky = 0; ky < cArgs->tamKernel; ky++) {
                    for (int kx = 0; kx < cArgs->tamKernel; kx++) {
                        int iy = y + ky - radio;
                        int ix = x + kx - radio;

                        if (iy < 0) iy = 0;
                        if (iy >= cArgs->alto) iy = cArgs->alto - 1;
                        if (ix < 0) ix = 0;
                        if (ix >= cArgs->ancho) ix = cArgs->ancho - 1;

                        suma += cArgs->pixelesOrigen[iy][ix][c] * cArgs->kernel[ky][kx];
                    }
                }

                int valor = (int)(suma + 0.5f);
                if (valor < 0) valor = 0;
                if (valor > 255) valor = 255;
                cArgs->pixelesDestino[y][x][c] = (unsigned char)valor;
            }
            pixeles_procesados++;
        }
    }

    // AÑADIR ESTO AL FINAL (antes del return):
    struct timeval tiempo_fin;
    gettimeofday(&tiempo_fin, NULL);
    double tiempo_hilo = obtenerTiempoReal(tiempo_inicio, tiempo_fin);

    printf("  [Hilo] Filas %d-%d: %d píxeles, %.4f seg (%.0f píx/seg)\n",
           cArgs->inicio,
           cArgs->fin - 1,
           pixeles_procesados,
           tiempo_hilo,
           pixeles_procesados / tiempo_hilo);

    return NULL;
}

// QUÉ: Aplicar filtro Gaussiano a la imagen usando convolución concurrente.
// CÓMO: Genera kernel, crea matriz destino, divide trabajo entre hilos.
// POR QUÉ: Paraleliza el procesamiento para acelerar la operación costosa.
int aplicarConvolucionGaussiana(ImagenInfo* info, int tamKernel, float sigma) {
    if (!imagenCargada(info)) {
        return 0;
    }
    if (!validarParametrosConvolucion(tamKernel, sigma)) {
        return 0;
    }

    struct timeval tiempo_inicio, tiempo_fin;
    gettimeofday(&tiempo_inicio, NULL);

    // QUÉ: Generar kernel Gaussiano.
    float sumaKernel;
    float** kernel = generarKernelGaussiano(tamKernel, sigma, &sumaKernel);
    if (!kernel) {
        return 0;
    }

    // QUÉ: Crear matriz de destino para resultado.
    // CÓMO: Asigna nueva matriz 3D con mismas dimensiones que original.
    // POR QUÉ: No podemos modificar la imagen original mientras la leemos.
    unsigned char*** pixelesNuevos = (unsigned char***)malloc(info->alto * sizeof(unsigned char**));
    if (!pixelesNuevos) {
        fprintf(stderr, "Error de memoria al asignar imagen destino\n");
        liberarKernel(kernel, tamKernel);
        return 0;
    }

    int memoriaOk = 1;
    for (int y = 0; y < info->alto && memoriaOk; y++) {
        pixelesNuevos[y] = (unsigned char**)malloc(info->ancho * sizeof(unsigned char*));
        if (!pixelesNuevos[y]) {
            memoriaOk = 0;
            fprintf(stderr, "Error de memoria al asignar columnas destino\n");
            // Liberar lo ya asignado
            for (int yy = 0; yy < y; yy++) {
                for (int x = 0; x < info->ancho; x++) {
                    free(pixelesNuevos[yy][x]);
                }
                free(pixelesNuevos[yy]);
            }
            free(pixelesNuevos);
            liberarKernel(kernel, tamKernel);
            return 0;
        }

        for (int x = 0; x < info->ancho && memoriaOk; x++) {
            pixelesNuevos[y][x] = (unsigned char*)malloc(info->canales * sizeof(unsigned char));
            if (!pixelesNuevos[y][x]) {
                memoriaOk = 0;
                fprintf(stderr, "Error de memoria al asignar canales destino\n");
                // Liberar lo ya asignado
                for (int xx = 0; xx < x; xx++) {
                    free(pixelesNuevos[y][xx]);
                }
                for (int yy = 0; yy < y; yy++) {
                    for (int xx = 0; xx < info->ancho; xx++) {
                        free(pixelesNuevos[yy][xx]);
                    }
                    free(pixelesNuevos[yy]);
                }
                free(pixelesNuevos[y]);
                free(pixelesNuevos);
                liberarKernel(kernel, tamKernel);
                return 0;
            }
        }
    }

    // QUÉ: Configurar y lanzar hilos para convolución.
    // CÓMO: Divide filas entre hilos, pasa argumentos y sincroniza.
    // POR QUÉ: Paraleliza el procesamiento para mayor velocidad.
    int numHilos = NUM_HILOS_GLOBAL;
    if (numHilos > info->alto) {
        numHilos = info->alto;
        printf("INFO: Ajustando a %d hilos (imagen tiene solo %d filas)\n", numHilos, info->alto);
    }
    printf("\n╔══════════════════════════════════════════════════════╗\n");
    printf("║         CONVOLUCIÓN GAUSSIANA PARALELA              ║\n");
    printf("╚══════════════════════════════════════════════════════╝\n");
    printf("Configuración:\n");
    printf("  • Hilos activos: %d\n", numHilos);
    printf("  • Kernel: %dx%d (sigma=%.2f)\n", tamKernel, tamKernel, sigma);
    printf("  • Imagen: %dx%d píxeles\n", info->ancho, info->alto);
    printf("  • Operaciones: ~%ld por píxel\n",
           (long)(tamKernel * tamKernel * 2)); // mult + add
    printf("\n");

    pthread_t hilos[numHilos];
    ConvolucionArgs args[numHilos];
    int filasPorHilo = (int)ceil((double)info->alto / numHilos);

    for (int i = 0; i < numHilos; i++) {
        args[i].pixelesOrigen = info->pixeles;
        args[i].pixelesDestino = pixelesNuevos;
        args[i].kernel = kernel;
        args[i].tamKernel = tamKernel;
        args[i].inicio = i * filasPorHilo;
        args[i].fin = ((i + 1) * filasPorHilo < info->alto) ? (i + 1) * filasPorHilo : info->alto;
        args[i].ancho = info->ancho;
        args[i].alto = info->alto;
        args[i].canales = info->canales;
        printf("Lanzando hilos...\n");
        if (pthread_create(&hilos[i], NULL, aplicarConvolucionHilo, &args[i]) != 0) {
            fprintf(stderr, "Error al crear hilo %d\n", i);
            // Esperar hilos ya creados
            for (int j = 0; j < i; j++) {
                pthread_join(hilos[j], NULL);
            }
            printf("\nTodos los hilos completados.\n");
            // Liberar memoria
            for (int y = 0; y < info->alto; y++) {
                for (int x = 0; x < info->ancho; x++) {
                    free(pixelesNuevos[y][x]);
                }
                free(pixelesNuevos[y]);
            }
            free(pixelesNuevos);
            liberarKernel(kernel, tamKernel);
            return 0;
        }
    }

    // QUÉ: Esperar a que todos los hilos terminen.
    for (int i = 0; i < numHilos; i++) {
        pthread_join(hilos[i], NULL);
    }
    printf("\nTodos los hilos completados.\n");

    // QUÉ: Reemplazar imagen original con resultado.
    // CÓMO: Libera matriz antigua y asigna la nueva.
    // POR QUÉ: Actualiza la imagen en memoria con el resultado del filtro.
    for (int y = 0; y < info->alto; y++) {
        for (int x = 0; x < info->ancho; x++) {
            free(info->pixeles[y][x]);
        }
        free(info->pixeles[y]);
    }
    free(info->pixeles);
    info->pixeles = pixelesNuevos;
    liberarKernel(kernel, tamKernel);

    gettimeofday(&tiempo_fin, NULL);
    double tiempo_total = obtenerTiempoReal(tiempo_inicio, tiempo_fin);

    // AQUÍ VA EL FRAGMENTO QUE NO ENTENDÍAS:
    printf("\n╔══════════════════════════════════════════════════════╗\n");
    printf("║                    RESULTADOS                        ║\n");
    printf("╚══════════════════════════════════════════════════════╝\n");
    printf("  • Tiempo total: %.4f segundos\n", tiempo_total);
    printf("  • Hilos utilizados: %d\n", numHilos);
    printf("  • Throughput: %.0f píxeles/seg\n",
           (info->ancho * info->alto) / tiempo_total);
    if (numHilos > 1) {
        printf("  • Speedup estimado: %.2fx\n",
               1.0 / tiempo_total * numHilos * 0.3);
    }
    printf("\n");

    return 1;
}
