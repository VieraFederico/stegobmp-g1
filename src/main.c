#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "./bmp_handler/bmp_handler.h"
#include "./lsb1/lsb1.h"

static int read_file(const char *path, uint8_t **buf, size_t *len) {
    *buf = NULL; *len = 0;
    FILE *f = fopen(path, "rb");

    if (!f) 
        return -1;

    if (fseek(f, 0, SEEK_END) != 0) { 
        fclose(f); 
        return -2; 
    }

    long sz = ftell(f);

    if (sz < 0) { 
        fclose(f); 
        return -3; 
    }

    if (fseek(f, 0, SEEK_SET) != 0) { 
        fclose(f); 
        return -4; 
    }

    *buf = (uint8_t*)malloc((size_t)sz);

    if (!*buf) { 
        fclose(f); 
        return -5; 
    }

    if (fread(*buf, 1, (size_t)sz, f) != (size_t)sz) { 
        free(*buf); 
        fclose(f); 
        return -6; 
    }

    fclose(f);
    *len = (size_t)sz;

    return 0;
}

static int write_file(const char *path, const uint8_t *buf, size_t len) {
    FILE *f = fopen(path, "wb");

    if (!f) 
        return -1;

    if (fwrite(buf, 1, len, f) != len) { 
        fclose(f); 
        return -2; 
    }

    fclose(f);

    return 0;
}

static void u32_to_be(uint32_t v, uint8_t out[4]) {
    out[0] = (uint8_t)((v >> 24) & 0xFF);
    out[1] = (uint8_t)((v >> 16) & 0xFF);
    out[2] = (uint8_t)((v >> 8)  & 0xFF);
    out[3] = (uint8_t)( v        & 0xFF);
}
static uint32_t be_to_u32(const uint8_t in[4]) {
    return ((uint32_t)in[0] << 24) | ((uint32_t)in[1] << 16) | ((uint32_t)in[2] << 8) | (uint32_t)in[3];
}

