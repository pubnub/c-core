/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

//#ifdef PUBNUB_CRYPTO_API

#include <stdio.h>
#include "pbcc_crypto.h"
#include "pbsha256.h"
#include "pubnub_crypto.h"
#include <string.h>

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
#define LEGACY_IDENTIFIER "0000"

struct pubnub_cryptor_header_v1 pbcc_prepare_cryptor_header_v1(uint8_t identifier[4], struct pubnub_byte_mem_block metadata) {
    struct pubnub_cryptor_header_v1 header;
    memset(&header, 0, sizeof header);

    memcpy(header.identifier, identifier, 4);
    header.data_length = metadata.ptr != NULL ? metadata.size: 0;

    return header;

}

size_t pbcc_cryptor_header_v1_size(struct pubnub_cryptor_header_v1 *cryptor_header) {
    return NULL == cryptor_header 
        ? 0 
        : (strlen(SENTINEL) + 1 + IDENTIFIER_LENGTH 
            + (cryptor_header->data_length < 255 ? 1 : 3) 
            + cryptor_header->data_length); 
}

struct pubnub_byte_mem_block* pbcc_cryptor_header_v1_to_alloc_block(struct pubnub_cryptor_header_v1 *cryptor_header) {
    uint8_t version = 1;
    struct pubnub_byte_mem_block* result = (struct pubnub_byte_mem_block*)malloc(sizeof(struct pubnub_byte_mem_block));
    if (NULL == result) {
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
        free(result);
        return NULL;
    }

    result->size = header_size;

    size_t offset = 0;

    memcpy(result->ptr + offset, SENTINEL, strlen(SENTINEL));
    offset += strlen(SENTINEL);

    memcpy(result->ptr + offset, &version, 1);
    offset += 1;

    if (0 != strcmp((char*)cryptor_header->identifier, LEGACY_IDENTIFIER)) {
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
//#endif // PUBNUB_CRYPTO_API
