#ifndef LSBI_H
#define LSBI_H

#include <stdint.h>
#include <stddef.h>
#include "../common/bmp_image.h"

#define PATTERN_MAP_SIZE 4

/**
 * @brief Embeds data into BMP pixels using LSBI (LSB with bit inversion)
 * 
 * LSBI Algorithm (from paper):
 * 1. Uses only Green and Blue channels (Red acts as noise for security)
 * 2. Applies standard LSB first
 * 3. Classifies pixels by 2nd and 3rd LSB patterns (00, 01, 10, 11)
 * 4. Inverts LSB if more pixels changed than unchanged in each pattern
 * 5. Stores pattern inversion map in first 4 components using LSB1 (all components)
 * 
 * @param bmp Pointer to BMPImage structure where data will be embedded
 * @param data Pointer to data to embed
 * @param num_bits Number of bits to embed
 * @param offset Pointer to the starting component index (updated after embedding)
 * 
 * @return 0 on success, -1 on failure
 * 
 * @note Requires at least 4 components for storing the pattern map
 * @note Capacity: (width * height * 2 - 4) bits (uses 2 channels, reserves 4 components)
 */
int lsbi_embed(BMPImage *bmp, const uint8_t *data, size_t num_bits, size_t *offset);

/**
 * @brief Extracts data from BMP pixels using LSBI
 * 
 * LSBI Extraction Algorithm:
 * 1. Pattern map must be read BEFORE calling this function using lsb1_extract
 * 2. Re-inverts LSBs of patterns that were marked as inverted
 * 3. Extracts bits from Green and Blue channels normally
 * 
 * @param bmp Pointer to BMPImage structure containing embedded data
 * @param num_bits Number of bits to extract
 * @param buffer Output buffer for extracted data
 * @param offset Pointer to the starting component index (updated after extraction)
 * @param context Pointer to pattern_map (uint8_t*) - REQUIRED for LSBI
 * 
 * @return 0 on success, -1 on failure
 * 
 * @note The pattern_map must be read using lsb1_extract before calling this function
 * @note The pattern_map is passed as context and should be a pointer to uint8_t
 */
int lsbi_extract(const BMPImage *bmp, size_t num_bits, uint8_t *buffer, size_t *offset, void *context);

#endif // LSBI_H