#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "./bmp_handler/bmp_handler.h"
#include "./utils/parser/parser.h"
#include "./utils/operations/operations.h"

int main(int argc, char **argv)
{
    stegobmp_config_t config;

    // Parse and validate arguments
    if (parse_arguments(argc, argv, &config) != 0)
    {
        fprintf(stderr, "%s\n", config.error_message);
        fprintf(stderr, "\nUsage:\n");
        fprintf(stderr, "  stegobmp -embed -in file -p carrier.bmp -out out.bmp -steg LSB1\n");
        fprintf(stderr, "  stegobmp -extract -p out.bmp -out recovered -steg LSB1\n");
        fprintf(stderr, "  stegobmp -embed -in file -p carrier.bmp -out out.bmp -steg LSB1 -a aes256 -m cbc -pass mypassword\n");
        free_config(&config);
        return 1;
    }

    // Load BMP file
    Bmp bmp;
    if (bmp_read(config.carrier_file, &bmp) != 0)
    {
        fprintf(stderr, "Error leyendo BMP (24bpp sin compresion requerido)\n");
        free_config(&config);
        return 1;
    }
    int rc=0;

    switch (config.operation)
    {
    case OP_EMBED:
        rc = perform_embed(&config, &bmp);
        break;
    case OP_EXTRACT:
        rc = perform_extract(&config, &bmp);
        break;
    default:
        fprintf(stderr, "not valid operation\n");
        break;
    }

    bmp_free(&bmp);
    free_config(&config);
    return rc;
}