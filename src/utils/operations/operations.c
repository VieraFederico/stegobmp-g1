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
#include "../encryption_manager/encryption_manager.h"
#include "operations.h"

OperationsResult perform_embed(const stegobmp_config_t *config, const Bmp *bmp)
{
    uint8_t *input_buffer = NULL;
    size_t input_length = 0;
    
    // Read input file to embed
    if (read_file(config->in_file, &input_buffer, &input_length) != 0)
    {
        fprintf(stderr, "Error: No pude leer archivo de entrada '%s'\n", config->in_file);
        return OPS_INPUT_READ_FAILED;
    }

    // Extract extension from input filename
    const char *extension_dot_ptr = strrchr(config->in_file, '.');
    char extension_buffer[64];

    if (extension_dot_ptr)
        snprintf(extension_buffer, sizeof(extension_buffer), "%s", extension_dot_ptr);
    else
        snprintf(extension_buffer, sizeof(extension_buffer), ".bin");

    size_t extension_length = strlen(extension_buffer) + 1; // include '\0'

    // Build unencrypted payload: tamaño real (4 BE) || datos archivo || extensión
    size_t unencrypted_payload_length = 4 + input_length + extension_length;
    uint8_t *unencrypted_payload = (uint8_t *)malloc(unencrypted_payload_length);

    if (!unencrypted_payload)
    {
        fprintf(stderr, "Error: No pude asignar memoria para payload\n");
        free(input_buffer);
        return OPS_PAYLOAD_ALLOC_FAILED;
    }

    // Build: size(4 BE) || data || extension
    u32_to_be((uint32_t)input_length, unencrypted_payload);
    memcpy(unencrypted_payload + 4, input_buffer, input_length);
    memcpy(unencrypted_payload + 4 + input_length, extension_buffer, extension_length);

    uint8_t *final_payload = NULL;
    size_t final_payload_length = 0;
    bool needs_free_final = false;

    // Check if encryption is enabled
    if (is_encryption_enabled(config))
    {
        // Encrypt: tamaño real || datos archivo || extensión
        uint8_t *encrypted_data = NULL;
        size_t encrypted_length = 0;

        printf("Encriptando con ");
        char enc_desc[64];
        printf("%s...\n", get_encryption_description(config, enc_desc, sizeof(enc_desc)));

        if (encrypt_data(config, unencrypted_payload, unencrypted_payload_length,
                        &encrypted_data, &encrypted_length) != 0)
        {
            fprintf(stderr, "Error: Fallo la encriptacion\n");
            free(unencrypted_payload);
            free(input_buffer);
            return OPS_ENCRYPTION_FAILED;
        }

        // Build final payload: tamaño cifrado (4 BE) || datos cifrados
        final_payload_length = 4 + encrypted_length;
        final_payload = (uint8_t *)malloc(final_payload_length);
        
        if (!final_payload)
        {
            fprintf(stderr, "Error: No pude asignar memoria para payload final\n");
            free(encrypted_data);
            free(unencrypted_payload);
            free(input_buffer);
            return OPS_PAYLOAD_ALLOC_FAILED;
        }

        u32_to_be((uint32_t)encrypted_length, final_payload);
        memcpy(final_payload + 4, encrypted_data, encrypted_length);

        free(encrypted_data);
        needs_free_final = true;
        
        printf("Payload: %zu bytes -> %zu bytes encriptados (con padding)\n", 
               unencrypted_payload_length, encrypted_length);
    }
    else
    {
        // No encryption: use unencrypted payload directly
        final_payload = unencrypted_payload;
        final_payload_length = unencrypted_payload_length;
        needs_free_final = false;
    }

    // Select steganography method
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
            fprintf(stderr, "Error: Metodo de esteganografia invalido: %d\n", config->steg_method);
            if (needs_free_final) free(final_payload);
            free(unencrypted_payload);
            free(input_buffer);
            return OPS_INVALID_STEG_METHOD;
    }

    // Check capacity
    size_t capacity_bytes = bmp->pixelsSize / pixel_bytes_per_data_byte;
    
    if (final_payload_length > capacity_bytes)
    {
        fprintf(stderr, "Error: Capacidad insuficiente en BMP.\n");
        fprintf(stderr, "       Necesitas: %zu bytes\n", final_payload_length);
        fprintf(stderr, "       Capacidad maxima (%s): %zu bytes\n", steg_method_name, capacity_bytes);
        if (needs_free_final) free(final_payload);
        free(unencrypted_payload);
        free(input_buffer);
        return OPS_CAPACITY_INSUFFICIENT;
    }

    // Perform embedding
    printf("Incrustando con %s...\n", steg_method_name);
    if (selected_embed_function(bmp->pixels, bmp->pixelsSize, final_payload, final_payload_length) != 0)
    {
        fprintf(stderr, "Error: Fallo embed %s\n", steg_method_name);
        if (needs_free_final) free(final_payload);
        free(unencrypted_payload);
        free(input_buffer);
        return OPS_EMBED_FAILED;
    }
    
    // Write output BMP file
    if (bmp_write(config->out_file, bmp) != 0)
    {
        fprintf(stderr, "Error: No pude escribir BMP de salida '%s'\n", config->out_file);
        if (needs_free_final) free(final_payload);
        free(unencrypted_payload);
        free(input_buffer);
        return OPS_BMP_WRITE_FAILED;
    }

    // Cleanup
    if (needs_free_final) free(final_payload);
    free(unencrypted_payload);
    free(input_buffer);
    
    // Success message
    if (is_encryption_enabled(config))
    {
        printf("\n=== EXITO ===\n");
        printf("Archivo: '%s' (%zu bytes)\n", config->in_file, input_length);
        printf("Encriptado y oculto en: '%s'\n", config->out_file);
        printf("Extension: %s\n", extension_buffer);
        printf("Metodo: %s\n", steg_method_name);
        char enc_desc[64];
        printf("Encriptacion: %s\n", get_encryption_description(config, enc_desc, sizeof(enc_desc)));
        printf("Total incrustado: %zu bytes\n", final_payload_length);
    }
    else
    {
        printf("\n=== EXITO ===\n");
        printf("Archivo: '%s' (%zu bytes)\n", config->in_file, input_length);
        printf("Oculto en: '%s'\n", config->out_file);
        printf("Extension: %s\n", extension_buffer);
        printf("Metodo: %s\n", steg_method_name);
        printf("Total incrustado: %zu bytes (incluye tamaño y extension)\n", final_payload_length);
    }
    
    return OPS_OK;
}

