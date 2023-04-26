/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PBSHA256
#define	INC_PBSHA256


/** @file pbsha256.h 

    This is the internal "SHA-256" Message Digest API of the Pubnub
    client library.

    In general, this should just defer to some available SHA-256
    implementation (in which case this header might be the only thing
    we write, i.e. a `.c` won't be needed), though it's conceivable
    that we migh provide our own SHA-256 implementation for some platforms
    in the future.

*/

#include <openssl/sha.h>
#if OPENSSL_VERSION_NUMBER < 0x30000000L
/** The SHA-256 "context". It's an "opaque" value type - that is, it's
    to be used as data, not a pointer, but "don't look inside".
*/
#define PBSHA256_CTX SHA256_CTX

/** Initializes the SHA-256 context for a new calculation. 
    @param x Pointer to a SHA-256 context
    @return 0: success, -1: error
 */
#define pbsha256_init(x) SHA256_Init(x) ? 0 : -1

/** Update the SHA-256 context with the "next part of the message".
    @param x Pointer to a SHA-256 context
    @param m Pointer to the start of the "next part of the message"
    @param l Length (in bytes) of the "next part of the message"
    @return 0: success, -1: error
 */
#define pbsha256_update(x, m, l) SHA256_Update((x), (m), (l)) ? 0 : -1

/** Update the SHA-256 context with the "next part of the message".
    Assumes it is an ASCIIZ string.
    @warning This macro uses @p str twice!
    @param x Pointer to a SHA-256 context
    @param str String being the "next part of the message"
    @return 0: success, -1: error
 */
#define pbsha256_update_str(x, str) SHA256_Update ((x), (str), strlen(str)) ? 0 : -1

/** Does the final calculations of the SHA-256 on context @p x and
    stores the digest to @p d.
    @param x Pointer to a SHA-256 context
    @param d Pointer to an array of (at least) 16 bytes
    @return 0: success, -1: error
*/
#define pbsha256_final(x, d) SHA256_Final((d), (x))
#endif // OPENSSL_VERSION_NUMBER < 0x30000000L

/** This helper macro will calculate the SHA-256 on the message
    in @p m, having the length @p l and store it in @p d.
*/
#if __UWP__
#define pbsha256_digest(m, l, d) do                         \
    {                                                       \
        EVP_MD_CTX *ctx;                                    \
        ctx = EVP_MD_CTX_create();                          \
        const EVP_MD* md = EVP_get_digestbyname("SHA256" ); \
        EVP_DigestInit(ctx, EVP_sha256());                  \
        EVP_DigestUpdate(ctx, (m)), (l)));                  \
        EVP_DigestFinal(ctx, (d), NULL);                    \
        EVP_MD_CTX_destroy(ctx);                            \
    } while(0);
#else


#if OPENSSL_VERSION_NUMBER < 0x30000000L
#define pbsha256_digest(m, l, d) do { SHA256_CTX M_ctx_;   \
        SHA256_Init(&M_ctx_);                              \
        SHA256_Update(&M_ctx_, (m), (l));                  \
        SHA256_Final((d), &M_ctx);                         \
    } while (0)
#else
#define pbsha256_digest(m, l, d) do                        \
    {                                                      \
        SHA256((unsigned char *)m, l, d);                  \
    } while (0)
#endif // OPENSSL_VERSION_NUMBER < 0x30000000L
#endif

/** This helper macro will calculate the SHA-256 on the message in @p str,
    assuming it's an ASCIIZ string andd store it in @p d.
    @warning This macro uses @p str twice!
*/
#if __UWP__
#define pbsha256_digest_str(str, d) do                      \
    {                                                       \
        EVP_MD_CTX *ctx;                                    \
        ctx = EVP_MD_CTX_create();                          \
        const EVP_MD* md = EVP_get_digestbyname("SHA256" ); \
        EVP_DigestInit(ctx, EVP_sha256());                  \
        EVP_DigestUpdate(ctx, str, strlen(str));            \
        EVP_DigestFinal(ctx, d, NULL);                      \
        EVP_MD_CTX_destroy(ctx);                            \
    } while(0);
#else
#if OPENSSL_VERSION_NUMBER < 0x30000000L
#define pbsha256_digest_str(str, d) do { SHA256_CTX M_ctx_;   \
        SHA256_Init(&M_ctx_);                                 \
        SHA256_Update(&M_ctx_, (str), strlen(str));           \
        SHA256_Final((d), &M_ctx_);                           \
    } while (0)
#else
#define pbsha256_digest_str(str, d) do                        \
    {                                                         \
        SHA256((unsigned char *)str, strlen(str), d);     \
    } while (0)
#endif // OPENSSL_VERSION_NUMBER < 0x30000000L
#endif

#endif /* !defined INC_PBSHA256 */
