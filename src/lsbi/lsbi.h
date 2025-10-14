#ifndef LSBI_H
#define LSBI_H

#include <stddef.h>
#include <stdint.h>

/**
 * @brief Embeds data into pixel data using LSBI (LSB Improved) steganography
 * 
 * This function embeds the provided data using an improved LSB algorithm that
 * selects pixels based on their suitability for hiding data, making detection
 * harder. The algorithm analyzes each pixel and only uses those where embedding
 * will cause minimal perceptible change.
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
 * @note Uses intelligent pixel selection for better steganographic security
 * @note Capacity varies based on image characteristics
 * @note More resistant to statistical analysis than LSB1
 */
int lsbi_embed(uint8_t *pixels, size_t pixels_len, const uint8_t *data, size_t data_len);

/**
 * @brief Extracts data from pixel data using LSBI (LSB Improved) steganography
 * 
 * This function extracts the previously embedded data using the LSBI algorithm,
 * which uses the same pixel selection criteria as the embedding process to
 * reconstruct the original data.
 * 
 * @param pixels Pointer to the pixel data array containing embedded data
 * @param pixels_len Length of the pixel data array in bytes
 * @param out Pointer to the output buffer where extracted data will be stored
 * @param data_len Length of the data to be extracted in bytes
 * 
 * @return 0 on success, negative error code on failure
 * 
 * @note The output buffer must be pre-allocated with sufficient space
 * @note Must use the same pixel selection algorithm as embedding
 * @note Extracts exactly data_len bytes from the pixel data
 */
int lsbi_extract(const uint8_t *pixels, size_t pixels_len, uint8_t *out, size_t data_len);

#endif // LSBI_H

