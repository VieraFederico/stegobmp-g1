#ifndef ENCRYPTION_MANAGER_H
#define ENCRYPTION_MANAGER_H

#include "../utils/parser/parser.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Encrypts data using the specified configuration
 * @return 0 on success, -1 on error
 */
int encrypt_data(const stegobmp_config_t *config,
                 const uint8_t *plaintext, size_t plaintext_len,
                 uint8_t **ciphertext, size_t *ciphertext_len); 

/**
 * @brief Decrypts data using the specified configuration
 * @return 0 on success, -1 on error
 */
int decrypt_data(const stegobmp_config_t *config,
                 const uint8_t *ciphertext, size_t ciphertext_len,
                 uint8_t **plaintext, size_t *plaintext_len); 

/**
 * @brief Checks if encryption is enabled in the configuration
 */
bool is_encryption_enabled(const stegobmp_config_t *config);

/**
 * @brief Gets a human-readable description of the encryption configuration
 * @param config Pointer to the configuration structure
 * @param buffer Buffer to store the description string
 * @param buffer_size Size of the buffer
 * @return Pointer to the buffer (or NULL on error)
 */
char* get_encryption_description(const stegobmp_config_t *config, char *buffer, size_t buffer_size);

#endif /* ENCRYPTION_MANAGER_H */