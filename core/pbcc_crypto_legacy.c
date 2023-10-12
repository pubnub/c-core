/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#include "pbcc_crypto.h"
#include "pubnub_crypto.h"
#include "pubnub_memory_block.h"
#include "pubnub_log.h"
#include <string.h>
#include <stdlib.h>

#define AES_BLOCK_SIZE 16

static int legacy_encrypt(
        struct pubnub_cryptor_t const *algo,
        struct pubnub_encrypted_data *result,
        pubnub_bymebl_t to_encrypt
);

static int legacy_decrypt(
        struct pubnub_cryptor_t const *algo,
        pubnub_bymebl_t* result,
        struct pubnub_encrypted_data to_decrypt
);

struct legacy_context {
    const uint8_t* cipher_key;
};

struct pubnub_cryptor_t *pbcc_legacy_crypto_init(const uint8_t* cipher_key) {
    struct pubnub_cryptor_t *algo = 
        (struct pubnub_cryptor_t *)malloc(sizeof(struct pubnub_cryptor_t));
    if (algo == NULL) {
        return NULL;
    }

    memcpy(algo->identifier, PUBNUB_LEGACY_CRYPTO_IDENTIFIER, 4);

    algo->encrypt = &legacy_encrypt;
    algo->decrypt = &legacy_decrypt;

    struct legacy_context *ctx = (struct legacy_context *)malloc(sizeof(struct legacy_context));
    if (NULL == ctx) {
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
    return n + 1; // for the terminating array
}

static int legacy_encrypt(
        struct pubnub_cryptor_t const *algo,
        struct pubnub_encrypted_data *result,
        pubnub_bymebl_t to_encrypt
) {
    struct legacy_context *ctx = (struct legacy_context *)algo->user_data;

    size_t estimated_size = estimated_enc_buffer_size(to_encrypt.size);
    result->data.ptr = (uint8_t*)malloc(estimated_size);
    memset(result->data.ptr, 0, estimated_size);
    if (NULL == result->data.ptr) {
        return -1;
    }
    result->data.size = estimated_size;

    result->data = pbcc_legacy_encrypt(ctx->cipher_key, to_encrypt);
    if (NULL == result->data.ptr) {
        PUBNUB_LOG_ERROR("pbcc_legacy_encrypt() failed\n");
        return -1;
    }

    result->metadata.ptr = NULL;
    result->metadata.size = 0;

    return 0;
}

static int legacy_decrypt(
        struct pubnub_cryptor_t const *algo,
        pubnub_bymebl_t *result,
        struct pubnub_encrypted_data to_decrypt
) {
    struct legacy_context *ctx = (struct legacy_context *)algo->user_data;

    size_t estimated_size = estimated_dec_buffer_size(to_decrypt.data.size);
    result->ptr = (uint8_t*)malloc(estimated_size);
    memset(result->ptr, 0, estimated_size);
    if (NULL == result->ptr) {
        PUBNUB_LOG_ERROR("legacy_decrypt: failed to allocate bytes for decryption");
        return -1;
    }
    result->size = estimated_size;

    pbcc_legacy_decrypt(ctx->cipher_key, result, to_decrypt.data);
    if (NULL == result->ptr) {
        PUBNUB_LOG_ERROR("pubnub_decrypt() failed\n");
        free(result->ptr);
        return -1;
    }

    result->size = strlen((char*)result->ptr);

    return 0;
}

