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

OperationsResult perform_embed(const stegobmp_config_t *config, const Bmp *bmp)
{
    uint8_t *input_buffer = NULL;
    size_t input_length = 0;
    
    // Read input file to embed
    if (read_file(config->in_file, &input_buffer, &input_length) != 0)
    {
        fprintf(stderr, "No pude leer -in\n");
        return OPS_INPUT_READ_FAILED;
    }

    // Build payload: size(4 BE) || data || extension(".ext\0")
    const char *extension_dot_ptr = strrchr(config->in_file, '.');
    char extension_buffer[64];

    if (extension_dot_ptr)
        snprintf(extension_buffer, sizeof(extension_buffer), "%s", extension_dot_ptr);
    else
        snprintf(extension_buffer, sizeof(extension_buffer), ".bin");

    size_t extension_length = strlen(extension_buffer) + 1; // include '\0'

    size_t payload_length = 4 + input_length + extension_length;
    uint8_t *payload_buffer = (uint8_t *)malloc(payload_length);

    if (!payload_buffer)
    {
        free(input_buffer);
        return OPS_PAYLOAD_ALLOC_FAILED;
    }

    u32_to_be((uint32_t)input_length, payload_buffer);
    memcpy(payload_buffer + 4, input_buffer, input_length);
    memcpy(payload_buffer + 4 + input_length, extension_buffer, extension_length);

    //Enc
    

    embed_func_t selected_embed_function = NULL;
    const char *steg_method_name = "";
    size_t pixel_bytes_per_data_byte = 0;

    switch (config->steg_method)
    {
        case STEG_LSB1:
            selected_embed_function = lsb1_embed;
            pixel_bytes_per_data_byte = 8;
            steg_method_name = "LSB1";
            break;
        case STEG_LSB4:
            selected_embed_function = lsb4_embed;
            pixel_bytes_per_data_byte = 2; 
            steg_method_name = "LSB4";
            break;
        case STEG_LSBI:
            selected_embed_function = lsbi_embed;
            pixel_bytes_per_data_byte = 8;
            steg_method_name = "LSBI";
            break;
        default:
            fprintf(stderr, "Invalid Steg Method: %d\n", config->steg_method);
            free(payload_buffer);
            free(input_buffer);
            return OPS_INVALID_STEG_METHOD;
            break;
    }

    size_t capacity_bytes = bmp_payload_size(bmp) / pixel_bytes_per_data_byte;
    
    if (payload_length > capacity_bytes)
    {
        fprintf(stderr, "Capacidad insuficiente. Max (%s): %zu bytes\n", steg_method_name, capacity_bytes);
        free(payload_buffer);
        free(input_buffer);
        return OPS_CAPACITY_INSUFFICIENT;
    }

 

    if (selected_embed_function(bmp->pixels, bmp->pixelsSize, payload_buffer, payload_length) != 0)
    {
        fprintf(stderr, "Fallo embed %s\n",steg_method_name);
        free(payload_buffer);
        free(input_buffer);
        return OPS_EMBED_FAILED;
    }
    

    // Write output BMP file
    if (bmp_write(config->out_file, bmp) != 0)
    {
        fprintf(stderr, "No pude escribir BMP de salida\n");
        free(payload_buffer);
        free(input_buffer);
        return OPS_BMP_WRITE_FAILED;
    }

    free(payload_buffer);
    free(input_buffer);
    printf("OK: incrustado %zu bytes (incluye tamaño y extension)\n", payload_length);
    return OPS_OK;
}

OperationsResult perform_extract(const stegobmp_config_t *config, const Bmp *bmp)
{
    extract_func_t selected_extract_function = NULL;
    const char *steg_method_name  = "";

    switch (config->steg_method){
        case STEG_LSB1:
            selected_extract_function = lsb1_extract;
            steg_method_name = "LSB1";
            break;
        case STEG_LSB4:
            selected_extract_function = lsb4_extract;
            steg_method_name = "LSB4";
            break;
        case STEG_LSBI:
            selected_extract_function = lsbi_extract;
            steg_method_name = "LSBI";
            break;
        default:
            fprintf(stderr, "Invalid Steg Method");
            return OPS_INVALID_STEG_METHOD;
    }
    
    uint8_t big_endian_size_header[4];
    if (selected_extract_function(bmp->pixels, bmp->pixelsSize, big_endian_size_header, 4) != 0)
    {
        fprintf(stderr,"Fallo extract size %s\n", steg_method_name);
        return OPS_EXTRACT_SIZE_FAILED;
    }

    uint32_t real_data_size = be_to_u32(big_endian_size_header);
    size_t bytes_remaining_to_read = real_data_size + 16; // margin for extension (".algo\0")

    uint8_t *temp_buffer = (uint8_t *)malloc(bytes_remaining_to_read);
    if (!temp_buffer) { return OPS_EXTRACT_ALLOC_FAILED; }

    uint8_t *full_block_buffer = (uint8_t *)malloc(4 + bytes_remaining_to_read);
    if (!full_block_buffer){ free(temp_buffer); return OPS_EXTRACT_ALLOC_FAILED; }

    //DEC

    if (selected_extract_function(bmp->pixels, bmp->pixelsSize, full_block_buffer, 4 + bytes_remaining_to_read) != 0){
        fprintf(stderr, "Fallo extract bloque\n");
        free(full_block_buffer);
        free(temp_buffer);
        return OPS_EXTRACT_BLOCK_FAILED;
    }

    // full_block_buffer: [size(4)] [data ...] [extension...]
    uint8_t *data_section_ptr = full_block_buffer + 4;
    
    // Find '\0' of extension after the data
    const char *extension_string_ptr = (const char *)(data_section_ptr + real_data_size);
    size_t extension_length_found = strnlen(extension_string_ptr, bytes_remaining_to_read - real_data_size);

    if (extension_length_found == bytes_remaining_to_read - real_data_size)
    {
        fprintf(stderr, "No encontre terminador de extension\n");
        free(full_block_buffer);
        free(temp_buffer);
        return OPS_EXTENSION_NOT_FOUND;
    }
    
    // Write out_file (data only)
    if (write_file(config->out_file, data_section_ptr, real_data_size) != 0)
    {
        fprintf(stderr, "No pude escribir archivo de salida\n");
        free(full_block_buffer);
        free(temp_buffer);
        return OPS_OUTPUT_WRITE_FAILED;
    }
    
    printf("OK: extraidos %u bytes, extension recuperada: %s\n", real_data_size, extension_string_ptr);
    free(full_block_buffer);
    free(temp_buffer);
    return OPS_OK;
}