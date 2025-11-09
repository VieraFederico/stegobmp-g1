#include "lsbi.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define HDR_BYTES 4  // cantidad de bytes reservados para la máscara (1 LSB por byte)

static inline uint8_t get_bit(const uint8_t *buf, size_t bit_idx) {
    size_t byte = bit_idx / 8;
    int    bit  = 7 - (int)(bit_idx % 8);
    return (uint8_t)((buf[byte] >> bit) & 1u);
}
static inline void set_bit(uint8_t *buf, size_t bit_idx, uint8_t v) {
    size_t byte = bit_idx / 8;
    int    bit  = 7 - (int)(bit_idx % 8);
    if (v) buf[byte] |=  (uint8_t)(1u << bit);
    else   buf[byte] &= (uint8_t)~(1u << bit);
}

// Patrón LSBI del MISMO byte portador: p = (bit2<<1) | bit1  (rango 0..3)
static inline int pattern_of(uint8_t carrier_byte) {
    uint8_t b1 = (carrier_byte >> 1) & 1u;
    uint8_t b2 = (carrier_byte >> 2) & 1u;
    return (b2 << 1) | b1;
}

// Escribir/leer máscara (4 bits) en los LSB de los primeros 4 bytes del portador
static inline void write_mask(uint8_t *pixels, uint8_t mask4) {
    for (int i = 0; i < HDR_BYTES; ++i) {
        uint8_t bit = (mask4 >> i) & 1u;
        pixels[i] = (uint8_t)((pixels[i] & 0xFEu) | bit);
    }
}
static inline uint8_t read_mask(const uint8_t *pixels) {
    uint8_t m = 0;
    for (int i = 0; i < HDR_BYTES; ++i) {
        m |= (uint8_t)((pixels[i] & 1u) << i);
    }
    return m;
}

int lsbi_embed(uint8_t *pixels, size_t pixels_len, const uint8_t *data, size_t data_len) {
    if (!pixels || !data) return -1;

    // Necesitamos: 4 portadores para la máscara + (data_len * 8) portadores para datos
    size_t bits_needed = data_len * 8;
    if (pixels_len < HDR_BYTES + bits_needed) return -1;

    // 1) Pre-cómputo de estadística changed/unchanged por patrón
    //    (comparando LSB original vs bit a embeber).
    size_t changed[4]   = {0,0,0,0};
    size_t unchanged[4] = {0,0,0,0};

    for (size_t bit_idx = 0; bit_idx < bits_needed; ++bit_idx) {
        size_t ci = HDR_BYTES + bit_idx;       // índice del byte portador
        uint8_t orig = pixels[ci];
        int pat = pattern_of(orig);

        uint8_t bit = get_bit(data, bit_idx);
        uint8_t before_lsb = orig & 1u;

        if (before_lsb != bit) changed[pat]++; else unchanged[pat]++;
    }

    // 2) Decidir máscara: invertir patrones con changed > unchanged
    uint8_t mask = 0;
    for (int p = 0; p < 4; ++p) {
        if (changed[p] > unchanged[p]) mask |= (uint8_t)(1u << p);
    }

    // 3) Embed base + inversión por patrón (sobre el rango de datos)
    for (size_t bit_idx = 0; bit_idx < bits_needed; ++bit_idx) {
        size_t ci = HDR_BYTES + bit_idx;
        uint8_t c = pixels[ci];

        // setear LSB con el bit del mensaje (embed base)
        uint8_t bit = get_bit(data, bit_idx);
        c = (uint8_t)((c & 0xFEu) | bit);

        // si el patrón de este byte está marcado, invertimos el LSB
        int pat = pattern_of(c);          // (bit1/bit2 no cambian por tocar el LSB)
        if (mask & (1u << pat)) c ^= 0x01u;

        pixels[ci] = c;
    }

    // 4) Escribir máscara en los LSB de pixels[0..3]
    write_mask(pixels, mask);

    return 0;
}

int lsbi_extract(const uint8_t *pixels, size_t pixels_len, uint8_t *out, size_t data_len) {
    if (!pixels || !out) return -1;

    size_t bits_to_recover = data_len * 8;
    if (pixels_len < HDR_BYTES + bits_to_recover) return -1;

    // 1) Leer máscara
    uint8_t mask = read_mask(pixels);

    // 2) Reconstruir bits aplicando inversión por patrón
    memset(out, 0, data_len);
    for (size_t bit_idx = 0; bit_idx < bits_to_recover; ++bit_idx) {
        size_t ci = HDR_BYTES + bit_idx;   // índice del byte portador de este bit
        uint8_t c = pixels[ci];

        int pat = pattern_of(c);
        uint8_t lsb = c & 1u;
        if (mask & (1u << pat)) lsb ^= 1u;

        // escribir en out (MSB primero)
        size_t ob  = bit_idx / 8;
        int    bit = 7 - (int)(bit_idx % 8);
        if (lsb) out[ob] |=  (uint8_t)(1u << bit);
        else     out[ob] &= (uint8_t)~(1u << bit);
    }
    return 0;
}