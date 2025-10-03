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
// Compilar: gcc -o img img_base.c -pthread -lm
// Ejecutar: ./img [ruta_imagen.png]

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <math.h>
#include <time.h> // para medir eficiencia
#include <sys/time.h>
#include <unistd.h>

// QUÉ: Variable global para número de hilos configurable.
// CÓMO: Se modifica desde el menú, se usa en todas las funciones paralelas.
// POR QUÉ: Permite demostrar escalabilidad y cumple con requisito de configurabilidad.
int NUM_HILOS_GLOBAL = 4; // Valor por defecto: 4 hilos

// QUÉ: Límites para número de hilos.
// CÓMO: Constantes que definen rango válido.
// POR QUÉ: Evita valores absurdos (muy pocos o demasiados hilos).
#define MIN_HILOS 1
#define MAX_HILOS 16

// QUÉ: Incluir bibliotecas stb para cargar y guardar imágenes PNG.
// CÓMO: stb_image.h lee PNG/JPG a memoria; stb_image_write.h escribe PNG.
// POR QUÉ: Son bibliotecas de un solo archivo, simples y sin dependencias externas.
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

// QUÉ: Estructura para almacenar la imagen (ancho, alto, canales, píxeles).
// CÓMO: Usa matriz 3D para píxeles (alto x ancho x canales), donde canales es
// 1 (grises) o 3 (RGB). Píxeles son unsigned char (0-255).
// POR QUÉ: Permite manejar tanto grises como color, con memoria dinámica para
// flexibilidad y evitar desperdicio.
typedef struct {
    int ancho;           // Ancho de la imagen en píxeles
    int alto;            // Alto de la imagen en píxeles
    int canales;         // 1 (escala de grises) o 3 (RGB)
    unsigned char*** pixeles; // Matriz 3D: [alto][ancho][canales]
} ImagenInfo;

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

// QUÉ: Calcular tiempo real transcurrido en segundos.
// CÓMO: Usa gettimeofday (tiempo de reloj de pared, no CPU time).
// POR QUÉ: clock() suma tiempo de todos los hilos, no muestra paralelización real.
double obtenerTiempoReal(struct timeval inicio, struct timeval fin) {
    long segundos = fin.tv_sec - inicio.tv_sec;
    long microsegundos = fin.tv_usec - inicio.tv_usec;
    return segundos + microsegundos / 1000000.0;
}

