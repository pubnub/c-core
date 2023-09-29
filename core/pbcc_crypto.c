/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

//#ifdef PUBNUB_CRYPTO_API

#include <stdio.h>
#include "pbcc_crypto.h"
#include "pbaes256.h"
#include "pbsha256.h"
#include "pbbase64.h"
#include "pubnub_crypto.h"
#include "pubnub_memory_block.h"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

int pbcc_cipher_key_hash(const uint8_t* cipher_key, uint8_t* hash) {
    uint8_t digest[32];
    pbsha256_digest_str((char*)cipher_key, digest);

    snprintf((char*)hash, 33,
        "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
        digest[0], digest[1], digest[2], digest[3],
        digest[4], digest[5], digest[6], digest[7],
        digest[8], digest[9], digest[10], digest[11],
        digest[12], digest[13], digest[14], digest[15]
    );

    return 0;
}

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
        struct pubnub_crypto_algorithm_t const *algo,
        pubnub_bymebl_t* result,
        struct pubnub_encrypted_data to_decrypt
) {
    struct aes_context *ctx = (struct aes_context *)algo->user_data;

    uint8_t key_hash[33];
    if (0 != pbcc_cipher_key_hash(ctx->cipher_key, key_hash)) {
        return -1;
    }

    pubnub_bymebl_t decoded = pbbase64_decode_alloc_std((char*)to_decrypt.data.ptr, to_decrypt.data.size);
    if (decoded.ptr == NULL) {
        return -2;
    }

    size_t dec_buffer_size = estimated_dec_buffer_size(to_decrypt.data.size) + 5000; // TODO: Why do I need additional space?

    result->ptr = (uint8_t *)malloc(dec_buffer_size);
    if (result->ptr == NULL) {
        return -3;
    }

    result->size = dec_buffer_size;

    if (0 != pbaes256_decrypt(
            decoded,
            key_hash,
            to_decrypt.metadata.ptr,
            result
    )) {
        free(result->ptr);
        free(decoded.ptr);
        return -4;
    }

    return 0;
}


//#endif // PUBNUB_CRYPTO_API
