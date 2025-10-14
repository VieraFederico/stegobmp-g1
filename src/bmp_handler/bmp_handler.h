#ifndef BMP_HANDLER_H
#define BMP_HANDLER_H

#include <stdint.h>
#include <stddef.h>

/**
 * @file bmp_handler.h
 * @brief BMP file format handling structures and functions
 * 
 * This module provides functionality to read, write, and manipulate BMP (Bitmap) files.
 * It supports 24-bit BMP files with bottom-up pixel ordering.
 */

// Force 1-byte packing for BMP structures
#pragma pack(push, 1)

/**
 * @brief BMP file header structure (14 bytes)
 * 
 * Contains the basic file information for a BMP file.
 * All fields are stored in little-endian format.
 */
typedef struct {
    uint16_t bfType;      /**< File type, must be 'BM' (0x4D42 in little-endian) */
    uint32_t bfSize;      /**< Size of the BMP file in bytes */
    uint16_t bfReserved1; /**< Reserved field, must be 0 */
    uint16_t bfReserved2; /**< Reserved field, must be 0 */
    uint32_t bfOffBits;   /**< Offset to start of pixel data (typically 54 for 24-bit BMP) */
} BITMAPFILEHEADER;

/**
 * @brief BMP info header structure (40 bytes)
 * 
 * Contains detailed information about the bitmap image.
 * This is the standard DIB (Device Independent Bitmap) header.
 */
typedef struct {
    uint32_t biSize;          /**< Size of this header in bytes (40) */
    int32_t  biWidth;         /**< Image width in pixels */
    int32_t  biHeight;        /**< Image height in pixels (positive = bottom-up) */
    uint16_t biPlanes;        /**< Number of color planes (must be 1) */
    uint16_t biBitCount;      /**< Bits per pixel (24 for 24-bit BMP) */
    uint32_t biCompression;   /**< Compression type (0 = BI_RGB, uncompressed) */
    uint32_t biSizeImage;     /**< Size of image data in bytes (width * height * 3 + padding) */
    int32_t  biXPelsPerMeter; /**< Horizontal resolution in pixels per meter */
    int32_t  biYPelsPerMeter; /**< Vertical resolution in pixels per meter */
    uint32_t biClrUsed;       /**< Number of colors in color palette (0 for 24-bit) */
    uint32_t biClrImportant;  /**< Number of important colors (0 = all important) */
} BITMAPINFOHEADER;

#pragma pack(pop)

/**
 * @brief Complete BMP structure containing headers and pixel data
 * 
 * This structure holds all the information needed to represent a BMP file in memory,
 * including the file header, info header, and raw pixel data.
 */
typedef struct {
    BITMAPFILEHEADER fileHeader; /**< BMP file header */
    BITMAPINFOHEADER infoHeader; /**< BMP info header */
    uint8_t *pixels;             /**< Raw pixel data buffer (BGR format with row padding) */
    size_t   pixelsSize;         /**< Size of pixel data in bytes (from bfOffBits to EOF) */
} Bmp;

/**
 * @brief Reads a BMP file from disk into memory
 * 
 * This function loads a BMP file from the specified path and parses it into
 * the Bmp structure. The function allocates memory for the pixel data.
 * 
 * @param path Path to the BMP file to read
 * @param out Pointer to Bmp structure where the loaded data will be stored
 * 
 * @return 0 on success, negative error code on failure:
 *         -1: Failed to open file
 *         -2: Invalid BMP file format
 *         -3: Unsupported BMP format (only 24-bit supported)
 *         -4: Memory allocation failed
 * 
 * @note The caller is responsible for calling bmp_free() to release memory
 * @note Only 24-bit uncompressed BMP files are supported
 * @note Pixel data is stored in BGR format with row padding
 */
int bmp_read(const char *path, Bmp *out);

/**
 * @brief Writes a BMP structure to a file on disk
 * 
 * This function saves a Bmp structure to the specified file path.
 * The file will be created or overwritten.
 * 
 * @param path Path where the BMP file will be written
 * @param bmp Pointer to the Bmp structure to write
 * 
 * @return 0 on success, negative error code on failure:
 *         -1: Failed to create/open file for writing
 *         -2: Failed to write file header
 *         -3: Failed to write info header
 *         -4: Failed to write pixel data
 * 
 * @note The function overwrites existing files
 * @note All data is written in little-endian format
 */
int bmp_write(const char *path, const Bmp *bmp);

/**
 * @brief Frees memory allocated for a BMP structure
 * 
 * This function releases all memory allocated for the Bmp structure,
 * including the pixel data buffer.
 * 
 * @param bmp Pointer to the Bmp structure to free
 * 
 * @note This function sets the pixels pointer to NULL and pixelsSize to 0
 * @note Safe to call multiple times on the same structure
 * @note Should be called when the BMP is no longer needed
 */
void bmp_free(Bmp *bmp);

/**
 * @brief Calculates the maximum payload size for steganography
 * 
 * This function calculates how many bytes can be embedded in the BMP
 * using LSB steganography techniques.
 * 
 * @param bmp Pointer to the Bmp structure
 * 
 * @return Maximum number of bytes that can be embedded
 * 
 * @note The calculation is based on the pixel data size
 * @note Different steganography methods may have different capacity limits
 * @note This is a theoretical maximum; actual capacity may be less
 */
size_t bmp_payload_size(const Bmp *bmp);

#endif // BMP_HANDLER_H
