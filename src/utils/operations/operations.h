#ifndef OPERATIONS_H
#define OPERATIONS_H

#include "../../bmp_handler/bmp_handler.h"
#include "../parser/parser.h"

typedef int (*embed_func_t)(uint8_t *pixels, size_t pixels_len, const uint8_t *data, size_t data_len);
typedef int (*extract_func_t)(const uint8_t *pixels, size_t pixels_len, uint8_t *out, size_t data_len);

typedef enum {
    OPS_OK = 0,
    OPS_INVALID_STEG_METHOD,
    OPS_INPUT_READ_FAILED,
    OPS_PAYLOAD_ALLOC_FAILED,
    OPS_CAPACITY_INSUFFICIENT,
    OPS_EMBED_FAILED,
    OPS_BMP_WRITE_FAILED,

    OPS_EXTRACT_SIZE_FAILED,
    OPS_EXTRACT_ALLOC_FAILED,
    OPS_EXTRACT_BLOCK_FAILED,
    OPS_EXTENSION_NOT_FOUND,
    OPS_OUTPUT_WRITE_FAILED
} OperationsResult;

/**
 * @brief Performs the embed operation
 * 
 * This function reads the input file, builds the payload (size + data + extension),
 * checks capacity, embeds the data using the specified steganography method,
 * and writes the output BMP file.
 * 
 * @param config Pointer to the configuration structure containing operation parameters
 * @param bmp Pointer to the BMP structure to embed data into
 * 
 * @return OperationsResult code indicating success or specific failure
 * 
 * @note The BMP pixel data is modified in place
 * @note Supports LSB1, LSB4, and LSBI steganography methods
 */
OperationsResult perform_embed(const stegobmp_config_t *config, const Bmp *bmp);

/**
 * @brief Performs the extract operation
 * 
 * This function extracts the hidden data from a BMP file using the specified
 * steganography method, recovers the file extension, and writes the extracted
 * data to the output file.
 * 
 * @param config Pointer to the configuration structure containing operation parameters
 * @param bmp Pointer to the BMP structure containing hidden data
 * 
 * @return OperationsResult code indicating success or specific failure
 * 
 * @note Supports LSB1, LSB4, and LSBI steganography methods
 */
OperationsResult perform_extract(const stegobmp_config_t *config, const Bmp *bmp);

#endif // OPERATIONS_H