// QUÉ: Cargar una imagen PNG desde un archivo.
// CÓMO: Usa stbi_load para leer el archivo, detecta canales (1 o 3), y convierte
// los datos a una matriz 3D (alto x ancho x canales).
// POR QUÉ: La matriz 3D es intuitiva para principiantes y permite procesar
// píxeles y canales individualmente.
int cargarImagen(const char* ruta, ImagenInfo* info) {
    int canales;
    // QUÉ: Cargar imagen con formato original (0 canales = usar formato nativo).
    // CÓMO: stbi_load lee el archivo y llena ancho, alto y canales.
    // POR QUÉ: Respetar el formato original asegura que grises o RGB se mantengan.
    unsigned char* datos = stbi_load(ruta, &info->ancho, &info->alto, &canales, 0);
    if (!datos) {
        fprintf(stderr, "Error al cargar imagen: %s\n", ruta);
        return 0;
    }
    
    // QUÉ: Validar dimensiones de la imagen.
    // CÓMO: Verifica que ancho y alto sean positivos y razonables.
    // POR QUÉ: Evita problemas con imágenes corruptas o inválidas.
    if (info->ancho <= 0 || info->alto <= 0) {
        fprintf(stderr, "ERROR: Dimensiones inválidas (%dx%d)\n", info->ancho, info->alto);
        stbi_image_free(datos);
        return 0;
    }
    if (info->ancho > 10000 || info->alto > 10000) {
        fprintf(stderr, "ADVERTENCIA: Imagen muy grande (%dx%d)\n", info->ancho, info->alto);
        fprintf(stderr, "El procesamiento puede ser lento. ¿Continuar? (s/n): ");
        char respuesta;
        scanf(" %c", &respuesta);
        while (getchar() != '\n');
        if (respuesta != 's' && respuesta != 'S') {
            stbi_image_free(datos);
            return 0;
        }
    }
    
    info->canales = (canales == 1 || canales == 3) ? canales : 1; // Forzar 1 o 3

    // QUÉ: Asignar memoria para matriz 3D.
    // CÓMO: Asignar alto filas, luego ancho columnas por fila, luego canales por píxel.
    // POR QUÉ: Estructura clara y flexible para grises (1 canal) o RGB (3 canales).
    info->pixeles = (unsigned char***)malloc(info->alto * sizeof(unsigned char**));
    if (!info->pixeles) {
        fprintf(stderr, "Error de memoria al asignar filas\n");
        stbi_image_free(datos);
        return 0;
    }
    for (int y = 0; y < info->alto; y++) {
        info->pixeles[y] = (unsigned char**)malloc(info->ancho * sizeof(unsigned char*));
        if (!info->pixeles[y]) {
            fprintf(stderr, "Error de memoria al asignar columnas\n");
            liberarImagen(info);
            stbi_image_free(datos);
            return 0;
        }
        for (int x = 0; x < info->ancho; x++) {
            info->pixeles[y][x] = (unsigned char*)malloc(info->canales * sizeof(unsigned char));
            if (!info->pixeles[y][x]) {
                fprintf(stderr, "Error de memoria al asignar canales\n");
                liberarImagen(info);
                stbi_image_free(datos);
                return 0;
            }
            // Copiar píxeles a matriz 3D
            for (int c = 0; c < info->canales; c++) {
                info->pixeles[y][x][c] = datos[(y * info->ancho + x) * info->canales + c];
            }
        }
    }

    stbi_image_free(datos); // Liberar buffer de stb
    printf("Imagen cargada: %dx%d, %d canales (%s)\n", info->ancho, info->alto,
           info->canales, info->canales == 1 ? "grises" : "RGB");
    return 1;
}

// QUÉ: Mostrar la matriz de píxeles (primeras 10 filas).
// CÓMO: Imprime los valores de los píxeles, agrupando canales por píxel (grises o RGB).
// POR QUÉ: Ayuda a visualizar la matriz para entender la estructura de datos.
void mostrarMatriz(const ImagenInfo* info) {
    if (!info->pixeles) {
        printf("No hay imagen cargada.\n");
        return;
    }
    printf("Matriz de la imagen (primeras 10 filas):\n");
    for (int y = 0; y < info->alto && y < 10; y++) {
        for (int x = 0; x < info->ancho; x++) {
            if (info->canales == 1) {
                printf("%3u ", info->pixeles[y][x][0]); // Escala de grises
            } else {
                printf("(%3u,%3u,%3u) ", info->pixeles[y][x][0], info->pixeles[y][x][1],
                       info->pixeles[y][x][2]); // RGB
            }
        }
        printf("\n");
    }
    if (info->alto > 10) {
        printf("... (más filas)\n");
    }
}

