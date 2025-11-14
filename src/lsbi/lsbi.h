#ifndef LSBI_H
#define LSBI_H

#include <stdint.h>
#include <stddef.h>

/**
 * @brief Embeds data into BMP pixels using LSBI (LSB with bit inversion)
 * 
 * LSBI Algorithm (from paper):
 * 1. Uses only Green and Blue channels (Red acts as noise for security)
 * 2. Applies standard LSB first
 * 3. Classifies pixels by 2nd and 3rd LSB patterns (00, 01, 10, 11)
 * 4. Inverts LSB if more pixels changed than unchanged in each pattern
 * 5. Stores pattern inversion map in first 4 pixels (Red channel)
 * 
 * @param pixels Pointer to BMP pixel data (BGR format, 3 bytes per pixel)
 * @param pixels_len Total size of pixel data in bytes
 * @param data Pointer to data to embed
 * @param data_len Size of data to embed in bytes
 * 
 * @return 0 on success, -1 on failure
 * 
 * @note Requires at least 4 pixels for storing the pattern map
 * @note Capacity: (pixels_len / 3 - 4) / 4 bytes (uses 2 channels, reserves 4 pixels)
 */
int lsbi_embed(uint8_t *pixels, size_t pixels_len, const uint8_t *data, size_t data_len);

/**
 * @brief Extracts data from BMP pixels using LSBI
 * 
 * LSBI Extraction Algorithm:
 * 1. Reads pattern inversion map from first 4 pixels
 * 2. Re-inverts LSBs of patterns that were marked as inverted
 * 3. Extracts bits from Green and Blue channels normally
 * 
 * @param pixels Pointer to BMP pixel data (BGR format)
 * @param pixels_len Total size of pixel data in bytes
 * @param out Output buffer for extracted data
 * @param data_len Number of bytes to extract
 * 
 * @return 0 on success, -1 on failure
 */
int lsbi_extract(const uint8_t *pixels, size_t pixels_len, uint8_t *out, size_t data_len);

#endif // LSBI_H