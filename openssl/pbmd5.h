/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PBMD5
#define	INC_PBMD5


/** @file pbmd5.h 

    This is the internal "MD5" Message Digest API of the Pubnub client
    library.

    In general, this should just defer to some available MD5
    implementation (in which case this header might be the only thing
    we write, i.e. a `.c` won't be needed), though it's conceivable
    that we migh provide our own MD5 implementation for some platforms
    in the future.

    To support older OpenSSL versions, we don't examine the return value - 
    because there was no return value in them (function was void).
*/

#include <openssl/md5.h>

/** The MD5 "context". It's an "opaque" value type - that is, it's to
    be used as data, not a pointer, but "don't look inside".
*/
#define PBMD5_CTX MD5_CTX

/** Initializes the MD5 context for a new calculation. 
    @param x Pointer to a MD5 context
 */
#define pbmd5_init(x) MD5_Init(x)

/** Update the MD5 context with the "next part of the message".
    @param x Pointer to a MD5 context
    @param m Pointer to the start of the "next part of the message"
    @param l Length (in bytes) of the "next part of the message"
 */
#define pbmd5_update(x, m, l) MD5_Update((x), (m), (l))

/** Update the MD5 context with the "next part of the message".
    Assumes it is an ASCIIZ string.
    @warning This macro uses @p str twice!
    @param x Pointer to a MD5 context
    @param str String being the "next part of the message"
 */
#define pbmd5_update_str(x, str) MD5_Update((x), (str), strlen(str))

/** Does the final calculations of the MD5 on context @p x and
    stores the digest to @p d.
    @param x Pointer to a MD5 context
    @param d Pointer to an array of (at least) 16 bytes
*/
#define pbmd5_final(x, d) MD5_Final((d), (x))

/** This helper macro will calculate the MD5 on the message
    in @p m, having the length @p l and store it in @p d.
*/
#define pbmd5_digest(m, l, d) do { MD5_CTX M_ctx_;      \
        MD5_Init(&M_ctx_);                              \
        MD5_Update(&M_ctx_, (m), (l));                  \
        MD5_Final((d), &M_ctx);                         \
    } while (0)

/** This helper macro will calculate the MD5 on the message in @p str,
    assuming it's an ASCIIZ string andd store it in @p d.
    @warning This macro uses @p str twice!
*/
#define pbmd5_digest_str(str, d) do { MD5_CTX M_ctx_;   \
        MD5_Init(&M_ctx_);                              \
        MD5_Update(&M_ctx_, (str), strlen(str));        \
        MD5_Final((d), &M_ctx);                         \
    } while (0)

#endif /* !defined INC_PBMD5 */
