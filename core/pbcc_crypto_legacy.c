/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

//#ifdef PUBNUB_CRYPTO_API

#include "pbcc_crypto.h"
#include "pubnub_crypto.h"
#include <string.h>
#include <stdlib.h>

#define LEGACY_IDENTIFIER "0000"

static int legacy_encrypt(
        struct pubnub_crypto_algorithm_t const *algo,
        struct pubnub_encrypted_data *msg,
        char *base64_str,
        size_t n
);

static int legacy_decrypt(
        struct pubnub_crypto_algorithm_t const *algo,
        char const *base64_str,
        struct pubnub_encrypted_data *msg
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
        struct pubnub_encrypted_data *msg,
        char *base64_str,
        size_t n
) {
    struct legacy_context *ctx = (struct legacy_context *)algo->user_data;

    if (0 != pubnub_encrypt(ctx->cipher_key, msg->data, base64_str, &n)) {
        return -1;
    }

    msg->metadata.ptr = NULL;
    msg->metadata.size = 0;

    return 0;
}

static int legacy_decrypt(
        struct pubnub_crypto_algorithm_t const *algo,
        char const *base64_str,
        struct pubnub_encrypted_data *msg
) {
    struct legacy_context *ctx = (struct legacy_context *)algo->user_data;

    if (0 != pubnub_decrypt(ctx->cipher_key, base64_str, &msg->data)) {
        return -1;
    }

    return strlen(base64_str);
}


//#endif // PUBNUB_CRYPTO_API
