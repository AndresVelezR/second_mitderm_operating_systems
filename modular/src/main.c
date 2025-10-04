// Programa de procesamiento de imágenes en C para principiantes en Linux.
// QUÉ: Procesa imágenes PNG (escala de grises o RGB) usando matrices, con soporte
// para carga, visualización, guardado y ajuste de brillo concurrente.
// CÓMO: Usa stb_image.h para cargar PNG y stb_image_write.h para guardar PNG,
// con hilos POSIX (pthread) para el procesamiento paralelo del brillo.
// POR QUÉ: Diseñado para enseñar manejo de matrices, concurrencia y gestión de
// memoria en C, manteniendo simplicidad y robustez para principiantes.
// Dependencias: Descarga stb_image.h y stb_image_write.h desde
// https://github.com/nothings/stb
//   wget https://raw.githubusercontent.com/nothings/stb/master/stb_image.h
//   wget https://raw.githubusercontent.com/nothings/stb/master/stb_image_write.h
//
// Compilar: make
// Ejecutar: ./img [ruta_imagen.png]

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "image.h"
#include "image_io.h"
#include "filters.h"
#include "threading.h"
#include "benchmark.h"

// QUÉ: Mostrar el menú interactivo.
// CÓMO: Imprime opciones y espera entrada del usuario.
// POR QUÉ: Proporciona una interfaz simple para interactuar con el programa.
void mostrarMenu() {
    printf("\n╔══════════════════════════════════════════════════════╗\n");
    printf("║   Plataforma de Edición de Imágenes - Linux C      ║\n");
    printf("╚══════════════════════════════════════════════════════╝\n");
    printf("  0. Benchmark de paralelización (prueba automática)\n");
    printf("  1. Cargar imagen PNG\n");
    printf("  2. Mostrar matriz de píxeles\n");
    printf("  3. Guardar como PNG\n");
    printf("  4. Ajustar brillo (+/- valor) concurrentemente\n");
    printf("  5. Aplicar convolución Gaussiana (blur)\n");
    printf("  6. Aplicar detector de bordes Sobel\n");
    printf("  7. Configurar número de hilos (actual: %d)\n", NUM_HILOS_GLOBAL);
    printf("  8. Información del sistema\n");
    printf("  9. Salir\n");
    printf("─────────────────────────────────────────────────────\n");
    printf("Opción: ");
}

// QUÉ: Función principal que controla el flujo del programa.
// CÓMO: Maneja entrada CLI, ejecuta el menú en bucle y llama funciones según opción.
// POR QUÉ: Centraliza la lógica y asegura limpieza al salir.
int main(int argc, char* argv[]) {
    ImagenInfo imagen = {0, 0, 0, NULL}; // Inicializar estructura
    char ruta[256] = {0}; // Buffer para ruta de archivo

    // QUÉ: Cargar imagen desde CLI si se pasa.
    // CÓMO: Copia argv[1] y llama cargarImagen.
    // POR QUÉ: Permite ejecución directa con ./img imagen.png.
    if (argc > 1) {
        strncpy(ruta, argv[1], sizeof(ruta) - 1);
        if (!cargarImagen(ruta, &imagen)) {
            return EXIT_FAILURE;
        }
    }

    int opcion;
    while (1) {
        mostrarMenu();
        // QUÉ: Leer opción del usuario.
        // CÓMO: Usa scanf y limpia el buffer para evitar bucles infinitos.
        // POR QUÉ: Manejo robusto de entrada evita errores comunes.
        if (scanf("%d", &opcion) != 1) {
            while (getchar() != '\n');
            printf("Entrada inválida.\n");
            continue;
        }
        while (getchar() != '\n'); // Limpiar buffer

        switch (opcion) {
            case 0: { // Benchmark automático
                if (!imagenCargada(&imagen)) {
                    printf("\n❌ Debes cargar una imagen primero (opción 1).\n");
                    break;
                }
                ejecutarBenchmark(&imagen);
                break;
            }
            case 1: { // Cargar imagen
                printf("Ingresa la ruta del archivo PNG: ");
                if (fgets(ruta, sizeof(ruta), stdin) == NULL) {
                    printf("Error al leer ruta.\n");
                    continue;
                }
                ruta[strcspn(ruta, "\n")] = 0; // Eliminar salto de línea
                liberarImagen(&imagen); // Liberar imagen previa
                if (!cargarImagen(ruta, &imagen)) {
                    continue;
                }
                break;
            }
            case 2: // Mostrar matriz
                mostrarMatriz(&imagen);
                break;
            case 3: { // Guardar PNG
                char nombreArchivo[256];
                char rutaCompleta[512];
                printf("Nombre del archivo PNG de salida: ");
                if (fgets(nombreArchivo, sizeof(nombreArchivo), stdin) == NULL) {
                    printf("Error al leer ruta.\n");
                    continue;
                }
                nombreArchivo[strcspn(nombreArchivo, "\n")] = 0;
                snprintf(rutaCompleta, sizeof(rutaCompleta), "results/%s", nombreArchivo);
                guardarPNG(&imagen, rutaCompleta);
                break;
            }
            case 4: { // Ajustar brillo
                int delta;
                printf("Valor de ajuste de brillo (+ para más claro, - para más oscuro): ");
                if (scanf("%d", &delta) != 1) {
                    while (getchar() != '\n');
                    printf("Entrada inválida.\n");
                    continue;
                }
                while (getchar() != '\n');
                ajustarBrilloConcurrente(&imagen, delta);
                break;
            }
            case 5: { // Convolución Gaussiana
                int tamKernel;
                float sigma;
                printf("Tamaño del kernel (impar, 3-15): ");
                if (scanf("%d", &tamKernel) != 1) {
                    while (getchar() != '\n');
                    printf("Entrada inválida.\n");
                    continue;
                }
                printf("Valor de sigma (e.g., 1.0): ");
                if (scanf("%f", &sigma) != 1) {
                    while (getchar() != '\n');
                    printf("Entrada inválida.\n");
                    continue;
                }
                while (getchar() != '\n');
                aplicarConvolucionGaussiana(&imagen, tamKernel, sigma);
                break;
            }
            case 6: { // Sobel
                aplicarSobel(&imagen);
                break;
            }
            case 7: { // Configurar hilos
                int nuevoNumHilos;
                printf("Número actual de hilos: %d\n", NUM_HILOS_GLOBAL);
                printf("Ingresa nuevo número de hilos (%d-%d): ", MIN_HILOS, MAX_HILOS);
                if (scanf("%d", &nuevoNumHilos) != 1) {
                    while (getchar() != '\n');
                    printf("Entrada inválida.\n");
                    continue;
                }
                while (getchar() != '\n');

                if (nuevoNumHilos < MIN_HILOS || nuevoNumHilos > MAX_HILOS) {
                    fprintf(stderr, "ERROR: Número de hilos debe estar entre %d y %d\n",
                            MIN_HILOS, MAX_HILOS);
                    continue;
                }

                NUM_HILOS_GLOBAL = nuevoNumHilos;
                printf("✓ Número de hilos configurado a: %d\n", NUM_HILOS_GLOBAL);
                printf("INFO: Este cambio afectará todas las operaciones futuras.\n");
                break;
            }
            case 8: { // Información del sistema
                mostrarInformacion(&imagen);
                break;
            }
            case 9: {// Salir (antes era case 7)
                liberarImagen(&imagen);
                printf("¡Adiós!\n");
                return EXIT_SUCCESS;
            }
            default:
                printf("Opción inválida.\n");
        }
    }
    liberarImagen(&imagen);
    return EXIT_SUCCESS;
}
