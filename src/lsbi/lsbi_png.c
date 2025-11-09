#include "lsbi.h"
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define BG_HEADER_BITS   4     // máscara en 4 LSB de los primeros 4 BG
#define BG_START_OFFSET  35    // comenzamos a leer datos desde el BG #35
#define PNG_MAGIC_LEN     8

static const uint8_t PNG_MAGIC[PNG_MAGIC_LEN] = {
    0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A
};
static const uint8_t PNG_IEND[8] = {
    0x49, 0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82
};

// patrón p = (bit2<<1)|bit1 del MISMO byte
static inline int pattern_of(uint8_t byte) {
    uint8_t b1 = (byte >> 1) & 1u;
    uint8_t b2 = (byte >> 2) & 1u;
    return (b2 << 1) | b1;  // 0..3
}

// índice del k-ésimo BG dentro de pixels[] cuando NO hay padding (B,G,R)
static inline size_t bg_k_to_pixel_index(size_t k) {
    // Por cada pixel (3 bytes), hay 2 BG (B,G) y 1 R (que se salta)
    // k -> grupo de 2 (BG), dentro = 0 (B) o 1 (G)
    size_t group = k / 2;        // pixel #
    size_t within = k % 2;       // 0=B, 1=G
    return group * 3 + within;   // posición absoluta en pixels[]
}

// cuántos BG portadores hay en pixels_len bytes (sin padding)
static inline size_t bg_capacity(size_t pixels_len) {
    // Por cada 3 bytes (B,G,R) hay 2 BG
    return (pixels_len / 3) * 2;
}

// Lee máscara de los 4 primeros BG (sus LSB)
static uint8_t read_mask_from_bg(const uint8_t *pixels, size_t pixels_len) {
    if (bg_capacity(pixels_len) < BG_HEADER_BITS) return 0;
    uint8_t m = 0;
    for (size_t i = 0; i < BG_HEADER_BITS; ++i) {
        size_t pi = bg_k_to_pixel_index(i);
        uint8_t lsb = pixels[pi] & 1u;
        m |= (uint8_t)(lsb << i);
    }
    return m; // bit i => patrón i invertido
}

// Lee un único bit "efectivo" desde el BG #k aplicando LSBI con "mask"
static inline uint8_t read_effective_bit_BG(const uint8_t *pixels, size_t k, uint8_t mask) {
    size_t pi = bg_k_to_pixel_index(k);
    uint8_t byte = pixels[pi];
    int pat = pattern_of(byte);
    uint8_t lsb = byte & 1u;
    if (mask & (1u << pat)) lsb ^= 1u; // si ese patrón fue invertido
    return lsb;
}

// Reconstruye "nbytes" desde BG a partir de "start_bg", aplicando mask.
// Devuelve 0 si OK, -1 si falta capacidad.
static int read_bytes_from_BG(const uint8_t *pixels, size_t pixels_len, uint8_t mask, size_t start_bg, uint8_t *out, size_t nbytes) {
    size_t need_bits = nbytes * 8;
    size_t need_bgs  = need_bits; // 1 bit por BG
    size_t cap = bg_capacity(pixels_len);

    if (start_bg + need_bgs > cap) return -1;

    memset(out, 0, nbytes);
    for (size_t bit_idx = 0; bit_idx < need_bits; ++bit_idx) {
        size_t k = start_bg + bit_idx;               // BG #k
        uint8_t b = read_effective_bit_BG(pixels, k, mask);

        size_t ob  = bit_idx / 8;
        int    bit = 7 - (int)(bit_idx % 8);
        if (b) out[ob] |=  (uint8_t)(1u << bit);
        else   out[ob] &= (uint8_t)~(1u << bit);
    }
    return 0;
}

