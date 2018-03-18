/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PBAES256
#define	INC_PBAES256

#include "core/pubnub_memory_block.h"


/** @file pbaes256.h 

    This is the internal "AES-256" Encryption Digest API of the Pubnub
    client library.

    In general, this should just defer to some available AES-256
    implementation (in which case this header might be the only thing
    we write, i.e. a `.c` won't be needed), though it's conceivable
    that we migh provide our own AES-256 implementation for some platforms
    in the future.

*/


/** Encrypt memory block @p msg using @p key and @p iv, allocating
    the encrypted contents and returning it. On error, block pointer
    will be NULL and size is undefined.
*/
pubnub_bymebl_t pbaes256_encrypt_alloc(pubnub_bymebl_t msg, uint8_t const* key, uint8_t const* iv);

/** Similar to pbaes256_encrypt_alloc, but will write the encrypted
    contents to @p encrypted.
*/
int pbaes256_encrypt(pubnub_bymebl_t msg, uint8_t const* key, uint8_t const* iv, pubnub_bymebl_t *encrypted);


/** Decrypt the memory block @p data using @p key and @p iv to  */
int pbaes256_decrypt(pubnub_bymebl_t data, uint8_t const* key, uint8_t const* iv, pubnub_bymebl_t *msg);

/** Similar to pbaes256_decrypt(), but will allocate the memory to
    wite the decrypted contents to and return it. On error, block
    pointer will be NULL and size is undefined.
*/
pubnub_bymebl_t pbaes256_decrypt_alloc(pubnub_bymebl_t data, uint8_t const* key, uint8_t const* iv);


#endif /* !defined INC_PBAES256 */
