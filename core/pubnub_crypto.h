/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_CRYPTO
#define	INC_PUBNUB_CRYPTO
#if PUBNUB_CRYPTO_API
#include "lib/pb_deprecated.h"
#include "core/pubnub_api_types.h"
#include "core/pubnub_memory_block.h"
#include "core/pbcc_crypto.h"
#include "lib/pb_extern.h"


/**
    Set cipher key to be used with the Pubnub context 

    @param p The Pubnub context to set cipher key for.

    @return PNR_OK on success, otherwise an error code.
 */
PUBNUB_EXTERN enum pubnub_res pubnub_set_cipher_key(pubnub_t *p, char const *cipher_key);


/** Sets @p secret_key to be used with the Pubnub context @p p.

    The @p secret_key is kept by pointer, so, user needs to make sure
    it remains a valid pointer through the lifetime of the Pubnub
    context @p p.

    @pre p != NULL
    @param p The Pubnub context to set secret key for
    @param secret_key The string of the secret key. Pass NULL to not
    use the secret key (i.e. do no encrpytion/decryption)
    @param PNR_OK or a value indicating an error
 */
PUBNUB_EXTERN enum pubnub_res pubnub_set_secret_key(pubnub_t *p, char const* secret_key);


/** Encrypts a message @p msg to a Base64 encoded @p base64_str, which
    is allocated by the user. The number of bytes allocated for @p base64_str
    is in the @p *n on input. On output *n holds the number of characters
    that were written to @p base64_str.

    This function will allocate memory as needed.

    @pre cipher_key != NULL
    @param cipher_key The key to use when encrypting
    @param msg The memory block (pointer and size) of the data to encrypt
    @param base64_str String (allocated by the user) to write encrypted and
    base64 encoded string
    @param n The size of the string
    @return 0: OK, -1: error
*/
PUBNUB_EXTERN PUBNUB_DEPRECATED int pubnub_encrypt(char const *cipher_key, pubnub_bymebl_t msg, char *base64_str, size_t *n);

/** Similar to pubnub_encrypt() - but this function doesn't allocate
    memory, it uses the memory provided by @p buffer for its "working
    memory".
*/
PUBNUB_EXTERN PUBNUB_DEPRECATED int pubnub_encrypt_buffered(char const *cipher_key, pubnub_bymebl_t msg, char *base64_str, size_t *n, pubnub_bymebl_t buffer);

/** Decrypts a message from a Base64 encoded string @p base64_str to
    user-allocated memory @p data. On input @p data->size holds the
    number of bytes allocated for @p data. On output @p data->size
    holds the number of bytes written to @p data.

    This function will allocate memory as needed.

    Keep in mind that this function doesn't know what is the actual
    contents (pre-encrypting) of the @p base64_str, so it doesn't
    assume it's a string and thus doens't `NUL` terminate it.

    @pre cipher_key != NULL
    @param cipher_key The key to use when decrypting
    @param base64_str String to Base64 decode and decrypt
    @param data User allocated memory block to write the decrypted contents to
    @return 0: OK, -1: error
*/
PUBNUB_EXTERN PUBNUB_DEPRECATED int pubnub_decrypt(char const *cipher_key, char const *base64_str, pubnub_bymebl_t *data);

/** Similar to pubnub_decrypt(), but never allocates memory - it uses
    the memory provided by @p buffer as its "working memory".
*/
PUBNUB_EXTERN PUBNUB_DEPRECATED int pubnub_decrypt_buffered(char const *cipher_key, char const *base64_str, pubnub_bymebl_t *data, pubnub_bymebl_t *buffer);

/** Similar to pubnub_decrpyt(), but this will allocate the memory to
    write the decrypted contents to and return it as the result.

    @param cipher_key The key to use when decrypting
    @param base64_str String to Base64 decode and decrypt
    @result Memory block (pointer and size) of the decoded and decrypted
    message. On failure, pointer will be NULL and size is undefined.
*/
PUBNUB_EXTERN PUBNUB_DEPRECATED pubnub_bymebl_t pubnub_decrypt_alloc(char const *cipher_key, char const *base64_str);


/** Decrypts the next message in the context @p p using the key
    @p cipher_key, puting the decrypted contents to user-allocated
    @p s having size @p n.

    @deprecated it has been deprecated, use `pubnub_set_crypto_module` instead

    The effect of this functions is similar to calling pubnub_get()
    and then pubnub_decrypt().

    In the following situations no encrpytion will be done and an
    error will be returned:

    - If there is no "next" message in @p p,
    - If the next message is not a JSON string
    - If there is an error in Base64 decoding of the message

    Also, if there is an error while decrypting the message, an error
    will be returned. Keep in mind that is _not_ the error of using
    the wrong @p cipher_key, as that cannot be detected.

    Keep in mind that decryption is a CPU intensive task. Another
    point to be made is that it also does some memory management
    (allocting and deallocating).
 */
