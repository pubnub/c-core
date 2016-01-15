/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_INTERNAL
#define      INC_PUBNUB_INTERNAL


#include "openssl/bio.h"
#include "openssl/err.h"


typedef BIO * pb_socket_t;

/** The Pubnub OpenSSL context */
struct pubnub_pal {
    BIO *socket;
    SSL_CTX *ctx;
};

#ifdef _WIN32
#define socket_set_rcv_timeout(socket, milliseconds) do {                       \
    DWORD M_tm = (milliseconds);                                                \
    setsockopt((socket), SOL_SOCKET, SO_RCVTIMEO, (char*)&M_tm, sizeof M_tm);   \
    } while(0)
#else
#include <unistd.h>
#include <sys/socket.h>
#define socket_set_rcv_timeout(socket, seconds) do {                            \
    struct timeval M_tm = { (seconds), 0 };                                     \
    setsockopt((socket), SOL_SOCKET, SO_RCVTIMEO, (char*)&M_tm, sizeof M_tm);   \
    } while(0)
#endif

/** With OpenSSL, one can set I/O to be blocking or non-blocking,
    though it can only be done before establishing the connection.
*/
#define PUBNUB_BLOCKING_IO_SETTABLE 1

#define PUBNUB_TIMERS_API 1

#if _MSC_VER < 1900
/** Microsoft C compiler (before VS2015) does not provide a 
    standard-conforming snprintf(), so we bring our own.
    */
int snprintf(char *buffer, size_t n, const char *format, ...);
#endif

#include "pubnub_internal_common.h"



#endif /* !defined INC_PUBNUB_INTERNAL */
