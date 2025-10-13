#ifndef TRANSLATOR_H
#define TRANSLATOR_H

#include <stdint.h>

/**
 * @brief Converts a 32-bit unsigned integer to big-endian byte array
 * 
 * This function takes a 32-bit unsigned integer and stores it as 4 bytes
 * in big-endian format (most significant byte first).
 * 
 * @param v The 32-bit value to convert
 * @param out Array of 4 bytes where the big-endian representation will be stored
 * 
 * @note Big-endian format: MSB is stored at index 0, LSB at index 3
 * @note Example: 0x12345678 becomes {0x12, 0x34, 0x56, 0x78}
 */
void u32_to_be(uint32_t v, uint8_t out[4]);

/**
 * @brief Converts a big-endian byte array to 32-bit unsigned integer
 * 
 * This function takes 4 bytes in big-endian format and reconstructs
 * the original 32-bit unsigned integer.
 * 
 * @param in Array of 4 bytes in big-endian format
 * @return The reconstructed 32-bit unsigned integer
 * 
 * @note Big-endian format: MSB is at index 0, LSB at index 3
 * @note Example: {0x12, 0x34, 0x56, 0x78} becomes 0x12345678
 */
uint32_t be_to_u32(const uint8_t in[4]);

#endif // TRANSLATOR_H
