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

// Busca el siguiente índice de componente GREEN/BLUE desde un offset
static inline size_t get_next_gb_index(size_t start) {
    while ((start % 3) == 2) start++; // Skip Red
    return start;
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
    
    // Necesitamos suficientes componentes GREEN/BLUE (aprox 2/3 de pixels_len)
    // Reservamos los primeros 4 componentes GREEN/BLUE para el pattern_map
    size_t component_index = get_next_gb_index(PATTERN_MAP_SIZE);
    size_t bit_to_embed_count = 0;
    
    // Contamos cuántos componentes GREEN/BLUE tenemos disponibles
    size_t available_gb_components = 0;
    for (size_t i = 0; i < pixels_len; i++) {
        if (is_green_or_blue(i)) available_gb_components++;
    }
    
    if (available_gb_components < bits_needed) return -1;

    // Guardamos los valores originales para contar los patterns
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
    component_index = get_next_gb_index(PATTERN_MAP_SIZE);
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
    // pattern 0 -> bit 0, pattern 1 -> bit 1, pattern 2 -> bit 2, pattern 3 -> bit 3
    uint8_t pattern_map = 0;
    for (int p = 0; p < 4; p++) {
        if (pattern_changed[p] > pattern_unchanged[p]) {
            pattern_map |= (1 << p);
        }
    }

    // Paso 3: Embeber el pattern_map en los primeros 4 componentes GREEN/BLUE usando LSB1
    // Movemos los 4 bits a las posiciones 4-7 (bit más significativo)
    // LSB1 lee desde el bit más significativo: bit 7, bit 6, bit 5, bit 4
    uint8_t pattern_map_to_embed = pattern_map << 4;
    size_t pattern_map_bits_embedded = 0;
    for (size_t i = 0; i < pixels_len && pattern_map_bits_embedded < PATTERN_MAP_SIZE; i++) {
        if (!is_green_or_blue(i)) continue;
        
        uint8_t bit = (pattern_map_to_embed >> (7 - pattern_map_bits_embedded)) & 0x01;
        pixels[i] = (pixels[i] & 0xFE) | bit;
        pattern_map_bits_embedded++;
    }

    // Paso 4: Aplicamos inversión de LSB según el pattern_map
    // IMPORTANTE: En extract, el pattern_map se invierte (pattern 0 -> bit 3)
    // y se usa (1 << (3 - pattern)) para verificar
    // Para que coincida, usamos el mismo pattern_map invertido aquí
    uint8_t pattern_map_inverted = 0;
    for (int p = 0; p < 4; p++) {
        if ((pattern_map & (1 << p)) != 0) {
            pattern_map_inverted |= (1 << (3 - p)); // pattern 0 -> bit 3, pattern 1 -> bit 2, etc.
        }
    }
    
    component_index = get_next_gb_index(PATTERN_MAP_SIZE);
    bit_to_embed_count = 0;
    orig_idx = 0;
    
    for (; component_index < pixels_len && bit_to_embed_count < num_bits; component_index++) {
        if (!is_green_or_blue(component_index)) continue;
        
        // Usamos el pattern del componente ORIGINAL (antes de modificar en paso 1)
        // porque el pattern se basa en los bits 1-2 que NO cambian
        uint8_t original_component = orig_values[orig_idx++];
        uint8_t pattern = pattern_from_byte(original_component);
        
        // Invertimos SIEMPRE si el pattern está marcado (igual que en extract)
        // LSBI invierte todos los bits de un pattern marcado, no solo los que no coinciden
        if ((pattern_map_inverted & (1 << (3 - pattern))) != 0) {
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
    
    // Inicializamos el buffer de salida
    memset(out, 0, data_len);

    // Paso 1: Leemos el pattern_map de los primeros 4 componentes GREEN/BLUE usando LSB1
    // LSB1 lee desde el bit más significativo: bit 7, bit 6, bit 5, bit 4
    uint8_t pattern_map_byte = 0;
    size_t pattern_map_bits_read = 0;
    for (size_t i = 0; i < pixels_len && pattern_map_bits_read < PATTERN_MAP_SIZE; i++) {
        if (!is_green_or_blue(i)) continue;
        
        uint8_t bit = pixels[i] & 0x01;
        pattern_map_byte |= (bit << (7 - pattern_map_bits_read));
        pattern_map_bits_read++;
    }
    
    // Movemos los bits de posiciones 4-7 a posiciones 0-3
    uint8_t pattern_map_raw = pattern_map_byte >> 4;
    
    // En extract, usamos (1 << (3 - pattern)) para verificar
    // Esto significa que pattern 0 verifica bit 3, pattern 1 verifica bit 2, etc.
    // Entonces necesitamos que pattern_map tenga pattern 0 en bit 3
    // Invertimos: pattern 0 -> bit 3, pattern 1 -> bit 2, pattern 2 -> bit 1, pattern 3 -> bit 0
    uint8_t pattern_map = 0;
    for (int p = 0; p < 4; p++) {
        if ((pattern_map_raw & (1 << p)) != 0) {
            pattern_map |= (1 << (3 - p));
        }
    }

    // Paso 2: Extraemos los datos embebidos
    size_t component_index = get_next_gb_index(PATTERN_MAP_SIZE);
    size_t bit_extracted_count = 0;

    for (; component_index < pixels_len && bit_extracted_count < num_bits; component_index++) {
        if (!is_green_or_blue(component_index)) continue;
        
        uint8_t component = pixels[component_index];
        uint8_t pattern = pattern_from_byte(component);
        
        // Verificamos si este pattern fue invertido usando el pattern_map
        // Usamos (1 << (3 - pattern)) para coincidir con embed
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


