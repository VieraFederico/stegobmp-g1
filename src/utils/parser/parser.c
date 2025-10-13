#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Helper functions to convert strings to enums
static steg_method_t parse_steg_method(const char *str) {
    if (strcmp(str, "LSB1") == 0) return STEG_LSB1;
    if (strcmp(str, "LSB4") == 0) return STEG_LSB4;
    if (strcmp(str, "LSBI") == 0) return STEG_LSBI;
    return STEG_NONE;
}

static encryption_algorithm_t parse_encryption_algo(const char *str) {
    if (strcmp(str, "aes128") == 0) return ENC_AES128;
    if (strcmp(str, "aes192") == 0) return ENC_AES192;
    if (strcmp(str, "aes256") == 0) return ENC_AES256;
    if (strcmp(str, "3des") == 0) return ENC_3DES;
    return ENC_NONE;
}

static encryption_mode_t parse_encryption_mode(const char *str) {
    if (strcmp(str, "ecb") == 0) return MODE_ECB;
    if (strcmp(str, "cfb") == 0) return MODE_CFB;
    if (strcmp(str, "ofb") == 0) return MODE_OFB;
    if (strcmp(str, "cbc") == 0) return MODE_CBC;
    return MODE_NONE;
}

// Validation logic
static int validate_config(stegobmp_config_t *config) {
    // Check: operation must be set
    if (config->operation == OP_NONE) {
        snprintf(config->error_message, sizeof(config->error_message),
                 "Error: Must specify either -embed or -extract");
        return -1;
    }
    
    // Check: carrier and output files are required
    if (!config->carrier_file || !config->out_file) {
        snprintf(config->error_message, sizeof(config->error_message),
                 "Error: -p and -out are required");
        return -2;
    }
    
    // Check: embed mode requires input file
    if (config->operation == OP_EMBED && !config->in_file) {
        snprintf(config->error_message, sizeof(config->error_message),
                 "Error: -embed requires -in parameter");
        return -3;
    }
    
    // Check: steg method must be valid
    if (config->steg_method == STEG_NONE) {
        snprintf(config->error_message, sizeof(config->error_message),
                 "Error: Invalid or missing -steg method (use LSB1, LSB4, or LSBI)");
        return -4;
    }
    
    // Check: if encryption is specified, all params must be present
    bool has_encryption = (config->encryption_algo != ENC_NONE) ||
                          (config->encryption_mode != MODE_NONE) ||
                          (config->password != NULL);
    
    if (has_encryption) {
        if (config->encryption_algo == ENC_NONE) {
            snprintf(config->error_message, sizeof(config->error_message),
                     "Error: Encryption mode/password specified but missing -a algorithm");
            return -5;
        }
        if (config->encryption_mode == MODE_NONE) {
            snprintf(config->error_message, sizeof(config->error_message),
                     "Error: Encryption algorithm/password specified but missing -m mode");
            return -6;
        }
        if (config->password == NULL) {
            snprintf(config->error_message, sizeof(config->error_message),
                     "Error: Encryption algorithm/mode specified but missing -pass password");
            return -7;
        }
    }
    
    config->is_valid = true;
    return 0;
}

// Main parsing function using manual parsing
int parse_arguments(int argc, char **argv, stegobmp_config_t *config) {
    // Initialize config to defaults
    memset(config, 0, sizeof(stegobmp_config_t));
    config->is_valid = false;
    
    // Manual parsing to handle all options as specified
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-embed") == 0) {
            config->operation = OP_EMBED;
        } else if (strcmp(argv[i], "-extract") == 0) {
            config->operation = OP_EXTRACT;
        } else if (strcmp(argv[i], "-in") == 0 && i + 1 < argc) {
            config->in_file = strdup(argv[++i]);
        } else if (strcmp(argv[i], "-p") == 0 && i + 1 < argc) {
            config->carrier_file = strdup(argv[++i]);
        } else if (strcmp(argv[i], "-out") == 0 && i + 1 < argc) {
            config->out_file = strdup(argv[++i]);
        } else if (strcmp(argv[i], "-steg") == 0 && i + 1 < argc) {
            config->steg_method = parse_steg_method(argv[++i]);
        } else if (strcmp(argv[i], "-a") == 0 && i + 1 < argc) {
            config->encryption_algo = parse_encryption_algo(argv[++i]);
        } else if (strcmp(argv[i], "-m") == 0 && i + 1 < argc) {
            config->encryption_mode = parse_encryption_mode(argv[++i]);
        } else if (strcmp(argv[i], "-pass") == 0 && i + 1 < argc) {
            config->password = strdup(argv[++i]);
        } else {
            snprintf(config->error_message, sizeof(config->error_message),
                     "Error: Unknown option '%s'", argv[i]);
            return -1;
        }
    }
    
    // Apply defaults for encryption if password is provided
    if (config->password != NULL) {
        // If password is provided but no algorithm, default to aes128
        if (config->encryption_algo == ENC_NONE) {
            config->encryption_algo = ENC_AES128;
        }
        // If password is provided but no mode, default to CBC
        if (config->encryption_mode == MODE_NONE) {
            config->encryption_mode = MODE_CBC;
        }
    }
    
    // Validate the parsed configuration
    return validate_config(config);
}

// Memory management
void free_config(stegobmp_config_t *config) {
    if (config->in_file) free(config->in_file);
    if (config->carrier_file) free(config->carrier_file);
    if (config->out_file) free(config->out_file);
    if (config->password) free(config->password);
    memset(config, 0, sizeof(stegobmp_config_t));
}

// Utility functions to convert enums back to strings
const char* steg_method_to_string(steg_method_t method) {
    switch (method) {
        case STEG_LSB1: return "LSB1";
        case STEG_LSB4: return "LSB4";
        case STEG_LSBI: return "LSBI";
        default: return "NONE";
    }
}

const char* encryption_algo_to_string(encryption_algorithm_t algo) {
    switch (algo) {
        case ENC_AES128: return "aes128";
        case ENC_AES192: return "aes192";
        case ENC_AES256: return "aes256";
        case ENC_3DES: return "3des";
        default: return "none";
    }
}

const char* encryption_mode_to_string(encryption_mode_t mode) {
    switch (mode) {
        case MODE_ECB: return "ecb";
        case MODE_CFB: return "cfb";
        case MODE_OFB: return "ofb";
        case MODE_CBC: return "cbc";
        default: return "none";
    }
}