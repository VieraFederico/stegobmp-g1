#ifndef LSB4_H
#define LSB4_H

#include <stddef.h>
#include <stdint.h>
#include "../common/bmp_image.h"

/**
 * @brief Embeds data into the 4 least significant bits of pixel data using LSB4 steganography
 * 
 * This function embeds the provided data into the 4 least significant bits of each pixel byte,
 * starting from the offset position. Each pixel byte stores 4 bits of data.
 * 
 * @param bmp Pointer to the BMPImage structure where data will be embedded
 * @param data Pointer to the data to be embedded
 * @param num_bits Number of bits to embed (must be multiple of 4)
 * @param offset Pointer to the starting component index (updated after embedding)
 * 
 * @return 0 on success, negative error code on failure
 * 
 * @note The function modifies the pixel data in place
 * @note Requires at least num_bits / 4 components to embed num_bits bits
 * @note Each data byte requires 2 components (4 bits per component)
 */
int lsb4_embed(BMPImage *bmp, const uint8_t *data, size_t num_bits, size_t *offset);

/**
 * @brief Extracts data from the 4 least significant bits of pixel data using LSB4 steganography
 * 
 * This function extracts the previously embedded data from the 4 least significant bits
 * of each pixel byte, reconstructing the original data.
 * 
 * @param bmp Pointer to the BMPImage structure containing embedded data
 * @param num_bits Number of bits to extract (must be multiple of 4)
 * @param buffer Pointer to the output buffer where extracted data will be stored
 * @param offset Pointer to the starting component index (updated after extraction)
 * @param context Context parameter (unused for LSB4, can be NULL)
 * 
 * @return 0 on success, negative error code on failure
 * 
 * @note The output buffer must be pre-allocated with sufficient space
 * @note Extracts exactly num_bits bits from the pixel data
 * @note Each extracted byte comes from 2 consecutive components
 */
int lsb4_extract(const BMPImage *bmp, size_t num_bits, uint8_t *buffer, size_t *offset, void *context);

#endif // LSB4_H

