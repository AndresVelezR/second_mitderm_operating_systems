/**
 * @file image_rotation.h
 * @brief Interface for concurrent image rotation functionality.
 *
 * @details
 * This header defines the data structures and function prototypes required to perform
 * concurrent image rotation using POSIX threads (pthreads). The implementation employs
 * transformation matrices and bilinear interpolation to achieve high-quality results,
 * while leveraging multi-core parallelism to improve performance.
 *
 * @note
 * Designed for both grayscale (1-channel) and RGB (3-channel) images. All image data
 * is represented as dynamically allocated 3D arrays of unsigned char.
 *
 */

#ifndef IMAGE_ROTATION_H
#define IMAGE_ROTATION_H

#include <pthread.h>
#include "image.h"

/**
 * @struct RotationThreadArgs
 * @brief Encapsulates the parameters required by each rotation thread.
 *
 * @details
 * Each thread operates on a specific row range of the output image. This structure
 * provides all necessary context, including references to source and destination
 * buffers, image dimensions, rotation angle, and color channel information.
 *
 * @note
 * The rotation angle is expressed in radians.
 */
typedef struct {
    ImagenInfo* srcInfo;           /**< Pointer to the source image structure. */
    unsigned char*** destPixels;   /**< Destination pixel matrix (output image). */
    float angleRadians;            /**< Rotation angle in radians. */
    int destWidth;                 /**< Width of the destination image. */
    int destHeight;                /**< Height of the destination image. */
    int rowStart;                  /**< Starting row index for this thread. */
    int rowEnd;                    /**< Ending row index (exclusive) for this thread. */
    int srcWidth;                  /**< Width of the source image. */
    int srcHeight;                 /**< Height of the source image. */
    int channels;                  /**< Number of channels (1 for grayscale, 3 for RGB). */
} RotationThreadArgs;

/**
 * @brief Rotates an image by a specified angle using concurrent processing.
 *
 * @details
 * This function performs a geometric rotation on an image, distributing the computation
 * across multiple threads to exploit multi-core parallelism. The rotation is applied
 * counter-clockwise for positive angles and clockwise for negative angles. Bilinear
 * interpolation ensures smooth pixel transitions for non-integer coordinate mappings.
 *
 * @param[in,out] info Pointer to an ImagenInfo structure containing the image to rotate.
 *                     The structure is updated in place with the rotated result.
 * @param[in] angle Rotation angle in degrees. Positive values rotate counter-clockwise.
 *
 * @return
 * - `1` if the rotation is completed successfully.
 * - `0` if a memory allocation or processing error occurs.
 *
 * @note
 * The function automatically recalculates new image dimensions to fit the rotated image
 * without clipping. Works seamlessly with both grayscale and RGB images.
 */
int rotateImageConcurrent(ImagenInfo* info, float angle);

/**
 * @brief Thread worker function for performing partial image rotation.
 *
 * @details
 * Each thread executes this function to process a contiguous block of rows in the
 * destination image. The function maps each destination pixel to the corresponding
 * coordinates in the source image using the inverse rotation transformation, and
 * applies bilinear interpolation to estimate pixel values.
 *
 * @param[in] args Pointer to a RotationThreadArgs structure containing the thread context.
 *
 * @return Always returns `NULL` (to comply with pthreads' expected function signature).
 *
 * @warning
 * This function is intended to be invoked exclusively by `pthread_create()` and must
 * not be called directly by user code.
 */
void* rotateImageThread(void* args);

/**
 * @brief Performs bilinear interpolation for a given fractional coordinate.
 *
 * @details
 * Estimates the pixel intensity at non-integer coordinates by sampling the four nearest
 * neighboring pixels and computing a weighted average based on their relative distances.
 * Produces smooth gradients in rotated or rescaled images.
 *
 * @param[in] pixels 3D pixel matrix of the source image.
 * @param[in] x Fractional X-coordinate.
 * @param[in] y Fractional Y-coordinate.
 * @param[in] width Image width in pixels.
 * @param[in] height Image height in pixels.
 * @param[in] channel Channel index (0 for grayscale, 0–2 for RGB).
 *
 * @return Interpolated pixel intensity value in the range [0, 255].
 *
 * @note
 * Out-of-bounds coordinates are handled gracefully by clamping or mirroring edge pixels.
 */
unsigned char bilinearInterpolate(unsigned char*** pixels, float x, float y, 
                                   int width, int height, int channel);

/**
 * @brief Computes the bounding box dimensions for a rotated image.
 *
 * @details
 * Given an image of specified width and height and a rotation angle (in radians),
 * this function determines the minimal bounding rectangle that fully contains
 * the rotated image. It does so by transforming the corner coordinates using
 * the rotation matrix and computing their min/max extents.
 *
 * @param[in] width Original image width in pixels.
 * @param[in] height Original image height in pixels.
 * @param[in] angleRadians Rotation angle in radians.
 * @param[out] newWidth Pointer to an integer to store the resulting width.
 * @param[out] newHeight Pointer to an integer to store the resulting height.
 *
 * @note
 * Typically used internally by `rotateImageConcurrent` to allocate the
 * appropriately sized destination image buffer.
 */
void calculateRotatedDimensions(int width, int height, float angleRadians, 
                                int* newWidth, int* newHeight);

#endif /**< IMAGE_ROTATION_H */