/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_CRYPTO
#define	INC_PUBNUB_CRYPTO


#include "pubnub_api_types.h"
#include "pubnub_memory_block.h"


/** @file pubnub_crypto.h 

    This is the "Crypto" API of the Pubnub client library.  It enables
    encryptyng and decrypting messages being sent/received (published/
    subscribed) to/from Pubnub.

    This is independent from using SSL/TLS - if supported on a
    platform, one can encrypt/decrypt with or without using SSL/TLS.

*/


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
enum pubnub_res pubnub_set_secret_key(pubnub_t *p, char const* secret_key);


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
int pubnub_encrypt(char const *cipher_key, pubnub_bymebl_t msg, char *base64_str, size_t *n);

/** Similar to pubnub_encrypt() - but this function doesn't allocate
    memory, it uses the memory provided by @p buffer for its "working
    memory".
*/
int pubnub_encrypt_buffered(char const *cipher_key, pubnub_bymebl_t msg, char *base64_str, size_t *n, pubnub_bymebl_t buffer);

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
int pubnub_decrypt(char const *cipher_key, char const *base64_str, pubnub_bymebl_t *data);

/** Similar to pubnub_decrypt(), but never allocates memory - it uses
    the memory provided by @p buffer as its "working memory".
*/
int pubnub_decrypt_buffered(char const *cipher_key, char const *base64_str, pubnub_bymebl_t *data, pubnub_bymebl_t *buffer);

/** Similar to pubnub_decrpyt(), but this will allocate the memory to
    write the decrypted contents to and return it as the result.

    @param cipher_key The key to use when decrypting
    @param base64_str String to Base64 decode and decrypt
    @result Memory block (pointer and size) of the decoded and decrypted
    message. On failure, pointer will be NULL and size is undefined.
*/
pubnub_bymebl_t pubnub_decrypt_alloc(char const *cipher_key, char const *base64_str);


/** Decrypts the next message in the context @p p using the key
    @p cipher_key, puting the decrypted contents to user-allocated
    @p s having size @p n.

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
enum pubnub_res pubnub_get_decrypted(pubnub_t *pb, char const* cipher_key, char *s, size_t *n);

/** This function is very similar to pubnub_get_decrypted(), but it
    allocates the (memory for the) decrypted string and returns it as
    its result. It is the caller's responsibility to free() thus
    allocated string.

    Thus, usage of this function can be simpler, as the user doesn't
    have to "guess" the size of the message. But, keep in mind that
    you will not be able to tell apart if there was a message and its
    decrypting failed or if there was no message - and, if decrypting
    failed, what was the reason for failure.

    On failure, a NULL pointer is returned.
*/
pubnub_bymebl_t pubnub_get_decrypted_alloc(pubnub_t *pb, char const* cipher_key);

/** Publishes the @p message on @p channel in the context @p p
    encrypted with the key @p cipher_key

    The effect of this function is similar to:
   
        struct pubnub_publish_options opts =  pubnub_publish_defopts();
        opts.cipher_key = cipher_key;;
        return pubnub_publish_ex(p, channel, message, opts);
*/
enum pubnub_res pubnub_publish_encrypted(pubnub_t *p, char const* channel, char const* message, char const* cipher_key);


#endif /* defined INC_PUBNUB_PROXY */
