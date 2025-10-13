#ifndef FILE_MANAGEMENT_H
#define FILE_MANAGEMENT_H

#include <stdint.h>
#include <stddef.h>

/**
 * @brief Reads the entire contents of a file into memory
 * 
 * This function allocates memory and reads the complete file content into a buffer.
 * The caller is responsible for freeing the allocated memory using free().
 * 
 * @param path Path to the file to read
 * @param buf Pointer to a uint8_t* where the file content will be stored
 * @param len Pointer to a size_t where the file size will be stored
 * 
 * @return 0 on success, negative error code on failure:
 *         -1: Failed to open file
 *         -2: Failed to seek to end of file
 *         -3: Failed to get file size
 *         -4: Failed to seek to beginning of file
 *         -5: Memory allocation failed
 *         -6: Failed to read file content
 * 
 * @note The function opens the file in binary mode ("rb")
 * @note On error, *buf will be NULL and *len will be 0
 * @note On success, *buf points to allocated memory that must be freed by caller
 */
int read_file(const char *path, uint8_t **buf, size_t *len);

/**
 * @brief Writes data to a file
 * 
 * This function writes the provided buffer to a file, overwriting any existing content.
 * 
 * @param path Path to the file to write
 * @param buf Pointer to the data buffer to write
 * @param len Number of bytes to write
 * 
 * @return 0 on success, negative error code on failure:
 *         -1: Failed to open file for writing
 *         -2: Failed to write data to file
 * 
 * @note The function opens the file in binary write mode ("wb")
 * @note If the file exists, it will be truncated to the new content
 */
int write_file(const char *path, const uint8_t *buf, size_t len);

#endif // FILE_MANAGEMENT_H
