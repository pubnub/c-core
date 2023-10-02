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

//#endif // PUBNUB_CRYPTO_API
