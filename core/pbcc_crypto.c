/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#include <stdint.h>
#include <stdio.h>
#include "pbbase64.h"
#include "pbcc_crypto.h"
#include "pbsha256.h"
#include "pbaes256.h"
#include "pubnub_crypto.h"
#include "pubnub_log.h"
#include "pubnub_memory_block.h"
#include "pubnub_assert.h"
#include <string.h>
#include <openssl/rand.h>

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

/// Maximum cryptor identifier length.
#define IDENTIFIER_LENGTH 4

#define SENTINEL "PNED"

struct pubnub_cryptor_header_v1 pbcc_prepare_cryptor_header_v1(uint8_t identifier[4], struct pubnub_byte_mem_block metadata) {
    struct pubnub_cryptor_header_v1 header;
    memset(&header, 0, sizeof header);

    memcpy(header.identifier, identifier, 4);
    header.data_length = metadata.ptr != NULL ? metadata.size : 0;

    return header;

}

size_t pbcc_cryptor_header_v1_size(struct pubnub_cryptor_header_v1 *cryptor_header) {
    return NULL == cryptor_header || memcmp(cryptor_header->identifier, PUBNUB_LEGACY_CRYPTO_IDENTIFIER, 4) == 0
        ? 0 
        : (strlen(SENTINEL) + 1 + IDENTIFIER_LENGTH 
            + (cryptor_header->data_length < 255 ? 1 : 3) 
            + cryptor_header->data_length); 
}

struct pubnub_byte_mem_block* pbcc_cryptor_header_v1_to_alloc_block(struct pubnub_cryptor_header_v1 *cryptor_header) {
    if (0 == memcmp((char*)cryptor_header->identifier, PUBNUB_LEGACY_CRYPTO_IDENTIFIER, IDENTIFIER_LENGTH)) {
        PUBNUB_LOG_WARNING("pbcc_cryptor_header_v1_to_alloc_block: Legacy encryption. No header created!\n");
        pubnub_bymebl_t* result = (pubnub_bymebl_t*)malloc(sizeof(pubnub_bymebl_t));
        result->ptr = NULL;
        result->size = 0;

        return result;
    }

    uint8_t version = 1;
    struct pubnub_byte_mem_block* result = (struct pubnub_byte_mem_block*)malloc(sizeof(struct pubnub_byte_mem_block));
    if (NULL == result) {
        PUBNUB_LOG_ERROR("pbcc_cryptor_header_v1_to_alloc_block: failed to allocate memory\n");
        return NULL;
    }

    if (NULL == cryptor_header) {
        result->ptr = NULL;
        result->size = 0;

        return result;
    }

    size_t header_size = pbcc_cryptor_header_v1_size(cryptor_header);

    result->ptr = (uint8_t*)malloc(header_size);
    if (NULL == result->ptr) {
        PUBNUB_LOG_ERROR("pbcc_cryptor_header_v1_to_alloc_block: failed to allocate memory\n");
        free(result);
        return NULL;
    }


    result->size = header_size;

    size_t offset = 0;

    memcpy(result->ptr + offset, SENTINEL, strlen(SENTINEL));
    offset += strlen(SENTINEL);

    memcpy(result->ptr + offset, &version, 1);
    offset += 1;

    if (0 != memcmp((char*)cryptor_header->identifier, PUBNUB_LEGACY_CRYPTO_IDENTIFIER, IDENTIFIER_LENGTH)) {
        memcpy(result->ptr + offset, cryptor_header->identifier, IDENTIFIER_LENGTH);
        offset += IDENTIFIER_LENGTH;
    }

    if (cryptor_header->data_length < 255) {
        uint8_t data_length = (uint8_t)cryptor_header->data_length;
        memcpy(result->ptr + offset, &data_length, 1);
    } else {
        uint8_t data_length = 255;
        memcpy(result->ptr + offset, &data_length, 1);
        offset += 1;

        uint8_t header_size_high = (uint8_t)(header_size >> 8);
        memcpy(result->ptr + offset, &header_size_high, 1);
        offset += 1;

        uint8_t header_size_low = (uint8_t)(header_size & 0xFF);
        memcpy(result->ptr + offset, &header_size_low, 1);
    }

    return result;
}

struct pubnub_cryptor_header_v1* pbcc_cryptor_header_v1_from_block(struct pubnub_byte_mem_block *cryptor_header) {
    if (NULL == cryptor_header || NULL == cryptor_header->ptr || 0 == cryptor_header->size) {
        PUBNUB_LOG_ERROR("pbcc_cryptor_header_v1_from_block: invalid argument\n");
        return NULL;
    }

    if (cryptor_header->size < strlen(SENTINEL) + 1 + IDENTIFIER_LENGTH + 1) {
        PUBNUB_LOG_ERROR("pbcc_cryptor_header_v1_from_block: invalid header size\n");
        return NULL;
    }

    struct pubnub_cryptor_header_v1* result = (struct pubnub_cryptor_header_v1*)malloc(sizeof(struct pubnub_cryptor_header_v1));
    if (NULL == result) {
        PUBNUB_LOG_ERROR("pbcc_cryptor_header_v1_from_block: failed to allocate memory\n");
        return NULL;
    }

    size_t offset = 0;

    if (0 != memcmp(cryptor_header->ptr + offset, SENTINEL, strlen(SENTINEL))) {
        PUBNUB_LOG_WARNING("pbcc_cryptor_header_v1_from_block: invalid header sentinel - assuming legacy crypto\n");
        free(result);
        return NULL;
    }
    offset += strlen(SENTINEL);

