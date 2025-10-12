#include "lsb1.h"

int lsb1_embed(uint8_t *pixels, size_t pixels_len, const uint8_t *data, size_t data_len) {
    size_t bits_needed = data_len * 8;

    if (pixels_len < bits_needed) 
        return -1;

    size_t bit_idx = 0;

    for (size_t i = 0; i < data_len; i++) {
        for (int b = 7; b >= 0; --b) {
            uint8_t bit = (data[i] >> b) & 1u;
            // escribir bit en LSB del byte de pixel correspondiente
            pixels[bit_idx] = (uint8_t)((pixels[bit_idx] & 0xFE) | bit);
            bit_idx++;
        }
    }
    return 0;
}

int lsb1_extract(const uint8_t *pixels, size_t pixels_len, uint8_t *out, size_t data_len) {
    size_t bits_needed = data_len * 8;

    if (pixels_len < bits_needed) 
        return -1;

    size_t bit_idx = 0;
    
    for (size_t i = 0; i < data_len; i++) {
        uint8_t v = 0;
        for (int b = 7; b >= 0; --b) {
            uint8_t bit = (uint8_t)(pixels[bit_idx] & 1u);
            v |= (bit << b);
            bit_idx++;
        }
        out[i] = v;
    }
    return 0;
}
