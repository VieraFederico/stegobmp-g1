#include "lsbi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PATTERN_MAP_SIZE 4

// Helper: Check if a pixel index corresponds to GREEN or BLUE component
// BMP format is BGR: index % 3 == 0 (Blue), 1 (Green), 2 (Red)
// We only use Blue (0) and Green (1), skip Red (2)
static inline int is_green_or_blue(size_t index) {
    return (index % 3) != 2; // Skip Red components (index % 3 == 2)
}

// Get the next GREEN or BLUE component index starting from a given offset
static inline size_t get_next_gb_index(size_t start) {
    while ((start % 3) == 2) start++; // Skip Red components
    return start;
}

// patrón = bits 1..2 (contando bit0 como LSB). 00->0, 01->1, 10->2, 11->3
static inline unsigned pattern_from_byte(uint8_t b) {
    return (b >> 1) & 0x03;
}

int lsbi_embed(uint8_t *pixels, size_t pixels_len, const uint8_t *data, size_t data_len) {
    if (!pixels || !data) return -1;

    const size_t num_bits = data_len * 8;
    const size_t bits_needed = PATTERN_MAP_SIZE + num_bits;
    
    // We need enough GREEN/BLUE components (approximately 2/3 of pixels_len)
    // Reserve first 4 GREEN/BLUE components for pattern_map
    size_t component_index = get_next_gb_index(PATTERN_MAP_SIZE);
    size_t bit_to_embed_count = 0;
    
    // Count how many GREEN/BLUE components we have available
    size_t available_gb_components = 0;
    for (size_t i = 0; i < pixels_len; i++) {
        if (is_green_or_blue(i)) available_gb_components++;
    }
    
    if (available_gb_components < bits_needed) return -1;

    // Store original values for pattern counting
    size_t max_components_needed = component_index + num_bits;
    uint8_t *orig_values = (uint8_t*)malloc(num_bits);
    if (!orig_values) return -2;
    
    size_t orig_idx = 0;
    for (size_t i = component_index; i < pixels_len && bit_to_embed_count < num_bits; i++) {
        if (!is_green_or_blue(i)) continue;
        orig_values[orig_idx++] = pixels[i];
        bit_to_embed_count++;
    }
    
    if (bit_to_embed_count < num_bits) {
        free(orig_values);
        return -1;
    }

    // Reset for actual embedding
    component_index = get_next_gb_index(PATTERN_MAP_SIZE);
    bit_to_embed_count = 0;
    size_t pattern_changed[4] = {0, 0, 0, 0};
    size_t pattern_unchanged[4] = {0, 0, 0, 0};
    orig_idx = 0;

    // Paso 1: Insertar datos usando LSB1 y contar cambios
    for (; component_index < pixels_len && bit_to_embed_count < num_bits; component_index++) {
        if (!is_green_or_blue(component_index)) continue;
        
        uint8_t original_component = pixels[component_index];
        uint8_t pattern = pattern_from_byte(original_component);
        uint8_t bit = (data[bit_to_embed_count / 8] >> (7 - (bit_to_embed_count % 8))) & 0x01;
        
        pixels[component_index] = (original_component & 0xFE) | bit;
        
        if (pixels[component_index] != original_component) {
            pattern_changed[pattern]++;
        } else {
            pattern_unchanged[pattern]++;
        }
        
        bit_to_embed_count++;
    }

    // Paso 2: Construir pattern_map
    // Los bits se almacenan en orden inverso: bit 3 (pattern 0) primero, bit 0 (pattern 3) último
    uint8_t pattern_map = 0;
    for (int p = 0; p < 4; p++) {
        if (pattern_changed[p] > pattern_unchanged[p]) {
            pattern_map |= (1 << (3 - p)); // Almacenar en orden inverso para coincidir con extract
        }
    }

    // Paso 3: Embeber pattern_map en los primeros 4 componentes GREEN/BLUE usando LSB1
    size_t pattern_map_offset = 0;
    size_t pattern_map_bits_embedded = 0;
    for (size_t i = 0; i < pixels_len && pattern_map_bits_embedded < PATTERN_MAP_SIZE; i++) {
        if (!is_green_or_blue(i)) continue;
        
        uint8_t bit = (pattern_map >> (3 - pattern_map_bits_embedded)) & 0x01;
        pixels[i] = (pixels[i] & 0xFE) | bit;
        pattern_map_bits_embedded++;
    }

    // Paso 4: Aplicar inversión de LSB según pattern_map
    component_index = get_next_gb_index(PATTERN_MAP_SIZE);
    bit_to_embed_count = 0;
    
    for (; component_index < pixels_len && bit_to_embed_count < num_bits; component_index++) {
        if (!is_green_or_blue(component_index)) continue;
        
        uint8_t pattern = pattern_from_byte(pixels[component_index]);
        uint8_t desired_bit = (data[bit_to_embed_count / 8] >> (7 - (bit_to_embed_count % 8))) & 0x01;
        uint8_t current_bit = pixels[component_index] & 0x01;
        
        // Solo invertir si el patrón está marcado y el LSB actualmente no coincide
        if ((pattern_map & (1 << (3 - pattern))) && (current_bit != desired_bit)) {
            pixels[component_index] ^= 0x01;
        }
        
        bit_to_embed_count++;
    }

    free(orig_values);
    return 0;
}

int lsbi_extract(const uint8_t *pixels, size_t pixels_len, uint8_t *out, size_t data_len) {
    if (!pixels || !out) return -1;

    const size_t num_bits = data_len * 8;
    const size_t bits_needed = PATTERN_MAP_SIZE + num_bits;
    
    // Initialize output buffer
    memset(out, 0, data_len);

    // Paso 1: Leer pattern_map de los primeros 4 componentes GREEN/BLUE
    uint8_t pattern_map = 0;
    size_t pattern_map_bits_read = 0;
    for (size_t i = 0; i < pixels_len && pattern_map_bits_read < PATTERN_MAP_SIZE; i++) {
        if (!is_green_or_blue(i)) continue;
        
        uint8_t bit = pixels[i] & 0x01;
        pattern_map |= (bit << (3 - pattern_map_bits_read));
        pattern_map_bits_read++;
    }

    // Paso 2: Extraer los datos embebidos
    size_t component_index = get_next_gb_index(PATTERN_MAP_SIZE);
    size_t bit_extracted_count = 0;

    for (; component_index < pixels_len && bit_extracted_count < num_bits; component_index++) {
        if (!is_green_or_blue(component_index)) continue;
        
        uint8_t component = pixels[component_index];
        uint8_t pattern = pattern_from_byte(component);
        
        // Verificar si este patrón fue invertido usando el pattern_map
        if ((pattern_map & (1 << (3 - pattern))) != 0) {
            component ^= 0x01;  // Invertir el LSB
        }
        
        // Extraer el LSB y almacenar en el buffer
        uint8_t bit = component & 0x01;
        out[bit_extracted_count / 8] |= (bit << (7 - (bit_extracted_count % 8)));
        
        bit_extracted_count++;
    }

    // Verificar que se hayan extraído todos los bits
    if (bit_extracted_count != num_bits) {
        return -1;
    }

    return 0;
}

