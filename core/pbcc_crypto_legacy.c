/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

//#ifdef PUBNUB_CRYPTO_API

#include "pbcc_crypto.h"
#include "pubnub_crypto.h"
#include "pubnub_memory_block.h"
#include <string.h>
#include <stdlib.h>

#define LEGACY_IDENTIFIER "0000"

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
    if (ctx == NULL) {
        free(algo);
        return NULL;
    }

    ctx->cipher_key = cipher_key;

    algo->user_data = (void*)ctx;

    return algo;
};

static int legacy_encrypt(
        struct pubnub_crypto_algorithm_t const *algo,
        struct pubnub_encrypted_data *result,
        pubnub_bymebl_t to_encrypt
) {
    struct legacy_context *ctx = (struct legacy_context *)algo->user_data;

    if (0 != pubnub_encrypt(ctx->cipher_key, to_encrypt, result->data.ptr, &result->data.size)) {
        return -1;
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

    if (0 != pubnub_decrypt(ctx->cipher_key, to_decrypt.data.ptr, result)) {
        return -1;
    }

    return 0;
}


//#endif // PUBNUB_CRYPTO_API
