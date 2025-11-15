#ifndef LSB1_H
#define LSB1_H

#include <stddef.h>
#include <stdint.h>
#include "../common/bmp_image.h"

/**
 * @brief Embeds data into the least significant bits of pixel data using LSB1 steganography
 * 
 * This function embeds the provided data into the least significant bit of each pixel byte,
 * starting from the offset position. Each data bit is stored in one pixel byte.
 * 
 * @param bmp Pointer to the BMPImage structure where data will be embedded
 * @param data Pointer to the data to be embedded
 * @param num_bits Number of bits to embed
 * @param offset Pointer to the starting component index (updated after embedding)
 * 
 * @return 0 on success, negative error code on failure
 * 
 * @note The function modifies the pixel data in place
 * @note Requires at least num_bits components to embed num_bits bits
 * @note Each data bit requires 1 component (1 bit per component)
 */
int lsb1_embed(BMPImage *bmp, const uint8_t *data, size_t num_bits, size_t *offset);

/**
 * @brief Extracts data from the least significant bits of pixel data using LSB1 steganography
 * 
 * This function extracts the previously embedded data from the least significant bit
 * of each pixel byte, reconstructing the original data.
 * 
 * @param bmp Pointer to the BMPImage structure containing embedded data
 * @param num_bits Number of bits to extract
 * @param buffer Pointer to the output buffer where extracted data will be stored
 * @param offset Pointer to the starting component index (updated after extraction)
 * 
 * @return 0 on success, negative error code on failure
 * 
 * @note The output buffer must be pre-allocated with sufficient space
 * @note Extracts exactly num_bits bits from the pixel data
 * @note Each extracted bit comes from 1 consecutive component
 */
int lsb1_extract(const BMPImage *bmp, size_t num_bits, uint8_t *buffer, size_t *offset);

#endif // LSB1_H
