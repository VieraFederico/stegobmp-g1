#include "lsbi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PATTERN_MAP_SIZE 4

// Chequea si un índice de pixel corresponde a GREEN o BLUE
// BMP es BGR: index % 3 == 0 (Blue), 1 (Green), 2 (Red)
// Solo usamos Blue y Green, saltamos Red
static inline int is_green_or_blue(size_t index) {
    return (index % 3) != 2;
}

// Extrae el patrón de los bits 1 y 2 (bit 0 es LSB)
// 00->0, 01->1, 10->2, 11->3
static inline unsigned pattern_from_byte(uint8_t b) {
    return (b >> 1) & 0x03;
}

int lsbi_embed(uint8_t *pixels, size_t pixels_len, const uint8_t *data, size_t data_len) {
    if (!pixels || !data) return -1;

    const size_t num_bits = data_len * 8;
    const size_t bits_needed = PATTERN_MAP_SIZE + num_bits;
    
    // Contamos cuántos componentes GREEN/BLUE tenemos disponibles
    size_t available_gb_components = 0;
    for (size_t i = 0; i < pixels_len; i++) {
        if (is_green_or_blue(i)) available_gb_components++;
    }
    
    if (available_gb_components < bits_needed) return -1;

    // Guardamos los valores originales para contar los patterns
    size_t component_index = PATTERN_MAP_SIZE; // Skip first 4 for pattern_map
    size_t bit_to_embed_count = 0;
    
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

    // Reset para empezar el embedding real
    component_index = PATTERN_MAP_SIZE;
    bit_to_embed_count = 0;
    size_t pattern_changed[4] = {0, 0, 0, 0};
    size_t pattern_unchanged[4] = {0, 0, 0, 0};
    orig_idx = 0;

    // Paso 1: Insertamos los datos usando LSB1 y contamos cuántos cambios hay por pattern
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

    // Paso 2: Construimos el pattern_map
    // Si un pattern tuvo más cambios que no-cambios, lo marcamos para invertir
    uint8_t pattern_map = 0;
    for (int p = 0; p < 4; p++) {
        if (pattern_changed[p] > pattern_unchanged[p]) {
            pattern_map |= (1 << p);
        }
    }

    // Paso 3: Embeber el pattern_map en los primeros 4 componentes GREEN/BLUE usando LSB1
    // Movemos los 4 bits a las posiciones 4-7 (bit más significativo)
    uint8_t pattern_map_to_embed = pattern_map << 4;
    size_t pattern_map_bits_embedded = 0;
    for (size_t i = 0; i < pixels_len && pattern_map_bits_embedded < PATTERN_MAP_SIZE; i++) {
        if (!is_green_or_blue(i)) continue;
        
        uint8_t bit = (pattern_map_to_embed >> (7 - pattern_map_bits_embedded)) & 0x01;
        pixels[i] = (pixels[i] & 0xFE) | bit;
        pattern_map_bits_embedded++;
    }

    // Paso 4: Aplicamos inversión de LSB según el pattern_map
    component_index = PATTERN_MAP_SIZE;
    bit_to_embed_count = 0;
    orig_idx = 0;
    
    for (; component_index < pixels_len && bit_to_embed_count < num_bits; component_index++) {
        if (!is_green_or_blue(component_index)) continue;
        
        // Usamos el pattern del componente ORIGINAL (antes de modificar en paso 1)
        uint8_t original_component = orig_values[orig_idx++];
        uint8_t pattern = pattern_from_byte(original_component);
        
        // Solo invertir si el pattern está marcado Y el bit actualmente no coincide
        if ((pattern_map & (1 << pattern)) != 0) {
            uint8_t expected_bit = (data[bit_to_embed_count / 8] >> (7 - (bit_to_embed_count % 8))) & 0x01;
            uint8_t current_bit = pixels[component_index] & 0x01;
            if (current_bit != expected_bit) {
                pixels[component_index] ^= 0x01;
            }
        }
        
        bit_to_embed_count++;
    }

    free(orig_values);
    return 0;
}

int lsbi_extract(const uint8_t *pixels, size_t pixels_len, uint8_t *out, size_t data_len) {
    if (!pixels || !out) return -1;

    const size_t num_bits = data_len * 8;
    
    // Inicializamos el buffer de salida
    memset(out, 0, data_len);

    // Paso 1: Leemos el pattern_map de los primeros 4 componentes GREEN/BLUE usando LSB1
    uint8_t pattern_map_byte = 0;
    size_t pattern_map_bits_read = 0;
    for (size_t i = 0; i < pixels_len && pattern_map_bits_read < PATTERN_MAP_SIZE; i++) {
        if (!is_green_or_blue(i)) continue;
        
        uint8_t bit = pixels[i] & 0x01;
        pattern_map_byte |= (bit << (7 - pattern_map_bits_read));
        pattern_map_bits_read++;
    }
    
    // Movemos los bits de posiciones 4-7 a posiciones 0-3
    uint8_t pattern_map = pattern_map_byte >> 4;

    // Paso 2: Extraemos los datos embebidos
    size_t component_index = PATTERN_MAP_SIZE;
    size_t bit_extracted_count = 0;

    for (; component_index < pixels_len && bit_extracted_count < num_bits; component_index++) {
        if (!is_green_or_blue(component_index)) continue;
        
        uint8_t component = pixels[component_index];
        uint8_t pattern = pattern_from_byte(component);
        
        // Verificamos si este pattern fue invertido usando el pattern_map
        // Usamos (1 << (3 - pattern)) para verificar (pattern 0 -> bit 3, pattern 1 -> bit 2, etc.)
        if ((pattern_map & (1 << (3 - pattern))) != 0) {
            component ^= 0x01;  // Invertimos el LSB
        }
        
        // Extraemos el LSB y lo guardamos en el buffer
        uint8_t bit = component & 0x01;
        out[bit_extracted_count / 8] |= (bit << (7 - (bit_extracted_count % 8)));
        
        bit_extracted_count++;
    }

    // Verificamos que hayamos extraído todos los bits
    if (bit_extracted_count != num_bits) {
        return -1;
    }

    return 0;
}
