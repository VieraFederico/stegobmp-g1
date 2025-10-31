#include "lsb4.h"
#include <stdio.h>

#include "lsb4.h"
#include <stdio.h>

int lsb4_embed(uint8_t *pixels, size_t pixels_len, const uint8_t *data, size_t data_len) {
    size_t bytes_needed = data_len * 2; 
    if (pixels_len < bytes_needed)
        return -1;

    size_t pixel_idx = 0;

    for (size_t i = 0; i < data_len; ++i) {
        uint8_t high = (data[i] >> 4) & 0x0F; // upper 
        uint8_t low  = data[i] & 0x0F;        // lower
        // 1010 1101 --> 0000 1010 -> 1010
        // 1010 1101 --> 0000 1101 -> 1101
        // embed high 
        pixels[pixel_idx] = (uint8_t)((pixels[pixel_idx] & 0xF0) | high);
        pixel_idx++;

        // embed low
        pixels[pixel_idx] = (uint8_t)((pixels[pixel_idx] & 0xF0) | low);
        pixel_idx++;
    }

    return 0; // TODO OK
}

int lsb4_extract(const uint8_t *pixels, size_t pixels_len, uint8_t *out, size_t data_len) {
    size_t bytes_needed = data_len * 2;
    if (pixels_len < bytes_needed)
        return -1;

    size_t pixel_idx = 0;

    for (size_t i = 0; i < data_len; ++i) {
        uint8_t high = pixels[pixel_idx++] & 0x0F;
        uint8_t low  = pixels[pixel_idx++] & 0x0F;
        out[i] = (uint8_t)((high << 4) | low);
    }

    return 0;
}