// QUÉ: Guardar la matriz como PNG (grises o RGB).
// CÓMO: Aplana la matriz 3D a 1D y usa stbi_write_png con el número de canales correcto.
// POR QUÉ: Respeta el formato original (grises o RGB) para consistencia.
int guardarPNG(const ImagenInfo* info, const char* rutaSalida) {
    if (!info->pixeles) {
        fprintf(stderr, "No hay imagen para guardar.\n");
        return 0;
    }

    // QUÉ: Aplanar matriz 3D a 1D para stb.
    // CÓMO: Copia píxeles en orden [y][x][c] a un arreglo plano.
    // POR QUÉ: stb_write_png requiere datos contiguos.
    unsigned char* datos1D = (unsigned char*)malloc(info->ancho * info->alto * info->canales);
    if (!datos1D) {
        fprintf(stderr, "Error de memoria al aplanar imagen\n");
        return 0;
    }
    for (int y = 0; y < info->alto; y++) {
        for (int x = 0; x < info->ancho; x++) {
            for (int c = 0; c < info->canales; c++) {
                datos1D[(y * info->ancho + x) * info->canales + c] = info->pixeles[y][x][c];
            }
        }
    }

    // QUÉ: Guardar como PNG.
    // CÓMO: Usa stbi_write_png con los canales de la imagen original.
    // POR QUÉ: Mantiene el formato (grises o RGB) de la entrada.
    int resultado = stbi_write_png(rutaSalida, info->ancho, info->alto, info->canales,
                                   datos1D, info->ancho * info->canales);
    free(datos1D);
    if (resultado) {
        printf("Imagen guardada en: %s (%s)\n", rutaSalida,
               info->canales == 1 ? "grises" : "RGB");
        return 1;
    } else {
        fprintf(stderr, "Error al guardar PNG: %s\n", rutaSalida);
        return 0;
    }
}

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

// QUÉ: Estructura para monitorear el progreso de cada hilo.
// CÓMO: Guarda ID, tiempo inicio/fin, filas procesadas.
// POR QUÉ: Permite visualizar trabajo de cada hilo individualmente.
typedef struct {
    int id;
    struct timeval inicio;
    struct timeval fin;
    int filas_procesadas;
    int pixeles_procesados;
} MonitorHilo;

// QUÉ: Ajustar brillo en un rango de filas (para hilos) con monitoreo.
// CÓMO: Suma delta a cada canal, registra inicio/fin y progreso.
// POR QUÉ: Permite visualizar el trabajo de cada hilo.
void* ajustarBrilloHilo(void* args) {
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
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// ============================================================================
// FUNCIÓN 1: CONVOLUCIÓN GAUSSIANA
// ============================================================================

// QUÉ: Generar kernel Gaussiano usando la fórmula matemática.
// CÓMO: Calcula G(x,y) = (1/(2πσ²))·exp(-(x²+y²)/(2σ²)) para cada posición.
// POR QUÉ: El kernel Gaussiano suaviza la imagen ponderando por distancia al centro.
float** generarKernelGaussiano(int tamKernel, float sigma, float* suma) {
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
int validarParametrosConvolucion(int tamKernel, float sigma) {
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
void liberarKernel(float** kernel, int tamKernel) {
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
void* aplicarConvolucionHilo(void* args) {
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

// ============================================================================
// FUNCIÓN 2: DETECCIÓN DE BORDES CON SOBEL
// ============================================================================

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
void* calcularSobelHilo(void* args) {
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





////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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
                
                printf("\n");
                printf("╔══════════════════════════════════════════════════════╗\n");
                printf("║          BENCHMARK DE PARALELIZACIÓN                ║\n");
                printf("╚══════════════════════════════════════════════════════╝\n");
                printf("\n");
                printf("Este benchmark ejecutará operaciones con diferentes\n");
                printf("números de hilos para demostrar la mejora de rendimiento.\n");
                printf("\n");
                printf("Imagen: %dx%d píxeles (%d total)\n", 
                       imagen.ancho, imagen.alto, imagen.ancho * imagen.alto);
                printf("\n¿Continuar? (s/n): ");
                
                char respuesta;
                scanf(" %c", &respuesta);
                while (getchar() != '\n');
                
                if (respuesta != 's' && respuesta != 'S') {
                    printf("Benchmark cancelado.\n");
                    break;
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
                    ajustarBrilloConcurrente(&imagen, 0); // delta=0 no modifica imagen
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
                    if (!aplicarConvolucionGaussiana(&imagen, 5, 1.5)) {
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
                snprintf(rutaCompleta, sizeof(rutaCompleta), "../imgs/results/%s", nombreArchivo);
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