#include "benchmark.h"
#include "filters.h"
#include "threading.h"
#include <stdio.h>
#include <sys/time.h>

// QUÉ: Ejecutar benchmark de paralelización con diferentes números de hilos.
// CÓMO: Prueba operaciones con 1, 2, 4, 8 hilos y muestra tabla comparativa.
// POR QUÉ: Demuestra mejora de rendimiento con paralelización.
void ejecutarBenchmark(ImagenInfo* imagen) {
    printf("\n");
    printf("╔══════════════════════════════════════════════════════╗\n");
    printf("║          BENCHMARK DE PARALELIZACIÓN                ║\n");
    printf("╚══════════════════════════════════════════════════════╝\n");
    printf("\n");
    printf("Este benchmark ejecutará operaciones con diferentes\n");
    printf("números de hilos para demostrar la mejora de rendimiento.\n");
    printf("\n");
    printf("Imagen: %dx%d píxeles (%d total)\n",
           imagen->ancho, imagen->alto, imagen->ancho * imagen->alto);
    printf("\n¿Continuar? (s/n): ");

    char respuesta;
    scanf(" %c", &respuesta);
    while (getchar() != '\n');

    if (respuesta != 's' && respuesta != 'S') {
        printf("Benchmark cancelado.\n");
        return;
    }

    // Guardar configuración original
    int hilos_original = NUM_HILOS_GLOBAL;

    // Arrays para guardar resultados
    double tiempos_brillo[4];    // 1, 2, 4, 8 hilos
    double tiempos_convolucion[4];
    int num_hilos[4] = {1, 2, 4, 8};

    printf("\n");
    printf("═══════════════════════════════════════════════════════\n");
    printf("PRUEBA 1: AJUSTE DE BRILLO (operación simple)\n");
    printf("═══════════════════════════════════════════════════════\n");

    for (int i = 0; i < 4; i++) {
        NUM_HILOS_GLOBAL = num_hilos[i];
        printf("\n[%d/%d] Ejecutando con %d hilo(s)...\n", i+1, 4, num_hilos[i]);

        struct timeval inicio, fin;
        gettimeofday(&inicio, NULL);
        ajustarBrilloConcurrente(imagen, 0); // delta=0 no modifica imagen
        gettimeofday(&fin, NULL);

        tiempos_brillo[i] = obtenerTiempoReal(inicio, fin);
        printf("    Tiempo: %.4f seg\n", tiempos_brillo[i]);
    }

    printf("\n");
    printf("═══════════════════════════════════════════════════════\n");
    printf("PRUEBA 2: CONVOLUCIÓN GAUSSIANA (operación compleja)\n");
    printf("═══════════════════════════════════════════════════════\n");
    printf("Kernel: 5x5, sigma: 1.5\n");

    for (int i = 0; i < 4; i++) {
        NUM_HILOS_GLOBAL = num_hilos[i];
        printf("\n[%d/%d] Ejecutando con %d hilo(s)...\n", i+1, 4, num_hilos[i]);

        struct timeval inicio, fin;
        gettimeofday(&inicio, NULL);

        // Ejecutar convolución
        if (!aplicarConvolucionGaussiana(imagen, 5, 1.5)) {
            printf("Error en convolución. Saltando...\n");
            tiempos_convolucion[i] = 0;
            continue;
        }

        gettimeofday(&fin, NULL);
        tiempos_convolucion[i] = obtenerTiempoReal(inicio, fin);
    }

    // TABLA DE RESULTADOS
    printf("\n\n");
    printf("╔══════════════════════════════════════════════════════╗\n");
    printf("║              RESULTADOS DEL BENCHMARK               ║\n");
    printf("╚══════════════════════════════════════════════════════╝\n");
    printf("\n");

    printf("┌────────┬─────────────┬─────────────┬──────────┬────────────┐\n");
    printf("│ Hilos  │   Brillo    │ Convolución │ Speedup  │ Eficiencia │\n");
    printf("│        │   (seg)     │    (seg)    │  (vs 1)  │    (%%)     │\n");
    printf("├────────┼─────────────┼─────────────┼──────────┼────────────┤\n");

    for (int i = 0; i < 4; i++) {
        double speedup_brillo = tiempos_brillo[0] / tiempos_brillo[i];
        double speedup_conv = tiempos_convolucion[0] / tiempos_convolucion[i];
        double speedup_promedio = (speedup_brillo + speedup_conv) / 2.0;
        double eficiencia = (speedup_promedio / num_hilos[i]) * 100.0;

        printf("│   %d    │   %7.4f   │   %7.4f   │  %5.2fx  │   %5.1f%%  │\n",
               num_hilos[i],
               tiempos_brillo[i],
               tiempos_convolucion[i],
               speedup_promedio,
               eficiencia);
    }

    printf("└────────┴─────────────┴─────────────┴──────────┴────────────┘\n");

    // INTERPRETACIÓN
    printf("\n📊 INTERPRETACIÓN:\n");

    double speedup_4hilos = ((tiempos_brillo[0] / tiempos_brillo[2]) +
                            (tiempos_convolucion[0] / tiempos_convolucion[2])) / 2.0;

    if (speedup_4hilos >= 3.0) {
        printf("  ✅ EXCELENTE: Speedup de %.2fx con 4 hilos\n", speedup_4hilos);
        printf("     El paralelismo es muy efectivo.\n");
    } else if (speedup_4hilos >= 2.0) {
        printf("  ✅ BUENO: Speedup de %.2fx con 4 hilos\n", speedup_4hilos);
        printf("     El paralelismo funciona bien.\n");
    } else if (speedup_4hilos >= 1.5) {
        printf("  ⚠️  MODERADO: Speedup de %.2fx con 4 hilos\n", speedup_4hilos);
        printf("     Hay mejora pero limitada (posible overhead o pocos cores).\n");
    } else {
        printf("  ⚠️  BAJO: Speedup de %.2fx con 4 hilos\n", speedup_4hilos);
        printf("     El overhead domina o la imagen es muy pequeña.\n");
    }

    printf("\n💡 NOTAS:\n");
    printf("  • Speedup ideal con 4 hilos: 4.0x (100%% eficiencia)\n");
    printf("  • Speedup real típico: 2.5x - 3.5x (60%%-85%% eficiencia)\n");
    printf("  • Factores que afectan: overhead, cache, memoria, CPU\n");

    // Restaurar configuración
    NUM_HILOS_GLOBAL = hilos_original;
    printf("\n✓ Configuración de hilos restaurada a: %d\n", NUM_HILOS_GLOBAL);
    printf("\n");
}

