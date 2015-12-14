/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_INTERNAL
#define      INC_PUBNUB_INTERNAL


#include <unistd.h>
#include <sys/socket.h>
#include "openssl/bio.h"
#include "openssl/err.h"


typedef BIO * pb_socket_t;

/** The Pubnub OpenSSL context */
struct pubnub_pal {
    BIO *socket;
    SSL_CTX *ctx;
};

#define socket_set_rcv_timeout(socket, seconds) do {                            \
    struct timeval M_tm = { (seconds), 0 };                                     \
    setsockopt((socket), SOL_SOCKET, SO_RCVTIMEO, (char*)&M_tm, sizeof M_tm);   \
    } while(0)

/** With OpenSSL, one can set I/O to be blocking or non-blocking,
    though it can only be done before establishing the connection.
*/
#define PUBNUB_BLOCKING_IO_SETTABLE 1

#include "pubnub_internal_common.h"



#endif /* !defined INC_PUBNUB_INTERNAL */
