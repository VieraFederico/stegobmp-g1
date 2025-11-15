#include "lsbi.h"
#include "../common/bmp_image.h"
#include "../lsb1/lsb1.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int lsbi_embed(BMPImage *bmp, const uint8_t *data, size_t num_bits, size_t *offset) {
    if (bmp == NULL || bmp->data == NULL || data == NULL || offset == NULL) {
        return -1;
    }

    size_t component_index = *offset + PATTERN_MAP_SIZE; 
    size_t bit_to_embed_count = 0;
    size_t pattern_changed[PATTERN_MAP_SIZE] = {0};
    size_t pattern_unchanged[PATTERN_MAP_SIZE] = {0};
    size_t max_component_index = bmp->width * bmp->height * 3;

    if (component_index + num_bits > max_component_index) {
        return -1;
    }


    for (; component_index < max_component_index && bit_to_embed_count < num_bits; component_index++) {
        Component comp = get_component_by_index(bmp, component_index);
        if (comp.component_ptr == NULL || (comp.color != GREEN && comp.color != BLUE)) continue;

        uint8_t original_component = *comp.component_ptr;
        uint8_t pattern = (original_component >> 1) & 0x03;

        uint8_t bit = (data[bit_to_embed_count / 8] >> (7 - (bit_to_embed_count % 8))) & 0x01;

        *comp.component_ptr = (original_component & 0xFE) | bit;

        if (*comp.component_ptr != original_component) {
            pattern_changed[pattern]++;
        } else {
            pattern_unchanged[pattern]++;
        }

        bit_to_embed_count++;
    }

    if (bit_to_embed_count < num_bits) {
        return -1;
    }

    uint8_t pattern_map = 0;
    for (int p = 0; p < PATTERN_MAP_SIZE; p++) {
        if (pattern_changed[p] > pattern_unchanged[p]) {
            pattern_map |= (1 << p);
        }
    }

    size_t pattern_map_offset = *offset;
    uint8_t pattern_map_to_embed = pattern_map << 4; 
    if (lsb1_embed(bmp, &pattern_map_to_embed, PATTERN_MAP_SIZE, &pattern_map_offset) != 0) {
        return -1;
    }

    component_index = *offset + PATTERN_MAP_SIZE;
    bit_to_embed_count = 0;
    for (; component_index < max_component_index && bit_to_embed_count < num_bits; component_index++) {
        Component comp = get_component_by_index(bmp, component_index);
        if (comp.component_ptr == NULL || (comp.color != GREEN && comp.color != BLUE)) {
            continue;
        }

        uint8_t pattern = (*comp.component_ptr >> 1) & 0x03;

        if ((pattern_map & (1 << pattern)) && ((*comp.component_ptr & 0x01) != ((data[bit_to_embed_count / 8] >> (7 - (bit_to_embed_count % 8))) & 0x01))) {
            *comp.component_ptr ^= 0x01;
        }

        bit_to_embed_count++;
    }

    *offset = component_index;
    return 0;
}

int lsbi_extract(const BMPImage *bmp, size_t num_bits, uint8_t *buffer, size_t *offset, void *context) {
    if (bmp == NULL || bmp->data == NULL || buffer == NULL || offset == NULL || context == NULL) {
        return -1;
    }

    memset(buffer, 0, (num_bits + 7) / 8);

    size_t max_component_index = bmp->width * bmp->height * 3; 
    size_t component_index = *offset;
    size_t bit_extracted_count = 0;
    uint8_t pattern_map = *((uint8_t *)context) >> 4; 

    for (; component_index < max_component_index && bit_extracted_count < num_bits; component_index++) {

        Component comp = get_component_by_index(bmp, component_index);
        if (comp.component_ptr == NULL || (comp.color != GREEN && comp.color != BLUE)) {
            continue; 
        }
        
        uint8_t component = *comp.component_ptr;
        uint8_t pattern = (component >> 1) & 0x03; 

        if ((pattern_map & (1 << (3 - pattern))) != 0) { 
            component ^= 0x01;  
        }

        uint8_t bit = component & 0x01;
        buffer[bit_extracted_count / 8] |= (bit << (7 - (bit_extracted_count % 8)));
        bit_extracted_count++;
    }

    if (bit_extracted_count != num_bits) {
        return -1;
    }

    *offset = component_index;
    return 0;
}