// QUÉ: Mostrar información del sistema y configuración actual.
// CÓMO: Imprime detalles de la imagen y configuración de hilos.
// POR QUÉ: Ayuda al usuario a entender el estado del programa.
void mostrarInformacion(const ImagenInfo* info) {
    printf("\n╔══════════════════════════════════════════════════════╗\n");
    printf("║           INFORMACIÓN DEL SISTEMA                   ║\n");
    printf("╚══════════════════════════════════════════════════════╝\n");

    // Información de configuración
    printf("\n[CONFIGURACIÓN]\n");
    printf("  • Hilos configurados: %d\n", NUM_HILOS_GLOBAL);
    printf("  • Rango válido: %d - %d hilos\n", MIN_HILOS, MAX_HILOS);

    // Información de imagen
    printf("\n[IMAGEN ACTUAL]\n");
    if (!info->pixeles) {
        printf("  ✗ No hay imagen cargada\n");
    } else {
        printf("  ✓ Imagen cargada:\n");
        printf("    - Dimensiones: %d x %d píxeles\n", info->ancho, info->alto);
        printf("    - Formato: %s (%d canal%s)\n",
               info->canales == 1 ? "Escala de grises" : "RGB",
               info->canales,
               info->canales == 1 ? "" : "es");
        printf("    - Tamaño total: %.2f MB\n",
               (info->ancho * info->alto * info->canales) / (1024.0 * 1024.0));
        printf("    - Píxeles totales: %d\n", info->ancho * info->alto);
    }

    // Recomendaciones
    printf("\n[RECOMENDACIONES]\n");
    if (info->pixeles) {
        int hilosRecomendados = info->alto / 100; // 1 hilo cada 100 filas
        if (hilosRecomendados < 2) hilosRecomendados = 2;
        if (hilosRecomendados > 8) hilosRecomendados = 8;
        printf("  • Para esta imagen, se recomiendan %d-%d hilos\n",
               hilosRecomendados, hilosRecomendados + 2);
    }
    printf("  • Blur leve: kernel=3, sigma=0.5\n");
    printf("  • Blur moderado: kernel=5, sigma=1.5\n");
    printf("  • Blur fuerte: kernel=9, sigma=3.0\n");
    printf("\n");
}
