#include "lsb4.h"
#include <stdio.h>

int lsb4_embed(uint8_t *pixels, size_t pixels_len, const uint8_t *data, size_t data_len) {
    printf("LSB4 embed: Placeholder - Not implemented yet!\n");
    printf("  Would embed %zu bytes into %zu pixel bytes\n", data_len, pixels_len);
    printf("  LSB4 uses 4 bits per pixel byte\n");
    
    // TODO: Implement LSB4 embedding algorithm
    // For now, just return success
    return 0;
}

int lsb4_extract(const uint8_t *pixels, size_t pixels_len, uint8_t *out, size_t data_len) {
    printf("LSB4 extract: Placeholder - Not implemented yet!\n");
    printf("  Would extract %zu bytes from %zu pixel bytes\n", data_len, pixels_len);
    printf("  LSB4 uses 4 bits per pixel byte\n");
    
    // TODO: Implement LSB4 extraction algorithm
    // For now, just return success
    return 0;
}

