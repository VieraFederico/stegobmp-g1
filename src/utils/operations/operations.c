#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "../../bmp_handler/bmp_handler.h"
#include "../../lsb1/lsb1.h"
#include "../../lsb4/lsb4.h"
#include "../../lsbi/lsbi.h"
#include "../file_management/file_management.h"
#include "../translator/translator.h"
#include "../parser/parser.h"
#include "operations.h"

int perform_embed(const stegobmp_config_t *config, const Bmp *bmp)
{
    uint8_t *inbuf = NULL;
    size_t inlen = 0;
    
    // Read input file to embed
    if (read_file(config->in_file, &inbuf, &inlen) != 0)
    {
        fprintf(stderr, "No pude leer -in\n");
        return 1;
    }

    // Build payload: size(4 BE) || data || extension(".ext\0")
    const char *dot = strrchr(config->in_file, '.');
    char extbuf[64];

    if (dot)
        snprintf(extbuf, sizeof(extbuf), "%s", dot);
    else
        snprintf(extbuf, sizeof(extbuf), ".bin");

    size_t extlen = strlen(extbuf) + 1; // include '\0'

    size_t payload_len = 4 + inlen + extlen;
    uint8_t *payload = (uint8_t *)malloc(payload_len);

    if (!payload)
    {
        free(inbuf);
        return 1;
    }

    u32_to_be((uint32_t)inlen, payload);
    memcpy(payload + 4, inbuf, inlen);
    memcpy(payload + 4 + inlen, extbuf, extlen);

    size_t cap_bytes = bmp_payload_size(bmp) / 8; // LSB1: 1 bit per byte -> /8
    if (payload_len > cap_bytes)
    {
        fprintf(stderr, "Capacidad insuficiente. Max (LSB1): %zu bytes\n", cap_bytes);
        free(payload);
        free(inbuf);
        return 1;
    }
    switch (config->steg_method)
    {
        //@TODO: IDEA, change to funpointer
        case STEG_LSB1:
            if (lsb1_embed(bmp->pixels, bmp->pixelsSize, payload, payload_len) != 0)
            {
                fprintf(stderr, "Fallo embed LSB1\n");
                free(payload);
                free(inbuf);
                return 1;
            }
            break;
        case STEG_LSB4:
            if (lsb4_embed(bmp->pixels, bmp->pixelsSize, payload, payload_len) != 0)
            {
                fprintf(stderr, "Fallo embed LSB4\n");
                free(payload);
                free(inbuf);
                return 1;
            }
            break;
        case STEG_LSBI:
            if (lsbi_embed(bmp->pixels, bmp->pixelsSize, payload, payload_len) != 0)
            {
                fprintf(stderr, "Fallo embed LSBI\n");
                free(payload);
                free(inbuf);
                return 1;
            }
            break;
        default:
            break;
    }
    // Embed using LSB1
    /*
    if (lsb1_embed(bmp->pixels, bmp->pixelsSize, payload, payload_len) != 0)
    {
        fprintf(stderr, "Fallo embed LSB1\n");
        free(payload);
        free(inbuf);
        return 1;
    }
    
    */

    // Write output BMP file
    if (bmp_write(config->out_file, bmp) != 0)
    {
        fprintf(stderr, "No pude escribir BMP de salida\n");
        free(payload);
        free(inbuf);
        return 1;
    }

    free(payload);
    free(inbuf);
    printf("OK: incrustado %zu bytes (incluye tamaño y extension)\n", payload_len);
    return 0;
}

int perform_extract(const stegobmp_config_t *config, const Bmp *bmp)
{
    uint8_t size_be[4];
    if (lsb1_extract(bmp->pixels, bmp->pixelsSize, size_be, 4) != 0)
    {
        fprintf(stderr, "Fallo extract size\n");
        return 1;
    }
    uint32_t real_size = be_to_u32(size_be);

    // Read real_size + extension (unknown until '\0')
    size_t remaining_bytes = real_size + 16; // margin for extension (".algo\0")
    uint8_t *tmp = (uint8_t *)malloc(remaining_bytes);

    if (!tmp)
    {
        return 1;
    }

    // Extract data+ext from bit offset 32 onwards
    // Simpler: re-extract everything in sequence
    uint8_t *all = (uint8_t *)malloc(4 + remaining_bytes);
    if (!all)
    {
        free(tmp);
        return 1;
    }

    if (lsb1_extract(bmp->pixels, bmp->pixelsSize, all, 4 + remaining_bytes) != 0)
    {
        fprintf(stderr, "Fallo extract bloque\n");
        free(all);
        free(tmp);
        return 1;
    }
    
    // all: [size(4)] [data ...] [extension...]
    uint8_t *data_ptr = all + 4;
    
    // Find '\0' of extension after the data
    const char *ext = (const char *)(data_ptr + real_size);
    size_t ext_found = strnlen(ext, remaining_bytes - real_size);

    if (ext_found == remaining_bytes - real_size)
    {
        fprintf(stderr, "No encontre terminador de extension\n");
        free(all);
        free(tmp);
        return 1;
    }
    
    // Write out_file (data only)
    if (write_file(config->out_file, data_ptr, real_size) != 0)
    {
        fprintf(stderr, "No pude escribir archivo de salida\n");
        free(all);
        free(tmp);
        return 1;
    }
    
    printf("OK: extraidos %u bytes, extension recuperada: %s\n", real_size, ext);
    free(all);
    free(tmp);
    return 0;
}