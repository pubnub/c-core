/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_INTERNAL
#define      INC_PUBNUB_INTERNAL


#include "openssl/bio.h"
#include "openssl/err.h"


typedef BIO * pb_socket_t;

/** The Pubnub POSIX context */
struct pubnub_pal {
    BIO *bio;
    SSL_CTX *ctx;
};

#include "pubnub_internal_common.h"



#endif /* !defined INC_PUBNUB_INTERNAL */
