#ifndef IMAGE_H
#define IMAGE_H

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
void liberarImagen(ImagenInfo* info);

// QUÉ: Verificar si hay una imagen cargada en memoria.
// CÓMO: Comprueba que el puntero de píxeles no sea NULL.
// POR QUÉ: Evita código repetitivo y centraliza la validación.
int imagenCargada(const ImagenInfo* info);

// QUÉ: Convertir imagen RGB a escala de grises.
// CÓMO: Usa ponderación perceptual (0.299R + 0.587G + 0.114B).
// POR QUÉ: Sobel requiere imagen de un solo canal para calcular gradientes.
int convertirAGrayscale(ImagenInfo* info);

#endif // IMAGE_H
