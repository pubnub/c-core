#include <stdio.h>
#include "pbcc_crypto.h"
#include "pbaes256.h"
#include "pbbase64.h"
#include "pubnub_crypto.h"
#include "pubnub_memory_block.h"
#include "pubnub_log.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>


#define AES_IDENTIFIER "ACRH"
#define AES_BLOCK_SIZE 16
#define AES_IV_SIZE 16

static int aes_encrypt(
        struct pubnub_cryptor_t const *algo,
        struct pubnub_encrypted_data *result,
        pubnub_bymebl_t to_encrypt
);

static int aes_decrypt(
        struct pubnub_cryptor_t const *algo,
        pubnub_bymebl_t* result,
        struct pubnub_encrypted_data to_decrypt
);

struct aes_context {
    const uint8_t* cipher_key;
};

struct pubnub_cryptor_t *pbcc_aes_cbc_init(const uint8_t* cipher_key) {
    struct pubnub_cryptor_t *algo = 
        (struct pubnub_cryptor_t *)malloc(sizeof(struct pubnub_cryptor_t));
    if (algo == NULL) {
        return NULL;
    }

    memcpy(algo->identifier, AES_IDENTIFIER, 4);

    algo->encrypt = &aes_encrypt;
    algo->decrypt = &aes_decrypt;

    struct aes_context *ctx = (struct aes_context *)malloc(sizeof(struct aes_context));
    if (ctx == NULL) {
        free(algo);
        return NULL;
    }

    ctx->cipher_key = cipher_key;

    algo->user_data = (void*)ctx;

    return algo;
};

static size_t estimated_enc_buffer_size(size_t n) {
    return n + (AES_BLOCK_SIZE - (n % AES_BLOCK_SIZE)) + AES_BLOCK_SIZE;
}

static size_t estimated_dec_buffer_size(size_t n) {
    return n;
}

static void generate_init_vector(uint8_t *iv) {
    for (int i = 0; i < AES_IV_SIZE; i++) {
        iv[i] = rand() % 256;
    }
}

static int aes_encrypt(
        struct pubnub_cryptor_t const *algo,
        struct pubnub_encrypted_data *result,
        pubnub_bymebl_t to_encrypt
) {
    struct aes_context *ctx = (struct aes_context *)algo->user_data;

    uint8_t key_hash[33];
    if (0 != pbcc_cipher_key_hash(ctx->cipher_key, key_hash)) {
        return -1;
    }

    size_t enc_buffer_size = estimated_enc_buffer_size(to_encrypt.size);

    pubnub_bymebl_t encrypted;
    encrypted.ptr = (uint8_t *)malloc(enc_buffer_size);
    if (encrypted.ptr == NULL) {
        return -1;
    }
    encrypted.size = enc_buffer_size;

    uint8_t iv[AES_IV_SIZE];
    generate_init_vector(iv);

    if (0 != pbaes256_encrypt(to_encrypt, key_hash, iv, &encrypted)) {
        free(encrypted.ptr);
        return -1;
    };

    size_t max_encoded_size = base64_max_size(encrypted.size);

    result->data.ptr = (uint8_t *)malloc(max_encoded_size);
    if (result->data.ptr == NULL) {
        free(encrypted.ptr);
        return -1;
    }

    if (0 != base64encode((char*)result->data.ptr, max_encoded_size, encrypted.ptr, encrypted.size)) {
        free(result->data.ptr);
        free(encrypted.ptr);
        return -1;
    }
    result->data.size = strlen((char*)result->data.ptr);

    result->metadata.ptr = (uint8_t *)malloc(AES_IV_SIZE);
    if (result->metadata.ptr == NULL) {
        free(encrypted.ptr);
        free(result->data.ptr);
        return -1;
    }

    memcpy(result->metadata.ptr, iv, AES_IV_SIZE);
    result->metadata.size = AES_IV_SIZE;

    free(encrypted.ptr);

    return 0;
}

static int aes_decrypt(
        struct pubnub_cryptor_t const *algo,
        pubnub_bymebl_t* result,
        struct pubnub_encrypted_data to_decrypt
) {
    struct aes_context *ctx = (struct aes_context *)algo->user_data;

    uint8_t key_hash[33];
    if (0 != pbcc_cipher_key_hash(ctx->cipher_key, key_hash)) {
        PUBNUB_LOG_ERROR("Failed to generate key hash\n");
        return -1;
    }

    pubnub_bymebl_t decoded = pbbase64_decode_alloc_std((char*)to_decrypt.data.ptr, to_decrypt.data.size);
    if (decoded.ptr == NULL) {
        PUBNUB_LOG_ERROR("Failed to decode base64\n");
        return -1;
    }

    size_t dec_buffer_size = estimated_dec_buffer_size(to_decrypt.data.size) + 5000; // TODO: Why do I need additional space?

    result->ptr = (uint8_t *)malloc(dec_buffer_size);
    if (result->ptr == NULL) {
        PUBNUB_LOG_ERROR("Failed to allocate memory for decrypted data\n");
        return -1;
    }

    result->size = dec_buffer_size;

    if (0 != pbaes256_decrypt(
            decoded,
            key_hash,
            to_decrypt.metadata.ptr,
            result
    )) {
        PUBNUB_LOG_ERROR("Failed to decrypt data\n");
        free(result->ptr);
        free(decoded.ptr);
        return -1;
    }

    return 0;
}


