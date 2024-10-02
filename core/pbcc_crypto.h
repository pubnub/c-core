/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#ifndef PBCC_CRYPTO_H
#define PBCC_CRYPTO_H

/** @file pbcc_crypto.h

    This has the functions for crypto related to the PubNub C client.
    It contains functions used by official PubNub C client API
    and some abstract functions that mey be need to be implemented by the user.
*/

#include "pubnub_ccore_pubsub.h"
#include "pubnub_memory_block.h"
#include <stdint.h>

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

#define PUBNUB_CRYPTOR_HEADER_IDENTIFIER_SIZE 4
#define PUBNUB_MAXIMUM_HEADER_VERSION 1
static char PUBNUB_LEGACY_CRYPTO_IDENTIFIER[4] = { 0, 0, 0, 0 };

/** Cryptor header version 1.
    
    This is the struct containing the information about the 
    cryptor header version 1. It contains the identifier of the 
    algorithm and the encrypted data length.
 */
struct pubnub_cryptor_header_v1 {
    /** Cryptor algorithm identifier. */
    uint8_t identifier[PUBNUB_CRYPTOR_HEADER_IDENTIFIER_SIZE];

    /** Encrypted data length. */
    uint32_t data_length;
};


/** Crypto algorithm type.
    
    This is the struct containing the information about the 
    cryptor algorithm type. It contains the identifier of the 
    algorithm and the function pointers to the algorithm implementation.
 */
typedef struct pubnub_cryptor_t {
    /** Identifier of the algorithm.
        
        Identifier will be encoded into crypto data header and passed along 
        with encrypted data.

        @note Identifier **must** be 4 bytes long.
     */
    uint8_t identifier[4];

    /** Function pointer to the encrypt function.
        
        @param cryptor Pointer to the cryptor structure.
        @param msg Memory block (pointer and size) of the data to encrypt.
        @param base64_str String (allocated by the user) to write encrypted and
                base64 encoded string.
        @param n The size of the string.

        @return 0: OK, -1: error
      */
    int (*encrypt)(struct pubnub_cryptor_t const *cryptor, struct pubnub_encrypted_data *result, pubnub_bymebl_t to_encrypt);

    /** Function pointer to the decrypt function.
        
        @param cryptor Pointer to the cryptor structure.
        @param base64_str String to Base64 decode and decrypt.
        @param data User allocated memory block to write the decrypted contents to.

        @return >=0: OK (size of the data), -1: error
     */
    int (*decrypt)(struct pubnub_cryptor_t const *cryptor, pubnub_bymebl_t* result, struct pubnub_encrypted_data to_decrypt);

    /** Pointer to the user data needed for the algorithm. */
    void *user_data;

} pubnub_cryptor_t;


/** Crypto Provider.
    
    This is the struct containing the information about the 
    cryptor provider. 

    It should use the `pubnub_cryptor_t` struct to provide 
    the algorithm implementation and select which one to use.
*/
typedef struct pubnub_crypto_provider_t {
    /** Function pointer to the encrypt function.
        
        @param provider Pointer to the provider structure.
        @param to_encrypt Memory block (pointer and size) of the data to encrypt.

        @return Encrypted data block.
      */
    pubnub_bymebl_t (*encrypt)(struct pubnub_crypto_provider_t const *provider, pubnub_bymebl_t to_encrypt);

    /** Function pointer to the decrypt function.
        
        @param cryptor Pointer to the cryptor structure.
        @param to_decrypt Memory block (pointer and size) of the data to decrypt.

        @return Decrypted data block.
     */
    pubnub_bymebl_t (*decrypt)(struct pubnub_crypto_provider_t const *provider, pubnub_bymebl_t to_decrypt);


    /** Pointer to the user data needed for the provider. */
    void *user_data;
} pubnub_crypto_provider_t;


/**
    Prepare the AES CBC algorithm for use.
    It is intended to be used with pubnub crypto module.

    @param cipher_key The cipher key to use.

    @return Pointer to the AES CBC algorithm structure.
*/
struct pubnub_cryptor_t *pbcc_aes_cbc_init(const uint8_t* cipher_key);


/**
    Prepare the legacy algorithm for use.
    It is intended to be used with pubnub crypto module.

    @param cipher_key The cipher key to use.

    @return Pointer to the legacy algorithm structure.
*/
struct pubnub_cryptor_t *pbcc_legacy_crypto_init(const uint8_t* cipher_key);

/**
    Prepare the cipher key hash algorithm for use.
    It is intended to be used with pubnub crypto module.

    @param cipher_key The cipher key to hash.
    @param hash The hash of the cipher key returned from function.

    @return 0: OK, -1: error
*/
int pbcc_cipher_key_hash(const uint8_t* cipher_key, uint8_t* hash);