OperationsResult perform_extract(const stegobmp_config_t *config, const Bmp *bmp)
{
    // Select extraction method
    extract_func_t selected_extract_function = NULL;
    const char *steg_method_name = "";

    switch (config->steg_method)
    {
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
            fprintf(stderr, "Error: Metodo de esteganografia invalido\n");
            return OPS_INVALID_STEG_METHOD;
    }
    
    printf("Extrayendo con %s...\n", steg_method_name);
    
    // Extract size header (first 4 bytes)
    uint8_t big_endian_size_header[4];
    if (selected_extract_function(bmp->pixels, bmp->pixelsSize, big_endian_size_header, 4) != 0)
    {
        fprintf(stderr, "Error: Fallo al extraer cabecera de tamaño con %s\n", steg_method_name);
        return OPS_EXTRACT_SIZE_FAILED;
    }

    uint32_t data_size = be_to_u32(big_endian_size_header);
    
    printf("Tamaño del bloque: %u bytes\n", data_size);
    
    // Extract the full data block (size + data)
    uint8_t *full_block_buffer = (uint8_t *)malloc(4 + data_size);
    if (!full_block_buffer)
    {
        fprintf(stderr, "Error: No pude asignar memoria para extraccion\n");
        return OPS_EXTRACT_ALLOC_FAILED;
    }

    if (selected_extract_function(bmp->pixels, bmp->pixelsSize, full_block_buffer, 4 + data_size) != 0)
    {
        fprintf(stderr, "Error: Fallo al extraer bloque de datos\n");
        free(full_block_buffer);
        return OPS_EXTRACT_BLOCK_FAILED;
    }

    // full_block_buffer: [size(4)] [data ...]
    uint8_t *extracted_data = full_block_buffer + 4;
    
    // Check if decryption is needed
    if (is_encryption_enabled(config))
    {
        printf("Desencriptando con ");
        char enc_desc[64];
        printf("%s...\n", get_encryption_description(config, enc_desc, sizeof(enc_desc)));
        
        // Decrypt the data
        uint8_t *decrypted_data = NULL;
        size_t decrypted_length = 0;

        if (decrypt_data(config, extracted_data, data_size,
                        &decrypted_data, &decrypted_length) != 0)
        {
            fprintf(stderr, "Error: Fallo la desencriptacion\n");
            fprintf(stderr, "       (Verifica la password y los parametros)\n");
            free(full_block_buffer);
            return OPS_DECRYPTION_FAILED;
        }

        printf("Desencriptado: %zu bytes\n", decrypted_length);

        // Parse decrypted data: tamaño real (4 BE) || datos archivo || extensión
        if (decrypted_length < 4)
        {
            fprintf(stderr, "Error: Datos desencriptados demasiado cortos\n");
            free(decrypted_data);
            free(full_block_buffer);
            return OPS_DECRYPTION_FAILED;
        }

        uint32_t real_data_size = be_to_u32(decrypted_data);
        
        if (decrypted_length < 4 + real_data_size)
        {
            fprintf(stderr, "Error: Datos desencriptados incompletos\n");
            fprintf(stderr, "       Esperaba: %u bytes, tengo: %zu bytes\n", 
                    4 + real_data_size, decrypted_length);
            free(decrypted_data);
            free(full_block_buffer);
            return OPS_DECRYPTION_FAILED;
        }

        uint8_t *file_data = decrypted_data + 4;
        const char *extension_string_ptr = (const char *)(file_data + real_data_size);
        
        // Find extension terminator
        size_t max_ext_search = decrypted_length - 4 - real_data_size;
        size_t extension_length = strnlen(extension_string_ptr, max_ext_search);

        if (extension_length == max_ext_search)
        {
            fprintf(stderr, "Error: No encontre terminador de extension\n");
            free(decrypted_data);
            free(full_block_buffer);
            return OPS_EXTENSION_NOT_FOUND;
        }

        // Write output file (data only, without size or extension)
        if (write_file(config->out_file, file_data, real_data_size) != 0)
        {
            fprintf(stderr, "Error: No pude escribir archivo de salida '%s'\n", config->out_file);
            free(decrypted_data);
            free(full_block_buffer);
            return OPS_OUTPUT_WRITE_FAILED;
        }

        // Success message
        printf("\n=== EXITO ===\n");
        printf("Archivo extraido: '%s' (%u bytes)\n", config->out_file, real_data_size);
        printf("Extension recuperada: %s\n", extension_string_ptr);
        printf("Metodo: %s\n", steg_method_name);
        printf("Desencriptacion: %s\n", get_encryption_description(config, enc_desc, sizeof(enc_desc)));
        
        free(decrypted_data);
    }
    else
    {
        // No encryption - parse directly: tamaño real || datos archivo || extensión
        if (data_size < 4)
        {
            fprintf(stderr, "Error: Datos extraidos demasiado cortos\n");
            free(full_block_buffer);
            return OPS_EXTRACT_BLOCK_FAILED;
        }

        uint32_t real_data_size = be_to_u32(extracted_data);
        
        if (data_size < 4 + real_data_size)
        {
            fprintf(stderr, "Error: Datos extraidos incompletos\n");
            free(full_block_buffer);
            return OPS_EXTRACT_BLOCK_FAILED;
        }

        uint8_t *file_data = extracted_data + 4;
        const char *extension_string_ptr = (const char *)(file_data + real_data_size);
        
        // Find extension terminator
        size_t max_ext_search = data_size - 4 - real_data_size;
        size_t extension_length = strnlen(extension_string_ptr, max_ext_search);

        if (extension_length == max_ext_search)
        {
            fprintf(stderr, "Error: No encontre terminador de extension\n");
            free(full_block_buffer);
            return OPS_EXTENSION_NOT_FOUND;
        }

        // Write output file (data only)
        if (write_file(config->out_file, file_data, real_data_size) != 0)
        {
            fprintf(stderr, "Error: No pude escribir archivo de salida '%s'\n", config->out_file);
            free(full_block_buffer);
            return OPS_OUTPUT_WRITE_FAILED;
        }

        // Success message
        printf("\n=== EXITO ===\n");
        printf("Archivo extraido: '%s' (%u bytes)\n", config->out_file, real_data_size);
        printf("Extension recuperada: %s\n", extension_string_ptr);
        printf("Metodo: %s\n", steg_method_name);
    }

    free(full_block_buffer);
    return OPS_OK;
}