#include "lsbi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// patrón = bits 1..2 (contando bit0 como LSB). 00->0, 01->1, 10->2, 11->3
static inline unsigned pattern_from_byte(uint8_t b) {
    unsigned b1 = (b >> 1) & 1u;
    unsigned b2 = (b >> 2) & 1u;
    return (b2 << 1) | b1;
}

// Lee flags (4 bits) guardados en los LSB de los primeros 4 bytes del bloque de píxeles
static inline void lsbi_read_flags(const uint8_t *pixels, uint8_t inv[4]) {
    inv[0] = pixels[0] & 1u; // patrón 00
    inv[1] = pixels[1] & 1u; // patrón 01
    inv[2] = pixels[2] & 1u; // patrón 10
    inv[3] = pixels[3] & 1u; // patrón 11
}

// Escribe flags (en orden 00,01,10,11) en los LSB de los primeros 4 bytes
static inline void lsbi_write_flags(uint8_t *pixels, const uint8_t inv[4]) {
    for (int p = 0; p < 4; ++p)
        pixels[p] = (uint8_t)((pixels[p] & 0xFE) | (inv[p] & 1u));
}

int lsbi_embed(uint8_t *pixels, size_t pixels_len, const uint8_t *data, size_t data_len) {
    if (!pixels || !data) return -1;

    // bits necesarios: 4 (mapa) + payload*8
    const size_t used_bytes = data_len * 8;
    const size_t bits_needed = 4 + used_bytes;
    if (pixels_len < bits_needed) return -1;

    // Guardamos copia de los bytes que vamos a tocar para contar cambios
    uint8_t *orig = (uint8_t*)malloc(used_bytes);
    if (!orig) return -2;
    memcpy(orig, pixels + 4, used_bytes);

    // Paso 1: LSB1 "plano" desde offset 4 (reservamos 0..3 para el mapa)
    size_t bit_idx = 0;
    for (size_t i = 0; i < data_len; ++i) {
        for (int b = 7; b >= 0; --b) {
            const uint8_t bit = (data[i] >> b) & 1u;
            const size_t px = 4 + bit_idx;
            pixels[px] = (uint8_t)((pixels[px] & 0xFE) | bit);
            ++bit_idx;
        }
    }

    // Paso 2: contar cambios por patrón (comparando LSB antes vs después)
    size_t changed[4]   = {0,0,0,0};
    size_t unchanged[4] = {0,0,0,0};
    for (size_t i = 0; i < used_bytes; ++i) {
        const uint8_t before = orig[i];
        const uint8_t after  = pixels[4 + i];
        const unsigned p = pattern_from_byte(before);
        if ((before & 1u) != (after & 1u)) changed[p]++; else unchanged[p]++;
    }

    // Paso 3: decidir inversión para cada patrón
    uint8_t invert[4] = {0,0,0,0};
    for (int p = 0; p < 4; ++p)
        if (changed[p] > unchanged[p]) invert[p] = 1;

    // Paso 4: aplicar inversión (flip LSB) donde corresponda
    for (size_t i = 0; i < used_bytes; ++i) {
        const unsigned p = pattern_from_byte(orig[i]);
        if (invert[p]) pixels[4 + i] ^= 1u;
    }

    // Paso 5: escribir el mapa de 4 bits en los 4 primeros bytes
    lsbi_write_flags(pixels, invert);

    free(orig);
    return 0;
}

int lsbi_extract(const uint8_t *pixels, size_t pixels_len, uint8_t *out, size_t data_len) {
    if (!pixels || !out) return -1;

    const size_t bits_needed = 4 + data_len * 8;
    if (pixels_len < bits_needed) return -1;

    uint8_t invert[4];
    lsbi_read_flags(pixels, invert);

    // El payload arranca en el byte índice 4 (porque 0..3 guardan el mapa)
    const size_t start_bit = 4;

    size_t bit_idx = 0;
    for (size_t i = 0; i < data_len; ++i) {
        uint8_t v = 0;
        for (int b = 7; b >= 0; --b) {
            const size_t px = start_bit + bit_idx; // índice de byte del bloque de píxeles
            uint8_t byte = pixels[px];
            unsigned p = pattern_from_byte(byte); // bits 1..2 no los tocamos, sirven para clasificar
            uint8_t lsb = (uint8_t)(byte & 1u);
            if (invert[p]) lsb ^= 1u;            // deshacer inversión
            v |= (uint8_t)(lsb << b);
            ++bit_idx;
        }
        out[i] = v;
    }
    return 0;
}

