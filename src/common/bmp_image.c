#include "bmp_image.h"
#include <stdio.h>
#include <string.h>

Component get_component_by_index(const BMPImage *bmp, size_t index) {
    Component result = {NULL, INVALID_COLOR};

    if (bmp == NULL || bmp->data == NULL) {
        return result;
    }

    size_t row_size = (bmp->width * 3 + 3) & ~3;  // Tamaño de la fila con padding
    size_t total_components = bmp->width * bmp->height * 3;

    if (index >= total_components) {
        return result;
    }

    size_t pixel_row = index / (bmp->width * 3);        // Fila del componente
    size_t offset_in_row = index % (bmp->width * 3);    // Desplazamiento en la fila

    // Obtener el puntero al componente en los datos de BMP
    result.component_ptr = &bmp->data[pixel_row * row_size + offset_in_row];

    // Determinar el tipo de color basándonos en la posición relativa (offset en el píxel)
    result.color = offset_in_row % 3;

    return result;
}