PUBNUB_EXTERN PUBNUB_DEPRECATED enum pubnub_res pubnub_get_decrypted(pubnub_t *pb, char const* cipher_key, char *s, size_t *n);

/** This function is very similar to pubnub_get_decrypted(), but it
    allocates the (memory for the) decrypted string and returns it as
    its result. It is the caller's responsibility to free() thus
    allocated string.

    @deprecated it has been deprecated, use `pubnub_set_crypto_module` instead

    Thus, usage of this function can be simpler, as the user doesn't
    have to "guess" the size of the message. But, keep in mind that
    you will not be able to tell apart if there was a message and its
    decrypting failed or if there was no message - and, if decrypting
    failed, what was the reason for failure.

    On failure, a NULL pointer is returned.
*/
PUBNUB_EXTERN PUBNUB_DEPRECATED pubnub_bymebl_t pubnub_get_decrypted_alloc(pubnub_t *pb, char const* cipher_key);

/** Publishes the @p message on @p channel in the context @p p
    encrypted with the key @p cipher_key

    @deprecated it has been deprecated, use `pubnub_set_crypto_module` instead

    The effect of this function is similar to:
   
        struct pubnub_publish_options opts =  pubnub_publish_defopts();
        opts.cipher_key = cipher_key;;
        return pubnub_publish_ex(p, channel, message, opts);
*/
PUBNUB_EXTERN PUBNUB_DEPRECATED enum pubnub_res pubnub_publish_encrypted(pubnub_t *p, char const* channel, char const* message, char const* cipher_key);

/** Get the buffer size required to encode an array with the given
    number of bytes.
*/
PUBNUB_EXTERN int base64_max_size(int encode_this_many_bytes);

/** Base64 encoding. Returns non-zero when the provided buffer is too small
    to hold the encoded string.
*/
PUBNUB_EXTERN int base64encode(char* result, int max_size, const void* b64_encode_this, int encode_this_many_bytes);

/** Cryptographic message signing for PAM.

    The caller is responsible for freeing the returned pointer.
*/
PUBNUB_EXTERN char* pn_pam_hmac_sha256_sign(char const* key, char const* message);

PUBNUB_EXTERN enum pubnub_res pn_gen_pam_v2_sign(pubnub_t* p, char const* qs_to_sign, char const* partial_url, char* signature, size_t signature_len);
PUBNUB_EXTERN enum pubnub_res pn_gen_pam_v3_sign(pubnub_t* p, char const* qs_to_sign, char const* partial_url, char const* msg, char* signature, size_t signature_len);

/** 
   Prepare the aes cbc crypto module for use.
   
   This module contains the aes cbc algorithm and the legacy one
   to be used with the pubnub crypto provider.

   It is recommended to use this module instead of the legacy one 
   if you are not using the legacy algorithm yet.

   @param cipher_key The cipher key to use.

   @return Pointer to the aes cbc crypto module structure.

*/
PUBNUB_EXTERN struct pubnub_crypto_provider_t *pubnub_crypto_aes_cbc_module_init(const uint8_t* cipher_key);


/** 
   Prepare the legacy crypto module for use.
   
   This module contains the legacy algorithm and the aes cbc one
   to be used with the pubnub crypto provider.

   @param cipher_key The cipher key to use.

   @return Pointer to the aes cbc crypto module structure.

*/
PUBNUB_EXTERN struct pubnub_crypto_provider_t *pubnub_crypto_legacy_module_init(const uint8_t* cipher_key);


/**
   Prepare the crypto module for use.

   Rhis module contains the alhorithms provided by the user to be used 
   with the pubnub crypto provider.

   @param default Pointer to the default crypto algorithm to use.
   @param algorithms Pointer to the array of crypto algorithms to use.
   @param n_algorithms Number of crypto algorithms in the array. (omit the default one)

   @return Pointer to the crypto module structure.
*/
PUBNUB_EXTERN struct pubnub_crypto_provider_t *pubnub_crypto_module_init(struct pubnub_cryptor_t *default_algorithm, struct pubnub_cryptor_t *algorithms, size_t n_algorithms);


/**
   Set the crypto module to be used by the pubnub context.

   This function sets the crypto module to be used by the pubnub context
   for the encryption and decryption of the messages.

   @param pubnub Pointer to the pubnub context.
   @param crypto_provider Pointer to the crypto provider to use.
*/
PUBNUB_EXTERN void pubnub_set_crypto_module(pubnub_t *pubnub, struct pubnub_crypto_provider_t *crypto_provider);

/**
   Get the crypto module used by the pubnub context.

   This function gets the crypto module used by the pubnub context
   for give a possibility to the user to use the crypto module directly.

   @param pubnub Pointer to the pubnub context.

   @return Pointer to the crypto provider used by the pubnub context.
*/
PUBNUB_EXTERN pubnub_crypto_provider_t *pubnub_get_crypto_module(pubnub_t *pubnub);
#endif // #if PUBNUB_CRYPTO_API
#endif /* defined INC_PUBNUB_CRYPTO */