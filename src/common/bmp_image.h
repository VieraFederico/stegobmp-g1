#ifndef BMP_IMAGE_H
#define BMP_IMAGE_H

#include <stddef.h>
#include <stdint.h>

#define BMP_HEADER_SIZE 54

/**
 * @brief Structure to hold BMP image data, including header and pixel data.
 */
typedef struct {
    unsigned char header[BMP_HEADER_SIZE];  // BMP header (54 bytes for V3 format)
    unsigned char *data;                    // Pointer to the pixel data
    size_t data_size;                       // Size of the pixel data in bytes and padding
    size_t width;                           // Width of the image in pixels
    size_t height;                          // Height of the image in pixels
} BMPImage;

typedef enum {
    BLUE = 0,
    GREEN = 1,
    RED = 2,
    INVALID_COLOR
} ColorType;

typedef struct {
    uint8_t *component_ptr; // Puntero al componente de color
    ColorType color;        // Tipo de color (BLUE, GREEN, RED)
} Component;

/**
 * @brief Obtiene un puntero al componente de color (byte) en un BMP según el índice de componente global.
 *
 * @param bmp        Puntero a la estructura BMPImage.
 * @param index      Índice del componente de color (byte) a acceder.
 * @return Component Puntero al componente y su color correspondiente (modificable).
 */
Component get_component_by_index(const BMPImage *bmp, size_t index);

#endif // BMP_IMAGE_H

