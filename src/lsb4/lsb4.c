#include "lsb4.h"
#include "../common/bmp_image.h"
#include <stdio.h>
#include <string.h>

int lsb4_embed(BMPImage *bmp, const uint8_t *data, size_t num_bits, size_t *offset) {
    if (bmp == NULL || bmp->data == NULL || data == NULL || offset == NULL) {
        return -1;
    }

    if (num_bits % 4 != 0) {
        return -1;
    }

    size_t component_index = *offset;
    size_t bit_index = 0;
    size_t max_component_index = bmp->width * bmp->height * 3;

    for (; bit_index < num_bits && component_index < max_component_index; bit_index += 4) {
        uint8_t byte = data[bit_index / 8];
        size_t start_bit_position = 7 - (bit_index % 8);
        uint8_t bits_value = 0;

        // Extraemos los 4 bits
        for (int bit = 0; bit < 4; bit++) {
            size_t current_bit_position = start_bit_position - bit;
            bits_value |= ((byte >> current_bit_position) & 0x01) << (3 - bit);
        }

        Component comp = get_component_by_index(bmp, component_index++);
        if (comp.component_ptr == NULL) {
            return -1;
        }

        // Colocar los 4 bits en los menos significativos del componente
        *comp.component_ptr = (*comp.component_ptr & 0xF0) | bits_value;
    }

    if (bit_index != num_bits) {
        return -1;
    }

    *offset = component_index;
    return 0;
}

int lsb4_extract(const BMPImage *bmp, size_t num_bits, uint8_t *buffer, size_t *offset) {
    if (bmp == NULL || bmp->data == NULL || buffer == NULL || offset == NULL) {
        return -1;
    }

    if (num_bits % 4 != 0) {
        return -1;
    }

    memset(buffer, 0, (num_bits + 7) / 8);

    size_t component_index = *offset;
    size_t bit_index = 0;
    size_t max_component_index = bmp->width * bmp->height * 3;

    for (; bit_index < num_bits && component_index < max_component_index; bit_index += 4) {
        Component comp = get_component_by_index(bmp, component_index++);
        if (comp.component_ptr == NULL) {
            return -1;
        }

        uint8_t extracted_bits = *comp.component_ptr & 0x0F;

        // Almacenar los 4 bits extraídos en el buffer
        for (int bit = 3; bit >= 0; bit--) {
            uint8_t bit_value = (extracted_bits >> bit) & 0x01;
            size_t bit_position = 7 - ((bit_index + (3 - bit)) % 8);
            buffer[(bit_index + (3 - bit)) / 8] |= (bit_value << bit_position);
        }
    }

    if (bit_index != num_bits) {
        return -1;
    }

    *offset = component_index;
    return 0;
}