int main(int argc, char **argv) {
    // args mínimos para arrancar
    int embed = 0, extract = 0;
    const char *in_file = NULL, *carrier = NULL, *out_file = NULL, *steg = NULL;

    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-embed") == 0) 
            embed = 1;
        else if (strcmp(argv[i], "-extract") == 0) 
            extract = 1;
        else if (strcmp(argv[i], "-in") == 0 && i+1 < argc) 
            in_file = argv[++i];
        else if (strcmp(argv[i], "-p") == 0 && i+1 < argc) 
            carrier = argv[++i];
        else if (strcmp(argv[i], "-out") == 0 && i+1 < argc) 
            out_file = argv[++i];
        else if (strcmp(argv[i], "-steg") == 0 && i+1 < argc) 
            steg = argv[++i];
    }

    if ((!embed && !extract) || !carrier || !out_file || !steg) {
        fprintf(stderr, "Uso:\n");
        fprintf(stderr, "  stegobmp -embed -in file -p carrier.bmp -out out.bmp -steg LSB1\n");
        fprintf(stderr, "  stegobmp -extract -p out.bmp -out recovered -steg LSB1\n");
        return 1;
    }

    if (strcmp(steg, "LSB1") != 0) {
        fprintf(stderr, "Por ahora solo LSB1 en este skeleton.\n");
        return 1;
    }

    Bmp bmp;
    if (bmp_read(carrier, &bmp) != 0) {
        fprintf(stderr, "Error leyendo BMP (24bpp sin compresion requerido)\n");
        return 1;
    }

    int rc = 0;
    if (embed) {
        if (!in_file) { 
            fprintf(stderr, "Falta -in\n"); 
            bmp_free(&bmp); 
            return 1; 
        }

        // leer archivo a ocultar
        uint8_t *inbuf = NULL; 
        size_t inlen = 0;

        if (read_file(in_file, &inbuf, &inlen) != 0) { 
            fprintf(stderr, "No pude leer -in\n"); 
            bmp_free(&bmp); 
            return 1; 
        }

        // formar bloque: tam(4 BE) || datos || extension(".ext\0")
        const char *dot = strrchr(in_file, '.');
        char extbuf[64];

        if (dot) 
            snprintf(extbuf, sizeof(extbuf), "%s", dot);
        else 
            snprintf(extbuf, sizeof(extbuf), ".bin");

        size_t extlen = strlen(extbuf) + 1; // incluir '\0'

        size_t payload_len = 4 + inlen + extlen;
        uint8_t *payload = (uint8_t*)malloc(payload_len);

        if (!payload) { 
            free(inbuf); 
            bmp_free(&bmp); 
            return 1; 
        }

        u32_to_be((uint32_t)inlen, payload);
        memcpy(payload + 4, inbuf, inlen);
        memcpy(payload + 4 + inlen, extbuf, extlen);

        size_t cap_bytes = bmp_payload_size(&bmp) / 8; // LSB1: 1 bit por byte -> /8
        if (payload_len > cap_bytes) {
            fprintf(stderr, "Capacidad insuficiente. Max (LSB1): %zu bytes\n", cap_bytes);
            free(payload); 
            free(inbuf); 
            bmp_free(&bmp); 
            return 1;
        }
        // embed
        if ((rc = lsb1_embed(bmp.pixels, bmp.pixelsSize, payload, payload_len)) != 0) {
            fprintf(stderr, "Fallo embed LSB1\n");
            free(payload); 
            free(inbuf); 
            bmp_free(&bmp); 
            return 1;
        }
        // escribir BMP resultante
        if (bmp_write(out_file, &bmp) != 0) {
            fprintf(stderr, "No pude escribir BMP de salida\n");
            free(payload); 
            free(inbuf); 
            bmp_free(&bmp); 
            return 1;
        }

        free(payload); free(inbuf);
        printf("OK: incrustado %zu bytes (incluye tamaño y extension)\n", payload_len);

    } else if (extract) {
        // extraer primeros 4 bytes (tam real) -> saber cuánto leer
        uint8_t size_be[4];
        if ((rc = lsb1_extract(bmp.pixels, bmp.pixelsSize, size_be, 4)) != 0) {
            fprintf(stderr, "Fallo extract size\n"); 
            bmp_free(&bmp); 
            return 1;
        }
        uint32_t real_size = be_to_u32(size_be);

        // luego leer real_size + extension (desconocida hasta '\0')
        size_t head_bits = 4 * 8;
        size_t remaining_bytes = real_size + 16; // margen para extension (".algo\0")
        uint8_t *tmp = (uint8_t*)malloc(remaining_bytes);

        if (!tmp) { 
            bmp_free(&bmp); 
            return 1; 
        }

        // extraer datos+ext desde el offset de bits 32 en adelante
        // más simple: re-extraemos todo seguido
        uint8_t *all = (uint8_t*)malloc(4 + remaining_bytes);
        if (!all) { 
            free(tmp); 
            bmp_free(&bmp); 
            return 1; 
        }

        if ((rc = lsb1_extract(bmp.pixels, bmp.pixelsSize, all, 4 + remaining_bytes)) != 0) {
            fprintf(stderr, "Fallo extract bloque\n");
            free(all); 
            free(tmp); 
            bmp_free(&bmp); 
            return 1;
        }
        // all: [size(4)] [datos ...] [extension...]
        uint8_t *data_ptr = all + 4;
        // buscar '\0' de extensión después de los datos
        const char *ext = (const char*)(data_ptr + real_size);
        size_t ext_found = strnlen(ext, remaining_bytes - real_size);

        if (ext_found == remaining_bytes - real_size) {
            fprintf(stderr, "No encontre terminador de extension\n");
            free(all); 
            free(tmp); 
            bmp_free(&bmp); 
            return 1;
        }
        // escribir out_file (solo datos)
        if (write_file(out_file, data_ptr, real_size) != 0) {
            fprintf(stderr, "No pude escribir archivo de salida\n");
            free(all); 
            free(tmp); 
            bmp_free(&bmp); 
            return 1;
        }
        printf("OK: extraidos %u bytes, extension recuperada: %s\n", real_size, ext);
        free(all); 
        free(tmp);
    }

    bmp_free(&bmp);
    return 0;
}
