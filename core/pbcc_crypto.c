/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

//#ifdef PUBNUB_CRYPTO_API

#include "pbcc_crypto.h"
#include "pbaes256.h"
#include "pubnub_memory_block.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#define AES_IDENTIFIER "ACRH"
#define AES_BLOCK_SIZE 16
#define AES_IV_SIZE 16

static int aes_encrypt(
        struct pubnub_crypto_algorithm_t const *algo,
        struct pubnub_encrypted_data *result,
        pubnub_bymebl_t to_encrypt
);

static int aes_decrypt(
        struct pubnub_crypto_algorithm_t const *algo,
        pubnub_bymebl_t* result,
        struct pubnub_encrypted_data to_decrypt
);

struct aes_context {
    const uint8_t* cipher_key;
};

struct pubnub_crypto_algorithm_t *pbcc_aes_cbc_init(const uint8_t* cipher_key) {
    struct pubnub_crypto_algorithm_t *algo = 
        (struct pubnub_crypto_algorithm_t *)malloc(sizeof(struct pubnub_crypto_algorithm_t));
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
        struct pubnub_crypto_algorithm_t const *algo,
        struct pubnub_encrypted_data *result,
        pubnub_bymebl_t to_encrypt
) {
    struct aes_context *ctx = (struct aes_context *)algo->user_data;

    size_t enc_buffer_size = estimated_enc_buffer_size(to_encrypt.size);

    result->data.ptr = (uint8_t *)malloc(enc_buffer_size);
    if (result->data.ptr == NULL) {
        return -1;
    }

    result->data.size = enc_buffer_size;

    uint8_t iv[AES_IV_SIZE];
    generate_init_vector(iv);

    pbaes256_encrypt(
            to_encrypt,
            ctx->cipher_key,
            iv,
            &result->data
    );

    result->metadata.ptr = (uint8_t *)malloc(AES_IV_SIZE);
    if (result->metadata.ptr == NULL) {
        free(result->data.ptr);
        return -1;
    }

    memcpy(result->metadata.ptr, iv, AES_IV_SIZE);
    result->metadata.size = AES_IV_SIZE;

    return 0;
}

static int aes_decrypt(
        struct pubnub_crypto_algorithm_t const *algo,
        pubnub_bymebl_t* result,
        struct pubnub_encrypted_data to_decrypt
) {
    struct aes_context *ctx = (struct aes_context *)algo->user_data;

    size_t dec_buffer_size = estimated_dec_buffer_size(to_decrypt.data.size) + 256; // TODO: Why do I need additional space?

    result->ptr = (uint8_t *)malloc(dec_buffer_size);
    if (result->ptr == NULL) {
        return -1;
    }

    result->size = dec_buffer_size;

    pbaes256_decrypt(
            to_decrypt.data,
            ctx->cipher_key,
            to_decrypt.metadata.ptr,
            result
    );

    return 0;
}


//#endif // PUBNUB_CRYPTO_API
