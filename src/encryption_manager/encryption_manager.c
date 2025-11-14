#include "encryption_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <openssl/evp.h>
#include <openssl/err.h>

// Fixed salt as per specification: 0x0000000000000000
static const unsigned char FIXED_SALT[8] = {0, 0, 0, 0, 0, 0, 0, 0};
static const int PBKDF2_ITERATIONS = 10000;

/**
 * @brief Maps parser enum to OpenSSL EVP_CIPHER based on algorithm and mode
 */
static const EVP_CIPHER* get_cipher(encryption_algorithm_t algo, encryption_mode_t mode)
{
    switch (algo) {
        case ENC_AES128:
            switch (mode) {
                case MODE_ECB: return EVP_aes_128_ecb();
                case MODE_CBC: return EVP_aes_128_cbc();
                case MODE_CFB: return EVP_aes_128_cfb1();  // CFB1 = 1 bit feedback (matches working project)
                case MODE_OFB: return EVP_aes_128_ofb();   // OFB = 128 bits feedback
                default: return NULL;
            }
        case ENC_AES192:
            switch (mode) {
                case MODE_ECB: return EVP_aes_192_ecb();
                case MODE_CBC: return EVP_aes_192_cbc();
                case MODE_CFB: return EVP_aes_192_cfb1();  // CFB1 = 1 bit feedback
                case MODE_OFB: return EVP_aes_192_ofb();
                default: return NULL;
            }
        case ENC_AES256:
            switch (mode) {
                case MODE_ECB: return EVP_aes_256_ecb();
                case MODE_CBC: return EVP_aes_256_cbc();
                case MODE_CFB: return EVP_aes_256_cfb1();  // CFB1 = 1 bit feedback
                case MODE_OFB: return EVP_aes_256_ofb();
                default: return NULL;
            }
        case ENC_3DES:
            switch (mode) {
                case MODE_ECB: return EVP_des_ede3_ecb();
                case MODE_CBC: return EVP_des_ede3_cbc();
                case MODE_CFB: return EVP_des_ede3_cfb8();  // CFB8 = 8 bits feedback
                case MODE_OFB: return EVP_des_ede3_ofb();   // OFB = 64 bits feedback for 3DES
                default: return NULL;
            }
        default:
            return NULL;
    }
}

/**
 * @brief Derives key and IV from password using PBKDF2-HMAC-SHA256
 */
static int derive_key_iv(const char *password, const EVP_CIPHER *cipher,unsigned char *key, unsigned char *iv)
{
    if (!password || !cipher) {
        return -1;
    }
    
    int key_len = EVP_CIPHER_key_length(cipher);
    int iv_len = EVP_CIPHER_iv_length(cipher);
    int total_len = key_len + iv_len;
    
    // Allocate buffer for key + iv
    unsigned char *key_iv_buffer = (unsigned char *)malloc(total_len);
    if (!key_iv_buffer) {
        fprintf(stderr, "Error: no se pudo asignar memoria para derivacion de clave\n");
        return -1;
    }
    
    // Use PBKDF2 with SHA256 as per specification
    int result = PKCS5_PBKDF2_HMAC(
        password, 
        strlen(password),
        FIXED_SALT, 
        sizeof(FIXED_SALT),
        PBKDF2_ITERATIONS,
        EVP_sha256(),
        total_len,
        key_iv_buffer
    );
    
    if (result != 1) {
        fprintf(stderr, "Error: fallo PBKDF2\n");
        free(key_iv_buffer);
        return -1;
    }
    
    // Split the buffer into key and IV
    memcpy(key, key_iv_buffer, key_len);
    if (iv_len > 0 && iv != NULL) {
        memcpy(iv, key_iv_buffer + key_len, iv_len);
    }
    
    free(key_iv_buffer);
    return 0;
}