/**
    Prepare the data header v1 for crypto identification. 

    @param identifier The identifier of the algorithm.
    @param metadata The metadata of the algorithm.

    @return The data header v1.
*/
struct pubnub_cryptor_header_v1 pbcc_prepare_cryptor_header_v1(uint8_t identifier[4], struct pubnub_byte_mem_block metadata);

/**
    Size of the data header v1 in bytes.

    @param cryptor_header The data header v1.

    @return The number of bytes needed to include the data header v1.
*/
size_t pbcc_cryptor_header_v1_size(struct pubnub_cryptor_header_v1 *cryptor_header);

/**
    Transform the data header v1 to the byte memory block.

    @param cryptor_header The data header v1.

    @return The byte memory block containing the data header v1. 
*/
struct pubnub_byte_mem_block* pbcc_cryptor_header_v1_to_alloc_block(struct pubnub_cryptor_header_v1 *cryptor_header); 

/** 
    Transform the byte memory block to the data header v1.

    @param cryptor_header The byte memory block containing the data header v1.

    @return The data header v1.
*/
struct pubnub_cryptor_header_v1* pbcc_cryptor_header_v1_from_block(struct pubnub_byte_mem_block *cryptor_header);
    
/**
    Encrypts data with legacy algorithm without encoding it to base64.

    This function behaves the same as a old `pubnub_encrypt` function, but it doesn't
    encode the encrypted data to base64. It allocates the memory for the encrypted data 
    and returns it to the user. 
    It is meant to be used within pubnub crypto module.

    @pre cipher_key != NULL

    @param cipher_key The key to use when encrypting
    @param msg The memory block (pointer and size) of the data to encrypt
    @param str String (allocated by the user) to write encrypted 
    @param n The size of the string

    @return encrypted data block
*/
pubnub_bymebl_t pbcc_legacy_encrypt(uint8_t const* cipher_key, pubnub_bymebl_t msg);

/**
    Decrypts data with legacy algorithm without decoding it from base64.

    This function behaves the same as a old `pubnub_decrypt` function, but it doesn't
    decode the encrypted data from base64. It allocates the memory for the decrypted data
    and returns it to the user.
    It is meant to be used within pubnub crypto module.

    @pre cipher_key != NULL

    @param cipher_key The key to use when decrypting
    @param to_decrypt The memory block (pointer and size) of the data to decrypt
    
    @return decrypted data block
*/
int pbcc_legacy_decrypt(uint8_t const* cipher_key, pubnub_bymebl_t *result, pubnub_bymebl_t to_decrypt);

/**
    Encodes the encrypted data to base64.

    This function encodes the encrypted data to base64. It allocates the memory for the 
    encoded data and returns it to the user.

    @param buffer The encrypted data to encode 

    @return The encoded string or NULL on error
*/
const char* pbcc_base64_encode(pubnub_bymebl_t buffer);

/** 
    Decodes the encrypted data from base64.

    This function decodes the encrypted data from base64. It allocates the memory for the 
    decoded data and returns it to the user.

    @param buffer The encrypted data to decode 

    @return The decoded string or NULL on error
*/
pubnub_bymebl_t pbcc_base64_decode(const char* buffer);


/**
   Set the crypto module to be used by the pubnub context.

   This function sets the crypto module to be used by the pubnub context
   for the encryption and decryption of the messages.

   @param pubnub Pointer to the pubnub context.
   @param crypto_provider Pointer to the crypto provider to use.
*/
void pbcc_set_crypto_module(struct pbcc_context *ctx, struct pubnub_crypto_provider_t *crypto_provider);

/**
   Get the crypto module used by the pubnub context.

   This function gets the crypto module used by the pubnub context 
   for the encryption and decryption of the messages directly.

   @param pubnub Pointer to the pubnub context.

   @return Pointer to the crypto provider used by the pubnub context.
*/
pubnub_crypto_provider_t *pbcc_get_crypto_module(struct pbcc_context *ctx);

#if PUBNUB_CRYPTO_API 
/**
   Decrypt the message received from PubNub with the crypto module.

   This function takes the message received from PubNub and decrypts it 
   for the user. It uses the crypto module used by the pubnub context.

   @param pubnub Pointer to the pubnub context.
   @param message The message received from PubNub.

   @return The decrypted message or NULL on error.
*/
const char* pbcc_decrypt_message(struct pbcc_context *ctx, const char* message, size_t len, size_t* out_len);
#endif /* PUBNUB_CRYPTO_API */
   
    
#endif /* PBCC_CRYPTO_H */
