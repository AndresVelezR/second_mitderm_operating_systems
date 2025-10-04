#include "filters.h"
#include "image.h"
#include "threading.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <math.h>
#include <sys/time.h>

// QUÉ: Estructura para pasar datos al hilo de ajuste de brillo.
// CÓMO: Contiene matriz, rango de filas, ancho, canales y delta de brillo.
// POR QUÉ: Los hilos necesitan datos específicos para procesar en paralelo.
typedef struct {
    unsigned char*** pixeles;
    int inicio;
    int fin;
    int ancho;
    int canales;
    int delta;
} BrilloArgs;

// QUÉ: Ajustar brillo en un rango de filas (para hilos) con monitoreo.
// CÓMO: Suma delta a cada canal, registra inicio/fin y progreso.
// POR QUÉ: Permite visualizar el trabajo de cada hilo.
static void* ajustarBrilloHilo(void* args) {
    BrilloArgs* bArgs = (BrilloArgs*)args;

    // Registrar inicio
    struct timeval tiempo_inicio;
    gettimeofday(&tiempo_inicio, NULL);

    int pixeles_procesados = 0;
    for (int y = bArgs->inicio; y < bArgs->fin; y++) {
        for (int x = 0; x < bArgs->ancho; x++) {
            for (int c = 0; c < bArgs->canales; c++) {
                int nuevoValor = bArgs->pixeles[y][x][c] + bArgs->delta;
                bArgs->pixeles[y][x][c] = (unsigned char)(nuevoValor < 0 ? 0 :
                                                          (nuevoValor > 255 ? 255 : nuevoValor));
            }
            pixeles_procesados++;
        }
    }

    // Registrar fin
    struct timeval tiempo_fin;
    gettimeofday(&tiempo_fin, NULL);
    double tiempo_hilo = obtenerTiempoReal(tiempo_inicio, tiempo_fin);

    // Mostrar progreso del hilo
    printf("  [Hilo #%d] Completado: %d filas (%d-%d), %d píxeles, %.4f seg\n",
           (int)(bArgs->inicio / ((bArgs->fin - bArgs->inicio) > 0 ? (bArgs->fin - bArgs->inicio) : 1)),
           bArgs->fin - bArgs->inicio,
           bArgs->inicio,
           bArgs->fin - 1,
           pixeles_procesados,
           tiempo_hilo);

    return NULL;
}

// QUÉ: Ajustar brillo de la imagen usando múltiples hilos con monitoreo.
// CÓMO: Divide las filas entre hilos, registra tiempos y muestra estadísticas.
// POR QUÉ: Demuestra paralelización con evidencia visual clara.
void ajustarBrilloConcurrente(ImagenInfo* info, int delta) {
    if (!imagenCargada(info)) {
        return;
    }

    if (delta < -255 || delta > 255) {
        fprintf(stderr, "ADVERTENCIA: delta fuera de rango recomendado [-255, 255]\n");
        fprintf(stderr, "Se procesará, pero el efecto será equivalente a ±255\n");
    }

    int numHilos = NUM_HILOS_GLOBAL;
    if (numHilos > info->alto) {
        numHilos = info->alto;
    }

    // INICIO: Mostrar información de la operación
    printf("\n╔══════════════════════════════════════════════════════╗\n");
    printf("║           AJUSTE DE BRILLO PARALELO                 ║\n");
    printf("╚══════════════════════════════════════════════════════╝\n");
    printf("Configuración:\n");
    printf("  • Hilos activos: %d\n", numHilos);
    printf("  • Imagen: %dx%d (%s)\n", info->ancho, info->alto,
           info->canales == 1 ? "grayscale" : "RGB");
    printf("  • Total píxeles: %d\n", info->ancho * info->alto);
    printf("  • Delta brillo: %+d\n", delta);
    printf("\n");

    struct timeval tiempo_inicio, tiempo_fin;
    gettimeofday(&tiempo_inicio, NULL);

    pthread_t* hilos = (pthread_t*)malloc(numHilos * sizeof(pthread_t));
    BrilloArgs* args = (BrilloArgs*)malloc(numHilos * sizeof(BrilloArgs));
    if (!hilos || !args) {
        fprintf(stderr, "Error de memoria al asignar hilos\n");
        if (hilos) free(hilos);
        if (args) free(args);
        return;
    }

    int filasPorHilo = (int)ceil((double)info->alto / numHilos);

    printf("Iniciando procesamiento paralelo...\n");

    // Crear hilos
    for (int i = 0; i < numHilos; i++) {
        args[i].pixeles = info->pixeles;
        args[i].inicio = i * filasPorHilo;
        args[i].fin = ((i + 1) * filasPorHilo < info->alto) ? (i + 1) * filasPorHilo : info->alto;
        args[i].ancho = info->ancho;
        args[i].canales = info->canales;
        args[i].delta = delta;
        printf("Lanzando hilos...\n");
        if (pthread_create(&hilos[i], NULL, ajustarBrilloHilo, &args[i]) != 0) {
            fprintf(stderr, "Error al crear hilo %d\n", i);
            for (int j = 0; j < i; j++) {
                pthread_join(hilos[j], NULL);
            }
            printf("\nTodos los hilos completados.\n");
            free(hilos);
            free(args);
            return;
        }
        printf("  [Hilo #%d] Lanzado: procesará filas %d-%d\n",
               i, args[i].inicio, args[i].fin - 1);
    }

    printf("\n");

    // Esperar hilos
    for (int i = 0; i < numHilos; i++) {
        pthread_join(hilos[i], NULL);
    }
    printf("\nTodos los hilos completados.\n");
    gettimeofday(&tiempo_fin, NULL);
    double tiempo_total = obtenerTiempoReal(tiempo_inicio, tiempo_fin);

    // REPORTE FINAL
    printf("\n");
    printf("╔══════════════════════════════════════════════════════╗\n");
    printf("║              RESULTADOS DEL PROCESAMIENTO           ║\n");
    printf("╚══════════════════════════════════════════════════════╝\n");
    printf("Estadísticas:\n");
    printf("  • Tiempo total: %.4f segundos\n", tiempo_total);
    printf("  • Hilos utilizados: %d\n", numHilos);
    printf("  • Píxeles procesados: %d\n", info->ancho * info->alto);
    printf("  • Throughput: %.0f píxeles/seg\n",
           (info->ancho * info->alto) / tiempo_total);
    printf("  • Eficiencia: %.1f%% (ideal: %.1f%%)\n",
           (1.0 / (numHilos * tiempo_total)) * 100.0 * numHilos,
           100.0);
    printf("\n");

    free(hilos);
    free(args);
}