    uint8_t version = 0;
    memcpy(&version, cryptor_header->ptr + offset, 1);
    offset += 1;

    if (version == 0 || version > PUBNUB_MAXIMUM_HEADER_VERSION) {
        PUBNUB_LOG_ERROR("pbcc_cryptor_header_v1_from_block: unknown header version\n");
        free(result);
        return NULL;
    }

    memcpy(result->identifier, cryptor_header->ptr + offset, IDENTIFIER_LENGTH);
    offset += IDENTIFIER_LENGTH;

    uint8_t data_length = 0;
    memcpy(&data_length, cryptor_header->ptr + offset, 1);
    offset += 1;

    if (data_length == 255) {
        uint8_t header_size_high = 0;
        memcpy(&header_size_high, cryptor_header->ptr + offset, 1);
        offset += 1;

        uint8_t header_size_low = 0;
        memcpy(&header_size_low, cryptor_header->ptr + offset, 1);
        offset += 1;

        result->data_length = (header_size_high << 8) + header_size_low;
    } else {
        result->data_length = data_length;
    }

    return result;
}

pubnub_bymebl_t pbcc_legacy_encrypt(uint8_t const* cipher_key, pubnub_bymebl_t msg)
{
    uint8_t key[33];
    unsigned char iv[17] = "0123456789012345";
#if PUBNUB_RAND_INIT_VECTOR
    int rand_status = RAND_bytes(iv, 16);
    PUBNUB_ASSERT_OPT(rand_status == 1);
#endif

    pbcc_cipher_key_hash((uint8_t*)cipher_key, key);
    pubnub_bymebl_t result = pbaes256_encrypt_alloc(msg, key, iv);

    #if PUBNUB_RAND_INIT_VECTOR
        memmove(result.ptr + 16, result.ptr, result.size);
        memcpy(result.ptr, iv, 16);
        result.size += 16;
        result.ptr[result.size] = '\0';
    #endif

    return result;
}

int pbcc_memory_encode(pubnub_bymebl_t buffer, char* base64_str, unsigned char* iv, size_t* n) 
{
#if PUBNUB_RAND_INIT_VECTOR
    memmove(buffer.ptr + 16, buffer.ptr, buffer.size);
    memcpy(buffer.ptr, iv, 16);
    buffer.size += 16;
    buffer.ptr[buffer.size] = '\0';
#endif

#if PUBNUB_LOG_LEVEL >= PUBNUB_LOG_LEVEL_DEBUG
    PUBNUB_LOG_DEBUG("\nbytes before encoding iv + encrypted msg = [");
    for (int i = 0; i < (int)buffer.size; i++) {
        PUBNUB_LOG_DEBUG("%d ", buffer.ptr[i]);
    }
    PUBNUB_LOG_DEBUG("]\n");
#endif

    int max_size = base64_max_size(buffer.size);
    if (*n + 1 < (size_t)max_size) {
        PUBNUB_LOG_DEBUG("base64encode needs %d bytes but only %zu bytes are available\n", max_size, *n);
        return -1;
    }
    char* base64_output = (char*)malloc(max_size);
    if (base64encode(base64_output, max_size, buffer.ptr, buffer.size) != 0) {
        PUBNUB_LOG_DEBUG("base64encode tried to use more than %d bytes to encode %zu bytes\n", max_size, buffer.size);
        free(base64_output);
        return -1;
    }
    int result = snprintf(base64_str, *n, "%s", base64_output);
    *n = (size_t)strlen(base64_str);

    free(base64_output);

    return result >= 0 ? 0 : -1; 
} 

int pbcc_legacy_decrypt(uint8_t const* cipher_key, pubnub_bymebl_t *result, pubnub_bymebl_t to_decrypt) {
    unsigned char iv[17] = "0123456789012345";
    uint8_t key[33];

    pbcc_cipher_key_hash(cipher_key, key);

    if (to_decrypt.ptr != NULL) {
        PUBNUB_LOG_DEBUG("pbcc_legacy_decrypt: Decrypting data with size size = %zu\n", to_decrypt.size);

#if PUBNUB_RAND_INIT_VECTOR
        memcpy(iv, to_decrypt.ptr, 16);
        memmove(to_decrypt.ptr, to_decrypt.ptr + 16, to_decrypt.size - 16);
        to_decrypt.size = to_decrypt.size - 16;
#endif

        return pbaes256_decrypt(to_decrypt, key, iv, result);
    }

    return -1;
}

const char* pbcc_base64_encode(pubnub_bymebl_t buffer) {
    int max_size = base64_max_size(buffer.size);

    char* result = (char*)malloc(max_size);
    if (result == NULL) {
        PUBNUB_LOG_ERROR("pbcc_base64_encode: failed to allocate memory for result\n");
        return NULL;
    }

    if (base64encode(result, max_size, buffer.ptr, buffer.size) != 0) {
        PUBNUB_LOG_ERROR("pbcc_base64_encode: failed to encode %zu bytes\n", buffer.size);
        free(result);
        return NULL;
    }

    return result;
}

pubnub_bymebl_t pbcc_base64_decode(const char* buffer) {
    return pbbase64_decode_alloc_std_str(buffer);
}

void pbcc_set_crypto_module(struct pbcc_context *ctx, struct pubnub_crypto_provider_t *crypto_provider) {
    ctx->crypto_module = crypto_provider;
}

pubnub_crypto_provider_t *pbcc_get_crypto_module(struct pbcc_context *ctx) {
    return ctx->crypto_module;
}
