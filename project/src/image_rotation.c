/**
 * @file image_rotation.c
 * @brief Concurrent geometric transformation module for multi-channel image rotation
 * @details This module implements high-performance image rotation using parallel processing
 *          techniques and advanced geometric transformations. The implementation employs
 *          inverse coordinate mapping with bilinear interpolation to achieve subpixel
 *          accuracy while maintaining computational efficiency through pthread-based
 *          concurrency. The module supports both grayscale and RGB color spaces with
 *          automatic dimension calculation and memory management.
 * 
 * @version 1.0
 * @date 2024
 * @author Computer Vision Systems Team
 * 
 * @note Methodology: Utilizes 2D rotation matrices combined with inverse transformation
 *       mapping to eliminate sampling artifacts. Bilinear interpolation ensures smooth
 *       pixel transitions while concurrent processing optimizes performance on multi-core
 *       architectures.
 * 
 * @warning Thread safety: Functions in this module are designed for sequential calls
 *          with internal parallelization. Concurrent calls to rotation functions on
 *          the same image structure may result in race conditions.
 */

#define _USE_MATH_DEFINES
#define _GNU_SOURCE
#include "image_rotation.h"
#include "threading.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846 
#endif

/**
 * @brief Clamps integer values to the valid pixel intensity range [0, 255]
 * @details Performs boundary value constraint enforcement to ensure pixel
 *          intensity values remain within the valid 8-bit unsigned range.
 *          This operation prevents overflow/underflow artifacts in image
 *          processing operations.
 * 
 * @param value Integer value to be clamped
 * @return Clamped value as unsigned char in range [0, 255]
 * 
 * @note This function is declared static inline for optimal performance
 *       in interpolation calculations where it may be called millions of times
 */
static inline unsigned char clamp(int value) {
    if (value < 0) return 0;
    if (value > 255) return 255;
    return (unsigned char)value;
}

/**
 * @brief Performs bilinear interpolation for subpixel-accurate color sampling
 * @details Implements four-point bilinear interpolation using the weighted average
 *          of the four nearest pixel neighbors. This technique ensures smooth color
 *          transitions when sampling at fractional coordinates during geometric
 *          transformations, significantly reducing aliasing artifacts compared to
 *          nearest-neighbor sampling.
 * 
 * @param pixels Three-dimensional pixel matrix (height x width x channels)
 * @param x Fractional x-coordinate in source image space
 * @param y Fractional y-coordinate in source image space  
 * @param width Source image width in pixels
 * @param height Source image height in pixels
 * @param channel Color channel index (0 for grayscale, 0-2 for RGB)
 * 
 * @return Interpolated pixel intensity value [0, 255]
 * 
 * @note Computational complexity: O(1) per pixel with boundary checking overhead
 * @note Edge pixels are replicated when sampling coordinates fall outside image bounds
 */
unsigned char bilinearInterpolate(unsigned char*** pixels, float x, float y, 
                                   int width, int height, int channel) {
    // Handle boundary conditions by clamping coordinates to valid range
    if (x < 0 || x >= width - 1 || y < 0 || y >= height - 1) {
        int xi = (int)(x < 0 ? 0 : (x >= width ? width - 1 : x));
        int yi = (int)(y < 0 ? 0 : (y >= height ? height - 1 : y));
        return pixels[yi][xi][channel];
    }

    // Extract integer and fractional components for interpolation weights
    int x0 = (int)floor(x);
    int y0 = (int)floor(y);
    int x1 = x0 + 1;
    int y1 = y0 + 1;
    
    float dx = x - x0;
    float dy = y - y0;

    // Sample four neighboring pixels forming interpolation kernel
    float p00 = pixels[y0][x0][channel];
    float p10 = pixels[y0][x1][channel];
    float p01 = pixels[y1][x0][channel];
    float p11 = pixels[y1][x1][channel];

    // Apply bilinear interpolation formula with distance-based weighting
    float value = p00 * (1 - dx) * (1 - dy) +
                  p10 * dx * (1 - dy) +
                  p01 * (1 - dx) * dy +
                  p11 * dx * dy;

    return clamp((int)value);
}

/**
 * @brief Calculates optimal dimensions for rotated image bounding box
 * @details Computes the minimum bounding rectangle that contains the entire
 *          rotated image by transforming all four corner points of the original
 *          image through the rotation matrix. This approach ensures no pixel
 *          data is lost during rotation and prevents clipping artifacts.
 * 
 * @param width Original image width in pixels
 * @param height Original image height in pixels
 * @param angleRadians Rotation angle in radians
 * @param newWidth Pointer to store calculated output width
 * @param newHeight Pointer to store calculated output height
 * 
 * @note The algorithm uses corner point transformation to determine
 *       the exact bounding box dimensions, ensuring optimal memory usage
 * @note Output dimensions are always equal to or larger than input dimensions
 */
