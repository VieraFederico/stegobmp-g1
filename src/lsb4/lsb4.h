#ifndef LSB4_H
#define LSB4_H

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Embeds data into the 4 least significant bits of pixel data using LSB4 steganography
 * 
 * This function embeds the provided data into the 4 least significant bits of each pixel byte,
 * starting from the beginning of the pixel array. Each pixel byte stores 4 bits of data.
 * 
 * @param pixels Pointer to the pixel data array where data will be embedded
 * @param pixels_len Length of the pixel data array in bytes
 * @param data Pointer to the data to be embedded
 * @param data_len Length of the data to be embedded in bytes
 * 
 * @return 0 on success, negative error code on failure:
 *         -1: Insufficient capacity in pixel array
 * 
 * @note The function modifies the pixel data in place
 * @note Requires at least 2 * data_len pixels to embed data_len bytes
 * @note Each data byte requires 2 pixel bytes (4 bits per pixel)
 * @note This method has higher capacity than LSB1 but may be more detectable
 */
int lsb4_embed(uint8_t *pixels, size_t pixels_len, const uint8_t *data, size_t data_len);

/**
 * @brief Extracts data from the 4 least significant bits of pixel data using LSB4 steganography
 * 
 * This function extracts the previously embedded data from the 4 least significant bits
 * of each pixel byte, reconstructing the original data.
 * 
 * @param pixels Pointer to the pixel data array containing embedded data
 * @param pixels_len Length of the pixel data array in bytes
 * @param out Pointer to the output buffer where extracted data will be stored
 * @param data_len Length of the data to be extracted in bytes
 * 
 * @return 0 on success, negative error code on failure
 * 
 * @note The output buffer must be pre-allocated with sufficient space
 * @note Extracts exactly data_len bytes from the pixel data
 * @note Each extracted byte comes from 2 consecutive pixel bytes
 */
int lsb4_extract(const uint8_t *pixels, size_t pixels_len, uint8_t *out, size_t data_len);

#endif // LSB4_H

