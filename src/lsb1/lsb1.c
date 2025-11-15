#include "lsb1.h"
#include "../common/bmp_image.h"
#include <stdio.h>
#include <string.h>

int lsb1_embed(BMPImage *bmp, const uint8_t *data, size_t num_bits, size_t *offset) {
    if (bmp == NULL || bmp->data == NULL || data == NULL || offset == NULL) {
        return -1;
    }

    size_t component_index = *offset;
    size_t bit_index = 0;
    size_t max_component_index = bmp->width * bmp->height * 3;

    for (; bit_index < num_bits && component_index < max_component_index; bit_index++) {
        Component comp = get_component_by_index(bmp, component_index++);
        if (comp.component_ptr == NULL) {
            return -1;
        }

        uint8_t byte = data[bit_index / 8];
        size_t bit_position = 7 - (bit_index % 8);
        uint8_t bit = (byte >> bit_position) & 0x01;

        *comp.component_ptr = (*comp.component_ptr & 0xFE) | bit;
    }

    if (bit_index != num_bits) {
        return -1;
    }

    *offset = component_index;
    return 0;
}

int lsb1_extract(const BMPImage *bmp, size_t num_bits, uint8_t *buffer, size_t *offset) {
    if (bmp == NULL || bmp->data == NULL || buffer == NULL || offset == NULL) {
        return -1;
    }

    memset(buffer, 0, (num_bits + 7) / 8);

    size_t component_index = *offset;
    size_t bit_index = 0;
    size_t max_component_index = bmp->width * bmp->height * 3;

    for (; bit_index < num_bits && component_index < max_component_index; component_index++) {
        Component comp = get_component_by_index(bmp, component_index);
        if (comp.component_ptr == NULL) {
            return -1;
        }

        uint8_t bit = *comp.component_ptr & 0x01;
        size_t bit_position = 7 - (bit_index % 8);
        buffer[bit_index / 8] |= (bit << bit_position);
        bit_index++;
    }

    if (bit_index != num_bits) {
        return -1;
    }

    *offset = component_index;
    return 0;
}
