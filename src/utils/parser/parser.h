#ifndef PARSER_H
#define PARSER_H

#include <stdbool.h>
#include <stdint.h>

// Enums for type-safe configuration
typedef enum {
    STEG_NONE = 0,
    STEG_LSB1,
    STEG_LSB4,
    STEG_LSBI
} steg_method_t;

typedef enum {
    ENC_NONE = 0,
    ENC_AES128,
    ENC_AES192,
    ENC_AES256,
    ENC_3DES
} encryption_algorithm_t;

typedef enum {
    MODE_NONE = 0,
    MODE_ECB,
    MODE_CFB,
    MODE_OFB,
    MODE_CBC
} encryption_mode_t;

typedef enum {
    OP_NONE = 0,
    OP_EMBED,
    OP_EXTRACT
} operation_t;

// Main configuration TAD
typedef struct {
    // Operation mode
    operation_t operation;
    
    // File paths
    char *in_file;           // Input file to hide (embed only)
    char *carrier_file;      // Carrier BMP file
    char *out_file;          // Output file
    
    // Steganography configuration
    steg_method_t steg_method;
    
    // Encryption configuration (optional)
    encryption_algorithm_t encryption_algo;
    encryption_mode_t encryption_mode;
    char *password;
    
    // Validation and error handling
    bool is_valid;
    char error_message[256];  // Stores validation error messages
} stegobmp_config_t;

// Parser functions
int parse_arguments(int argc, char **argv, stegobmp_config_t *config);
void free_config(stegobmp_config_t *config);
const char* steg_method_to_string(steg_method_t method);
const char* encryption_algo_to_string(encryption_algorithm_t algo);
const char* encryption_mode_to_string(encryption_mode_t mode);

#endif // PARSER_H
