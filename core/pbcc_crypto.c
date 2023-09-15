/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

//#ifdef PUBNUB_CRYPTO_API

#include "pbcc_crypto.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#define AES_IDENTIFIER "ACHE"
#define AES_BLOCK_SIZE 16
#define AES_IV_SIZE 16

static int aes_encrypt(
        struct pubnub_crypto_algorithm_t const *algo,
        struct pubnub_encrypted_data *msg,
        char *base64_str,
        size_t n
);

static int aes_decrypt(
        struct pubnub_crypto_algorithm_t const *algo,
        char const *base64_str,
        struct pubnub_encrypted_data *msg
);

struct aes_context {
    const char* cipher_key;
};

struct pubnub_crypto_algorithm_t *pbcc_aes_cbc_init(const char* cipher_key) {
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

static void generate_init_vector(char *iv) {
    for (int i = 0; i < AES_IV_SIZE; i++) {
        iv[i] = rand() % 256;
    }
}

static int aes_encrypt(
        struct pubnub_crypto_algorithm_t const *algo,
        struct pubnub_encrypted_data *msg,
        char *base64_str,
        size_t n
) {
    struct aes_context *ctx = (struct aes_context *)algo->user_data;

    size_t enc_buffer_size = estimated_enc_buffer_size(n);

    msg->data.ptr = (uint8_t *)malloc(enc_buffer_size);
    if (msg->data.ptr == NULL) {
        return -1;
    }

    msg->data.size = enc_buffer_size;

    char iv[AES_IV_SIZE];
    generate_init_vector(iv);

    // TODO: use proper function to fulfill the data.ptr
    
    msg->metadata.ptr = (char *)malloc(AES_IV_SIZE);
    if (msg->metadata.ptr == NULL) {
        free(msg->data.ptr);
        return -1;
    }

    // TODO: resize the data.ptr to the actual size

    memcpy(msg->metadata.ptr, iv, AES_IV_SIZE);
    msg->metadata.size = AES_IV_SIZE;

    return 0;
}

static int aes_decrypt(
        struct pubnub_crypto_algorithm_t const *algo,
        char const *base64_str,
        struct pubnub_encrypted_data *msg
) {
    struct aes_context *ctx = (struct aes_context *)algo->user_data;

    if (msg->metadata.ptr == NULL || msg->metadata.size != AES_IV_SIZE) {
        return -1;
    }

    size_t dec_buffer_size = estimated_dec_buffer_size(msg->data.size);

    msg->data.ptr = (uint8_t *)malloc(dec_buffer_size);
    if (msg->data.ptr == NULL) {
        return -1;
    }

    msg->data.size = dec_buffer_size;

    // TODO: use proper function to fulfill base64_str

    return strlen(base64_str);
}


//#endif // PUBNUB_CRYPTO_API