int encrypt_data(const stegobmp_config_t *config,
                 const uint8_t *plaintext, size_t plaintext_len,
                 uint8_t **ciphertext, size_t *ciphertext_len)
{
    if (!config || !plaintext || !ciphertext || !ciphertext_len) {
        fprintf(stderr, "Error: parametros invalidos para encriptacion\n");
        return -1;
    }
    
    if (!is_encryption_enabled(config)) {
        fprintf(stderr, "Error: encriptacion no habilitada en config\n");
        return -1;
    }
    
    const EVP_CIPHER *cipher = get_cipher(config->encryption_algo, config->encryption_mode);
    if (!cipher) {
        fprintf(stderr, "Error: combinacion de algoritmo/modo no soportada\n");
        return -1;
    }
    
    unsigned char key[EVP_MAX_KEY_LENGTH];
    unsigned char iv[EVP_MAX_IV_LENGTH];
    memset(key, 0, sizeof(key));
    memset(iv, 0, sizeof(iv));
    
    if (derive_key_iv(config->password, cipher, key, iv) != 0) {
        fprintf(stderr, "Error: no se pudo derivar clave e IV\n");
        return -1;
    }
    
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        fprintf(stderr, "Error: no se pudo crear contexto de encriptacion\n");
        return -1;
    }
    
    if (EVP_EncryptInit_ex(ctx, cipher, NULL, key, iv) != 1) {
        fprintf(stderr, "Error: fallo inicializacion de encriptacion\n");
        ERR_print_errors_fp(stderr);
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    
    int block_size = EVP_CIPHER_block_size(cipher);
    size_t max_ciphertext_len = plaintext_len + block_size;
    *ciphertext = (uint8_t *)malloc(max_ciphertext_len);
    if (!*ciphertext) {
        fprintf(stderr, "Error: no se pudo asignar memoria para texto cifrado\n");
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    
    int len = 0;
    int total_len = 0;
    
    if (EVP_EncryptUpdate(ctx, *ciphertext, &len, plaintext, plaintext_len) != 1) {
        fprintf(stderr, "Error: fallo durante encriptacion\n");
        ERR_print_errors_fp(stderr);
        free(*ciphertext);
        *ciphertext = NULL;
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    total_len = len;
    
    if (EVP_EncryptFinal_ex(ctx, *ciphertext + len, &len) != 1) {
        fprintf(stderr, "Error: fallo finalizacion de encriptacion\n");
        ERR_print_errors_fp(stderr);
        free(*ciphertext);
        *ciphertext = NULL;
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    total_len += len;
    
    *ciphertext_len = total_len;
    
    // Cleanup
    EVP_CIPHER_CTX_free(ctx);
    
    // Clear sensitive data
    memset(key, 0, sizeof(key));
    memset(iv, 0, sizeof(iv));
    
    return 0;
}

int decrypt_data(const stegobmp_config_t *config,const uint8_t *ciphertext, size_t ciphertext_len,uint8_t **plaintext, size_t *plaintext_len)
{
    if (!config || !ciphertext || !plaintext || !plaintext_len) {
        fprintf(stderr, "Error: parametros invalidos para desencriptacion\n");
        return -1;
    }
    
    if (!is_encryption_enabled(config)) {
        fprintf(stderr, "Error: encriptacion no habilitada en config\n");
        return -1;
    }
    
    const EVP_CIPHER *cipher = get_cipher(config->encryption_algo, config->encryption_mode);
    if (!cipher) {
        fprintf(stderr, "Error: combinacion de algoritmo/modo no soportada\n");
        return -1;
    }
    
    unsigned char key[EVP_MAX_KEY_LENGTH];
    unsigned char iv[EVP_MAX_IV_LENGTH];
    memset(key, 0, sizeof(key));
    memset(iv, 0, sizeof(iv));
    
    if (derive_key_iv(config->password, cipher, key, iv) != 0) {
        fprintf(stderr, "Error: no se pudo derivar clave e IV\n");
        return -1;
    }
    
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        fprintf(stderr, "Error: no se pudo crear contexto de desencriptacion\n");
        return -1;
    }
    
    if (EVP_DecryptInit_ex(ctx, cipher, NULL, key, iv) != 1) {
        fprintf(stderr, "Error: fallo inicializacion de desencriptacion\n");
        ERR_print_errors_fp(stderr);
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    
    // Deshabilitar padding (matches working project)
    EVP_CIPHER_CTX_set_padding(ctx, 0);
    
    *plaintext = (uint8_t *)malloc(ciphertext_len);
    if (!*plaintext) {
        fprintf(stderr, "Error: no se pudo asignar memoria para texto plano\n");
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    
    int len = 0;
    int total_len = 0;
    
    if (EVP_DecryptUpdate(ctx, *plaintext, &len, ciphertext, ciphertext_len) != 1) {
        fprintf(stderr, "Error: fallo durante desencriptacion\n");
        ERR_print_errors_fp(stderr);
        free(*plaintext);
        *plaintext = NULL;
        EVP_CIPHER_CTX_free(ctx);
        return -1;
    }
    total_len = len;
    
    // For stream ciphers (OFB, CFB) with padding disabled, DecryptFinal may not add bytes
    // Check if this is a stream cipher mode
    int is_stream_mode = (config->encryption_mode == MODE_OFB || 
                          config->encryption_mode == MODE_CFB);
    
    if (EVP_DecryptFinal_ex(ctx, *plaintext + len, &len) != 1) {
        // For stream modes with padding disabled, this is expected to fail
        // The data was already decrypted in Update
        if (!is_stream_mode) {
            fprintf(stderr, "Error: fallo finalizacion de desencriptacion\n");
            fprintf(stderr, "       (password incorrecta o datos corruptos)\n");
            ERR_print_errors_fp(stderr);
            free(*plaintext);
            *plaintext = NULL;
            EVP_CIPHER_CTX_free(ctx);
            return -1;
        }
        // For stream modes, ignore the error - data was already decrypted
        len = 0;
    }
    total_len += len;
    
    *plaintext_len = total_len;
    
    // Cleanup
    EVP_CIPHER_CTX_free(ctx);
    
    // Clear sensitive data
    memset(key, 0, sizeof(key));
    memset(iv, 0, sizeof(iv));
    
    return 0;
}

bool is_encryption_enabled(const stegobmp_config_t *config)
{
    return config != NULL && 
           config->password != NULL && 
           strlen(config->password) > 0;
}

char* get_encryption_description(const stegobmp_config_t *config, char *buffer, size_t buffer_size)
{
    if (!config || !buffer || buffer_size == 0) {
        return NULL;
    }
    
    if (!is_encryption_enabled(config)) {
        snprintf(buffer, buffer_size, "Ninguna");
        return buffer;
    }
    
    const char *algo_str = encryption_algo_to_string(config->encryption_algo);
    const char *mode_str = encryption_mode_to_string(config->encryption_mode);
    
    // Format: "AES128-CBC" (uppercase algorithm, uppercase mode)
    // Convert to uppercase format for display
    if (strcmp(algo_str, "aes128") == 0) {
        snprintf(buffer, buffer_size, "AES128-%s", 
                 strcmp(mode_str, "ecb") == 0 ? "ECB" :
                 strcmp(mode_str, "cbc") == 0 ? "CBC" :
                 strcmp(mode_str, "cfb") == 0 ? "CFB" :
                 strcmp(mode_str, "ofb") == 0 ? "OFB" : mode_str);
    } else if (strcmp(algo_str, "aes192") == 0) {
        snprintf(buffer, buffer_size, "AES192-%s",
                 strcmp(mode_str, "ecb") == 0 ? "ECB" :
                 strcmp(mode_str, "cbc") == 0 ? "CBC" :
                 strcmp(mode_str, "cfb") == 0 ? "CFB" :
                 strcmp(mode_str, "ofb") == 0 ? "OFB" : mode_str);
    } else if (strcmp(algo_str, "aes256") == 0) {
        snprintf(buffer, buffer_size, "AES256-%s",
                 strcmp(mode_str, "ecb") == 0 ? "ECB" :
                 strcmp(mode_str, "cbc") == 0 ? "CBC" :
                 strcmp(mode_str, "cfb") == 0 ? "CFB" :
                 strcmp(mode_str, "ofb") == 0 ? "OFB" : mode_str);
    } else if (strcmp(algo_str, "3des") == 0) {
        snprintf(buffer, buffer_size, "3DES-%s",
                 strcmp(mode_str, "ecb") == 0 ? "ECB" :
                 strcmp(mode_str, "cbc") == 0 ? "CBC" :
                 strcmp(mode_str, "cfb") == 0 ? "CFB" :
                 strcmp(mode_str, "ofb") == 0 ? "OFB" : mode_str);
    } else {
        snprintf(buffer, buffer_size, "%s-%s", algo_str, mode_str);
    }
    
    return buffer;
}

