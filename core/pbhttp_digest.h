/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#if PUBNUB_PROXY_API

#if !defined INC_PBHTTP_DIGEST
#define      INC_PBHTTP_DIGEST

#include "pubnub_memory_block.h"


/** Maximum length of the "nonce" field used in HTTP authentication
    headers. Server sets this.
*/
#define PUBNUB_MAX_HTTP_NONCE 63

/** Maximum length of the "cnonce" (client nonce) field used in HTTP
    authentication headers. We generate this.
*/
#define PUBNUB_MAX_HTTP_CLIENT_NONCE 63

/** Maximum length of the "opaque" field used in HTTP authentication
    headers, including the quotes, which are kept for convenience.
*/
#define PUBNUB_MAX_HTTP_OPAQUE 63

/** HTTP Digest hashing algorithm */
enum pbhttp_digest_algorithm {
    /** MD5 */
    pbhtdigalMD5,
    /** MD5 "session" */
    pbhtdigalMD5_sess,
    /** SHA-256 is not yet supported, it's a placeholder, as it's
	defined by RFC7616. */
    pbhtdigalSHA256,
    /** SHA-256 ("session") is not yet supported, it's a placeholder, as it's
	defined by RFC7616. */
    pbhtdigalSHA256_sess,
    /** SHA-512/256 is not yet supported, it's a placeholder, as it's
	defined by RFC7616. */
    pbhtdigalSHA512256,
    /** SHA-512/256 ("session") is not yet supported, it's a
	placeholder, as it's defined by RFC7616. */
    pbhtdigalSHA512256_sess,
    /** An unknown algorithm implies "ignore the challenge" */
    pbhtdigalUnknown
};

/** HTTP Digest Quality of Protection options */
enum pbhttp_digest_qop {
    /** No Quality of Protection - IOW, don't use `cnonce` (client
	nonce). */
    pbhtdigqopNone,
    /** Authentication Quality of Protection */
    pbgtdigqop_auth,
    /** Authentication w/integrity Quality of Protection - IOW include
	the body in the calculated response.
     */
    pbgtdigqop_auth_int
};

/** HTTP Digest authentication header parsing result */
enum pbhttp_digest_parse_header_rslt {
    /** Authentication parameter(one, or more) is invalid */ 
    pbhtdig_ParameterError,
    /** Realm in new 'authentication required' message is different from
        realm previously used
     */
    pbhtdig_DifferentConsecutiveRealms,
    /** Realm in new 'authentication required' message is the same as
        realm currently in use
     */
    pbhtdig_EqualConsecutiveRealms,
    /** attribute 'realm' is not found yet in 'authentication required' message header
     */
    pbhtdig_RealmNotFound
};    

/** 
 */
struct pbhttp_digest_context {
    /** The auth challenge nonce - received from the server */
    char nonce[PUBNUB_MAX_HTTP_NONCE + 1];

    /** The opaque data of the challenge - received from the server */
    char opaque[PUBNUB_MAX_HTTP_OPAQUE + 1];

    /** The hashing algorithm to use. Server can give several options,
        we choose one.
     */
    enum pbhttp_digest_algorithm algorithm;

    /** Quality of Protection - received from the server */
    enum pbhttp_digest_qop qop;

    /** The auth client nonce - we generate this */
    char client_nonce[PUBNUB_MAX_HTTP_CLIENT_NONCE + 1];

    /** Nonce usage counter. Since we don't want to have some global
        state, we will not keep a global usage counter, nor record of
        nonces used. It is valid only withing one context and only
        "until a nonce is received from the server".
    */
    uint32_t nc;
};


/** Initializes the HTTP Digest authentication context */
void pbhttp_digest_init(struct pbhttp_digest_context *ctx);

/** Parse a HTTP header pertaining to HTTP digest proxy
    authentication. It expects only the "key-value" pairs, not the
    "Proxy-Authenticate: Digest" prefix.

    @param ctx The HTTP digest context
    @param header The string of the header (key-value pairs)
    @param realm pointer to the corresponding pubnub context field to be filled in
    @retvel pbhtdig_ParameterError invalid parameter found while parsing
    @retval pbhtdig_EqualConsecutiveRealms realm parsed from the header is equal as
                                           one already in use
    @retval pbhtdig_DifferentConsecutiveRealms new realm is different from the one previously
                                               used,
    @retval pbhtdig_RealmNotFound realm is not discovered(yet) in digest 'auth-info' header line
 */
enum pbhttp_digest_parse_header_rslt pbhttp_digest_parse_header(struct pbhttp_digest_context *ctx,
                                                                char const* header,
                                                                char* realm);

/** Sets the contents of the string buffer to send as the header
    during HTTP Digest authentication.

    @param ctx The HTTP digest context
    @param username The username to use
    @param password The password to use
    @param uri The URI to use
    @param realm The URI realm to use
    @param buf The buffer which contents to set
    @param 0: OK, -1: error
 */
int pbhttp_digest_prep_header_to_send(struct pbhttp_digest_context *ctx,
                                      char const* username,
                                      char const* password,
                                      char const* uri,
                                      char const* realm,
                                      pubnub_chamebl_t *buf);

/** Returns a read-only string representation of the HTTP Digest
    Quality of Protection value.
*/
char const* pbhttp_digest_qop2str(enum pbhttp_digest_qop e);

/** Returns a read-only string representation of the HTTP digest
    (Hash) algorithm value.
*/
char const* pbhttp_digest_algorithm2str(enum pbhttp_digest_algorithm e);


#endif /* !defined INC_PBHTTP_DIGEST */
#endif /* PUBNUB_PROXY_API */