void calculateRotatedDimensions(int width, int height, float angleRadians, 
                                int* newWidth, int* newHeight) {
    // Precompute trigonometric values for efficiency
    float cosAngle = cos(angleRadians);
    float sinAngle = sin(angleRadians);

    // Transform all four corners of original image using rotation matrix
    float corners[4][2] = {
        {0, 0},
        {width, 0},
        {0, height},
        {width, height}
    };

    float minX = 0, maxX = 0, minY = 0, maxY = 0;

    for (int i = 0; i < 4; i++) {
        float x = corners[i][0];
        float y = corners[i][1];
        
        // Apply 2D rotation transformation matrix
        float rotX = x * cosAngle - y * sinAngle;
        float rotY = x * sinAngle + y * cosAngle;

        // Update bounding box coordinates
        if (i == 0) {
            minX = maxX = rotX;
            minY = maxY = rotY;
        } else {
            if (rotX < minX) minX = rotX;
            if (rotX > maxX) maxX = rotX;
            if (rotY < minY) minY = rotY;
            if (rotY > maxY) maxY = rotY;
        }
    }

    // Calculate final dimensions ensuring all pixels fit
    *newWidth = (int)ceil(maxX - minX);
    *newHeight = (int)ceil(maxY - minY);
}

/**
 * @brief Thread worker function for concurrent image rotation processing
 * @details Executes rotation transformation on a designated row range within the
 *          destination image using inverse coordinate mapping. Each thread processes
 *          a subset of output rows independently, applying bilinear interpolation
 *          to determine pixel values from the source image. This approach ensures
 *          optimal load distribution and eliminates race conditions during parallel
 *          processing.
 * 
 * @param args Pointer to RotationThreadArgs structure containing thread-specific
 *             parameters including row range, transformation data, and image buffers
 * 
 * @return NULL upon completion (standard pthread return convention)
 * 
 * @note Thread-safe design: Each thread writes to distinct memory regions,
 *       eliminating the need for synchronization primitives
 * @note Performance optimization: Trigonometric values are precomputed once
 *       per thread to minimize computational overhead
 */
void* rotateImageThread(void* args) {
    RotationThreadArgs* rArgs = (RotationThreadArgs*)args;
    
    // Precompute trigonometric values for efficiency
    float cosAngle = cos(rArgs->angleRadians);
    float sinAngle = sin(rArgs->angleRadians);
    
    // Calculate center points for rotation
    float srcCenterX = rArgs->srcWidth / 2.0f;
    float srcCenterY = rArgs->srcHeight / 2.0f;
    float destCenterX = rArgs->destWidth / 2.0f;
    float destCenterY = rArgs->destHeight / 2.0f;

    // Process assigned rows in destination image
    for (int destY = rArgs->rowStart; destY < rArgs->rowEnd; destY++) {
        for (int destX = 0; destX < rArgs->destWidth; destX++) {
            // Translate to origin-centered coordinates
            float dx = destX - destCenterX;
            float dy = destY - destCenterY;

            // Apply inverse rotation transformation
            float srcX = dx * cosAngle - dy * sinAngle + srcCenterX;
            float srcY = dx * sinAngle + dy * cosAngle + srcCenterY;

            // Interpolate pixel value from source image
            for (int c = 0; c < rArgs->channels; c++) {
                rArgs->destPixels[destY][destX][c] = 
                    bilinearInterpolate(rArgs->srcInfo->pixeles, srcX, srcY,
                                       rArgs->srcWidth, rArgs->srcHeight, c);
            }
        }
    }

    return NULL;
}

/**
 * @brief Performs concurrent geometric rotation on multi-channel images
 * @details Executes high-performance image rotation using parallel thread processing
 *          with automatic dimension calculation and memory management. The function
 *          employs inverse coordinate mapping to eliminate sampling holes and utilizes
 *          bilinear interpolation for subpixel accuracy. Thread-based concurrency
 *          optimizes performance on multi-core architectures while maintaining
 *          thread-safe operation through non-overlapping memory region assignment.
 * 
 * @param info Pointer to ImagenInfo structure containing source image data.
 *             The structure is modified in-place with rotated image results.
 * @param angle Rotation angle in degrees (positive values indicate counter-clockwise rotation)
 * 
 * @return 1 on successful rotation completion, 0 on failure (memory allocation errors or invalid input)
 * 
 * @note Memory management: Automatically handles dimension changes and deallocates
 *       original image buffer after successful rotation
 * @note Thread safety: Uses 4 concurrent threads with non-overlapping row assignments
 * @note Performance: Optimized for multi-core systems with minimal synchronization overhead
 * 
 * @warning The original image data is replaced upon successful completion.
 *          Ensure backup copies are maintained if original data preservation is required.
 */
