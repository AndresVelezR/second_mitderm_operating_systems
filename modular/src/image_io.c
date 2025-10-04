#include "image_io.h"
#include "image.h"
#include <stdio.h>
#include <stdlib.h>

// QUÉ: Incluir bibliotecas stb para cargar y guardar imágenes PNG.
// CÓMO: stb_image.h lee PNG/JPG a memoria; stb_image_write.h escribe PNG.
// POR QUÉ: Son bibliotecas de un solo archivo, simples y sin dependencias externas.
#define STB_IMAGE_IMPLEMENTATION
#include "../stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../stb/stb_image_write.h"

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
