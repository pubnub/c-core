/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

//#ifdef PUBNUB_CRYPTO_API
#ifndef PBCC_CRYPTO_H
#define PBCC_CRYPTO_H

/** @file pbcc_crypto.h

    This has the functions for crypto related to the PubNub C client.
    It contains functions used by official PubNub C client API
    and some abstract functions that mey be need to be implemented by the user.
*/

#include "pubnub_memory_block.h"

/** @file pubnub_crypto.h 

    This is the "Crypto" API of the Pubnub client library.  It enables
    encryptyng and decrypting messages being sent/received (published/
    subscribed) to/from Pubnub.

    This is independent from using SSL/TLS - if supported on a
    platform, one can encrypt/decrypt with or without using SSL/TLS.

*/

/**
   Encrypted data structure.
 */
struct pubnub_encrypted_data {
    /** Encrypted data. */
    struct pubnub_byte_mem_block data;

    /** Metadata. 
        
        Cryptor may provide here any information which will be usefull when data 
        should be decrypted.

        For example `metadata` may contain:
        - initialization vector
        - cipher key Identifier
        - encrypted *data* length
     */
    struct pubnub_byte_mem_block metadata;
};


/** Cryptor header version 1.
    
    This is the struct containing the information about the 
    cryptor header version 1. It contains the identifier of the 
    algorithm and the encrypted data length.
 */
struct pubnub_cryptor_header_v1 {
    /** Cryptor algorithm identifier. */
    uint8_t identifier[4];

    /** Encrypted data length. */
    uint32_t data_length;
};


/** Crypto algorithm type.
    
    This is the struct containing the information about the 
    cryptor algorithm type. It contains the identifier of the 
    algorithm and the function pointers to the algorithm implementation.
 */
typedef struct pubnub_crypto_algorithm_t {
    /** Identifier of the algorithm.
        
        Identifier will be encoded into crypto data header and passed along 
        with encrypted data.

        @note Identifier **must** be 4 bytes long.
     */
    uint8_t identifier[4];

    // TODO: return type - int or enum?
    /** Function pointer to the encrypt function.
        
        @param cryptor Pointer to the cryptor structure.
        @param msg Memory block (pointer and size) of the data to encrypt.
        @param base64_str String (allocated by the user) to write encrypted and
                base64 encoded string.
        @param n The size of the string.

        @return 0: OK, -1: error
      */
    int (*encrypt)(struct pubnub_crypto_algorithm_t const *cryptor, struct pubnub_encrypted_data *result, pubnub_bymebl_t to_encrypt);

    // TODO: return type - int or enum?
    /** Function pointer to the decrypt function.
        
        @param cryptor Pointer to the cryptor structure.
        @param base64_str String to Base64 decode and decrypt.
        @param data User allocated memory block to write the decrypted contents to.

        @return >=0: OK (size of the data), -1: error
     */
    int (*decrypt)(struct pubnub_crypto_algorithm_t const *cryptor, pubnub_bymebl_t* result, struct pubnub_encrypted_data to_decrypt);

    /** Pointer to the user data needed for the algorithm. */
    void *user_data;

} pubnub_crypto_algorithm_t;


/** Crypto algorithm wrapper
    
    This is the struct containing the information about the 
    abstract cryptor algorithm. It wraps the algorithm implementation
    and provides the interface to the Pubnub client library.
 */
typedef struct pubnub_cryptor_t {
    /** Cryptor algorithm for data encription / decryption. */
    struct pubnub_crypto_algorithm_t algorithm;

} pubnub_cryptor;


/** Retrieves the cryptor algorithm identifier.
    
    @param cryptor Pointer to the cryptor structure.

    @return Pointer to the cryptor algorithm identifier.
 */
uint8_t const *pubnub_cryptor_identifier(pubnub_cryptor const *cryptor);


// TODO: return type - int or enum?
/** Encrypt provided data.

    @param cryptor Pointer to the cryptor structure.
    @param msg The memory block (pointer and size) of the data to encrypt.
    @param base64_block The char block (pointer and size) to write encrypted and
            base64 encoded string.

    @return 0: OK, -1: error
 */
int pubnub_cryptor_encrypt(pubnub_cryptor const *cryptor, pubnub_bymebl_t const *msg, pubnub_chamebl_t base64_block);


// TODO: return type - int or enum?
/** Decrypt provided data.

    @param cryptor Pointer to the cryptor structure.
    @param base64_block The char block (pointer and size) to Base64 decode and decrypt.
    @param data User allocated memory block to write the decrypted contents to.

    @return 0: OK, -1: error
 */
int pubnub_cryptor_decrypt(pubnub_cryptor const *cryptor, pubnub_chamebl_t const *base64_block, pubnub_bymebl_t *data);


/**
    Prepare the AES CBC algorithm for use.
    It is intended to be used with pubnub crypto module.

    @param cipher_key The cipher key to use.

    @return Pointer to the AES CBC algorithm structure.
*/
struct pubnub_crypto_algorithm_t *pbcc_aes_cbc_init(const uint8_t* cipher_key);


/**
    Prepare the legacy algorithm for use.
    It is intended to be used with pubnub crypto module.

    @param cipher_key The cipher key to use.

    @return Pointer to the legacy algorithm structure.
*/
struct pubnub_crypto_algorithm_t *pbcc_legacy_crypto_init(const uint8_t* cipher_key);

/**
    Prepare the cipher key hash algorithm for use.
    It is intended to be used with pubnub crypto module.

    @param cipher_key The cipher key to hash.
    @param hash The hash of the cipher key returned from function.

    @return 0: OK, -1: error
*/
int pbcc_cipher_key_hash(const uint8_t* cipher_key, uint8_t* hash);


#endif /* PBCC_CRYPTO_H */
//#endif /* PUBNUB_CRYPTO_API */
