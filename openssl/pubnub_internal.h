/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_INTERNAL
#define      INC_PUBNUB_INTERNAL


#include "openssl/bio.h"
#include "openssl/err.h"


typedef BIO * pb_socket_t;

/** The Pubnub OpenSSL context */
struct pubnub_pal {
    BIO *bio;
    SSL_CTX *ctx;
};

/** With OpenSSL, one can set I/O to be blocking or non-blocking,
    though it can only be done before establishing the connection.
*/
#define PUBNUB_BLOCKING_IO_SETTABLE 1

#include "pubnub_internal_common.h"



#endif /* !defined INC_PUBNUB_INTERNAL */
