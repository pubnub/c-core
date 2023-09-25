/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

//#ifdef PUBNUB_CRYPTO_API

#include "pbcc_crypto.h"
#include "pubnub_crypto.h"
#include "pubnub_memory_block.h"
#include <string.h>
#include <stdlib.h>

#define LEGACY_IDENTIFIER "0000"
#define AES_BLOCK_SIZE 16

static int legacy_encrypt(
        struct pubnub_crypto_algorithm_t const *algo,
        struct pubnub_encrypted_data *result,
        pubnub_bymebl_t to_encrypt
);

static int legacy_decrypt(
        struct pubnub_crypto_algorithm_t const *algo,
        pubnub_bymebl_t* result,
        struct pubnub_encrypted_data to_decrypt
);

struct legacy_context {
    const char* cipher_key;
};

struct pubnub_crypto_algorithm_t *pbcc_legacy_crypto_init(const char* cipher_key) {
    struct pubnub_crypto_algorithm_t *algo = 
        (struct pubnub_crypto_algorithm_t *)malloc(sizeof(struct pubnub_crypto_algorithm_t));
    if (algo == NULL) {
        return NULL;
    }

    memcpy(algo->identifier, LEGACY_IDENTIFIER, 4);

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
    return n;
}

static int legacy_encrypt(
        struct pubnub_crypto_algorithm_t const *algo,
        struct pubnub_encrypted_data *result,
        pubnub_bymebl_t to_encrypt
) {
    struct legacy_context *ctx = (struct legacy_context *)algo->user_data;

    size_t estimated_size = base64_max_size(estimated_enc_buffer_size(to_encrypt.size));
    result->data.ptr = (char*)malloc(estimated_size);
    if (NULL == result->data.ptr) {
        return -1;
    }
    result->data.size = estimated_size;

    int res = pubnub_encrypt(ctx->cipher_key, to_encrypt, result->data.ptr, &result->data.size);
    if (0 != res) {
        return res;
    }

    result->metadata.ptr = NULL;
    result->metadata.size = 0;

    return 0;
}

static int legacy_decrypt(
        struct pubnub_crypto_algorithm_t const *algo,
        pubnub_bymebl_t *result,
        struct pubnub_encrypted_data to_decrypt
) {
    struct legacy_context *ctx = (struct legacy_context *)algo->user_data;

    size_t estimated_size = estimated_dec_buffer_size(to_decrypt.data.size);
    result->ptr = (uint8_t*)malloc(estimated_size);
    if (NULL == result->ptr) {
        return -1;
    }
    result->size = estimated_size;

int res = pubnub_decrypt(ctx->cipher_key, to_decrypt.data.ptr, result);
    if (0 != res) {
        return res;
    }

    return 0;
}


//#endif // PUBNUB_CRYPTO_API