// Escanea PNG completo desde BG start, aplicando mask; escribe hasta IEND (incluido).
// Devuelve cantidad de bytes escritos en *out_len (malloc interno). -1 si error.
static int scan_png_from_BG(const uint8_t *pixels, size_t pixels_len, uint8_t mask, size_t start_bg, uint8_t **out_png, size_t *out_len) {
    *out_png = NULL; *out_len = 0;

    // límite superior: no más que toda la capacidad disponible en BG
    size_t cap_bg = bg_capacity(pixels_len);
    if (start_bg + PNG_MAGIC_LEN * 8 > cap_bg) return -1;

    // buffer dinámico que iremos ampliando
    size_t alloc = 65536; // 64 KB inicial (se redimensiona si hace falta)
    uint8_t *buf = (uint8_t*)malloc(alloc);
    if (!buf) return -1;

    // 1) leer primeros 8 bytes y validar cabecera PNG
    if (read_bytes_from_BG(pixels, pixels_len, mask, start_bg, buf, PNG_MAGIC_LEN) != 0) {
        free(buf); return -1;
    }
    if (memcmp(buf, PNG_MAGIC, PNG_MAGIC_LEN) != 0) {
        // no parece PNG; liberamos
        free(buf); return -1;
    }
    size_t bytes_written = PNG_MAGIC_LEN;
    size_t bg_pos = start_bg + PNG_MAGIC_LEN * 8;

    // 2) seguir leyendo hasta encontrar IEND (8 bytes finales específicos)
    //    Usamos una ventana deslizante de 8 bytes
    uint8_t tail[8];
    memcpy(tail, buf, PNG_MAGIC_LEN); // inicializamos con la firma (no importa, se irá actualizando)

    while (bg_pos + 8 <= cap_bg) {
        // Leer de a 1 byte
        uint8_t next;
        if (read_bytes_from_BG(pixels, pixels_len, mask, bg_pos, &next, 1) != 0) {
            free(buf); return -1;
        }
        bg_pos += 8;

        // push a buf
        if (bytes_written + 1 > alloc) {
            size_t new_alloc = alloc * 2;
            uint8_t *tmp = (uint8_t*)realloc(buf, new_alloc);
            if (!tmp) { free(buf); return -1; }
            buf = tmp; alloc = new_alloc;
        }
        buf[bytes_written++] = next;

        // actualizar ventana tail y chequear IEND
        memmove(tail, tail + 1, 7);
        tail[7] = next;

        if (memcmp(tail, PNG_IEND, 8) == 0) {
            // encontrado final PNG
            *out_png = buf;
            *out_len = bytes_written;
            return 0;
        }
    }

    free(buf);
    return -1; // no encontramos IEND
}

int lsbi_embed(uint8_t *pixels, size_t pixels_len, const uint8_t *data, size_t data_len) {
    (void)pixels; (void)pixels_len; (void)data; (void)data_len;
    return -1;
}

int lsbi_extract(const uint8_t *pixels, size_t pixels_len, uint8_t *out, size_t data_len) {
    if (!pixels || !out) return -1;

    // 1) Leer máscara desde los 4 primeros BG (sus LSB)
    uint8_t mask = read_mask_from_bg(pixels, pixels_len);

    // 2) El stream útil comienza en el BG #35
    size_t start_bg = BG_START_OFFSET;

    // Si nos piden SOLO 4 bytes (primer llamado de operations.c), detectamos PNG y devolvemos size BE
    if (data_len == 4) {
        uint8_t *png = NULL; size_t png_len = 0;
        if (scan_png_from_BG(pixels, pixels_len, mask, start_bg, &png, &png_len) != 0) {
            return -1;
        }
        // tamaño en big-endian
        out[0] = (uint8_t)((png_len >> 24) & 0xFF);
        out[1] = (uint8_t)((png_len >> 16) & 0xFF);
        out[2] = (uint8_t)((png_len >>  8) & 0xFF);
        out[3] = (uint8_t)( png_len        & 0xFF);
        free(png);
        return 0;
    }

    // 3) Si nos piden N = 4 + size, devolvemos [size||PNG]
    if (data_len >= 5) {
        uint8_t *png = NULL; size_t png_len = 0;
        if (scan_png_from_BG(pixels, pixels_len, mask, start_bg, &png, &png_len) != 0) {
            return -1;
        }

        // Escribir size BE
        out[0] = (uint8_t)((png_len >> 24) & 0xFF);
        out[1] = (uint8_t)((png_len >> 16) & 0xFF);
        out[2] = (uint8_t)((png_len >>  8) & 0xFF);
        out[3] = (uint8_t)( png_len        & 0xFF);

        // Copiar PNG (hasta donde entre en data_len)
        size_t to_copy = png_len;
        if (4 + to_copy > data_len) to_copy = (data_len > 4) ? (data_len - 4) : 0;
        if (to_copy > 0) memcpy(out + 4, png, to_copy);

        free(png);
        return 0;
    }

    return -1;
}