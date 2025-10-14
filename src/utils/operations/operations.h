#ifndef OPERATIONS_H
#define OPERATIONS_H

#include "../../bmp_handler/bmp_handler.h"
#include "../parser/parser.h"

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
 * @return 0 on success, 1 on failure
 * 
 * @note The BMP pixel data is modified in place
 * @note Supports LSB1, LSB4, and LSBI steganography methods
 */
int perform_embed(const stegobmp_config_t *config, const Bmp *bmp);

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
 * @return 0 on success, 1 on failure
 * 
 * @note Supports LSB1, LSB4, and LSBI steganography methods
 */
int perform_extract(const stegobmp_config_t *config, const Bmp *bmp);

#endif // OPERATIONS_H