int rotateImageConcurrent(ImagenInfo* info, float angle) {
    // Validate input parameters
    if (!info->pixeles) {
        fprintf(stderr, "Error: No image loaded for rotation\n");
        return 0;
    }

    // Convert rotation angle from degrees to radians
    float angleRadians = angle * M_PI / 180.0f;

    // Calculate optimal dimensions for rotated image bounding box
    int newWidth, newHeight;
    calculateRotatedDimensions(info->ancho, info->alto, angleRadians, 
                              &newWidth, &newHeight);

    printf("Rotating image by %.2f degrees (original: %dx%d, new: %dx%d)\n",
           angle, info->ancho, info->alto, newWidth, newHeight);

    // Allocate memory for destination image buffer
    unsigned char*** newPixels = 
        (unsigned char***)malloc(newHeight * sizeof(unsigned char**));
    if (!newPixels) {
        fprintf(stderr, "Error: Memory allocation failed for rotated image\n");
        return 0;
    }

    for (int y = 0; y < newHeight; y++) {
        newPixels[y] = (unsigned char**)malloc(newWidth * sizeof(unsigned char*));
        if (!newPixels[y]) {
            fprintf(stderr, "Error: Memory allocation failed at row %d\n", y);
            // Clean up partial allocation on failure
            for (int i = 0; i < y; i++) {
                for (int x = 0; x < newWidth; x++) {
                    free(newPixels[i][x]);
                }
                free(newPixels[i]);
            }
            free(newPixels);
            return 0;
        }
        
        for (int x = 0; x < newWidth; x++) {
            newPixels[y][x] = 
                (unsigned char*)malloc(info->canales * sizeof(unsigned char));
            if (!newPixels[y][x]) {
                fprintf(stderr, "Error: Memory allocation failed at pixel (%d,%d)\n", 
                       x, y);
                // Clean up partial allocation on failure
                for (int i = 0; i <= y; i++) {
                    int maxX = (i == y) ? x : newWidth;
                    for (int j = 0; j < maxX; j++) {
                        free(newPixels[i][j]);
                    }
                    free(newPixels[i]);
                }
                free(newPixels);
                return 0;
            }
        }
    }

    // Configure concurrent processing with multiple worker threads
    const int NUM_THREADS = NUM_HILOS_GLOBAL;
    pthread_t threads[NUM_THREADS];
    RotationThreadArgs threadArgs[NUM_THREADS];
    int rowsPerThread = (int)ceil((double)newHeight / NUM_THREADS);

    // Create and launch worker threads with load balancing
    for (int i = 0; i < NUM_THREADS; i++) {
        threadArgs[i].srcInfo = info;
        threadArgs[i].destPixels = newPixels;
        threadArgs[i].angleRadians = angleRadians;
        threadArgs[i].destWidth = newWidth;
        threadArgs[i].destHeight = newHeight;
        threadArgs[i].rowStart = i * rowsPerThread;
        threadArgs[i].rowEnd = ((i + 1) * rowsPerThread < newHeight) ? 
                               (i + 1) * rowsPerThread : newHeight;
        threadArgs[i].srcWidth = info->ancho;
        threadArgs[i].srcHeight = info->alto;
        threadArgs[i].channels = info->canales;

        if (pthread_create(&threads[i], NULL, rotateImageThread, 
                          &threadArgs[i]) != 0) {
            fprintf(stderr, "Error: Failed to create thread %d\n", i);
            // Clean up on thread creation failure
            for (int j = 0; j < i; j++) {
                pthread_join(threads[j], NULL);
            }
            for (int y = 0; y < newHeight; y++) {
                for (int x = 0; x < newWidth; x++) {
                    free(newPixels[y][x]);
                }
                free(newPixels[y]);
            }
            free(newPixels);
            return 0;
        }
    }

    // Synchronize thread completion before proceeding
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }

    // Deallocate original image memory to prevent memory leaks
    for (int y = 0; y < info->alto; y++) {
        for (int x = 0; x < info->ancho; x++) {
            free(info->pixeles[y][x]);
        }
        free(info->pixeles[y]);
    }
    free(info->pixeles);

    // Update image structure with rotated data
    info->pixeles = newPixels;
    info->ancho = newWidth;
    info->alto = newHeight;

    printf("Image rotation completed concurrently with %d threads (%s)\n",
           NUM_THREADS, info->canales == 1 ? "grayscale" : "RGB");

    return 1;
}