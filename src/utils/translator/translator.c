#include "translator.h"

void u32_to_be(uint32_t v, uint8_t out[4]) {
    out[0] = (uint8_t)((v >> 24) & 0xFF);
    out[1] = (uint8_t)((v >> 16) & 0xFF);
    out[2] = (uint8_t)((v >> 8)  & 0xFF);
    out[3] = (uint8_t)( v        & 0xFF);
}


uint32_t be_to_u32(const uint8_t in[4]) {
    return ((uint32_t)in[0] << 24) | ((uint32_t)in[1] << 16) | ((uint32_t)in[2] << 8) | (uint32_t)in[3];
}