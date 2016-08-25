/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_INTERNAL_COMMON
#define      INC_PUBNUB_INTERNAL_COMMON

#include "pubnub_config.h"
#include "pubnub_ccore.h"
#include "pubnub_netcore.h"
#include "pubnub_mutex.h"

#if defined(PUBNUB_CALLBACK_API)
#include "pubnub_ntf_callback.h"
#endif

#if !defined PUBNUB_PROXY_API
#define PUBNUB_PROXY_API 0
#elif PUBNUB_PROXY_API
#include "pubnub_proxy.h"
#endif

#include <stdint.h>


#if !defined PUBNUB_USE_ADNS
#define PUBNUB_USE_ADNS 0
#endif


/** State of a Pubnub socket. Some states are specific to some
    PALs.
 */
enum PBSocketState {
    /** Socket idle - unused */
    STATE_NONE = 0,
    /** ACK received on the socket */
    STATE_ACKED = 1,
    /** Reading a number of octets */
    STATE_READ = 2,
    /** Block reading of new data, even though we have an indicator
        that there is data to read.
    */
    STATE_BLOCKED_NEWDATA = 3,
    /** All of the data that has arrived has been read */
    STATE_NEWDATA_EXHAUSTED = 4,
    /** All of the data that was to be sent has been sent. */
    STATE_DATA_SENT = 6,
    /** Reading a line */
    STATE_READ_LINE = 7
};


/** The Pubnub context 

    @note Don't declare any members as `bool`, as there may be
    alignment issues when this is included from both C and C++
    compilers, especially pre-C99 C compilers (like MSVC (at least
    until MSVC 2013)).
*/
struct pubnub_ {
    struct pbcc_context core;

    /** Network communication state */
    enum pubnub_state state;
    /** Type of current transaction */
    enum pubnub_trans trans;

    /** Pointer to the next data to be sent. */
    uint8_t const *sendptr;   

    /** The number of bytes left to be sent. */
    uint16_t sendlen;         

    /** The number of bytes left to be read. */
    uint16_t readlen;         

    /** Pointer to next free byte in the read buffer*/
    uint8_t *ptr;          

    /** Number of bytes left (empty) in the read buffer */
    uint16_t left;   

    /** The state of the socket. */
    enum PBSocketState sock_state;   

    /** Number of bytes to read - given by the user */
    unsigned len;          

    /** Indicates whether we are receiving chunked or regular HTTP
     * response
     */
    uint16_t http_chunked;

    /** Last received HTTP (result) code */
    uint16_t http_code;

#if defined PUBNUB_ORIGIN_SETTABLE
    char const *origin;
#endif

#if 0
    /** Process that started last transaction */
    struct process *initiator;

    uint8_t *readptr;         /* Pointer to the next data to be read. */
#endif

    struct pubnub_pal pal;

    struct pubnub_options {
        /** Indicates whether to use blocking I/O. Ignored if 
            choosing between blocking and non-blocking is not supported
            on a platform. Would be ifdef-ed out, but then it would be
            possible for this struct to have no members which is 
            prohibited by the ISO C standard.
        */
        bool use_blocking_io : 1;

#if PUBNUB_USE_SSL
        /** Should the PubNub client establish the connection to
         * PubNub using SSL? */
        bool useSSL : 1;
        /** When SSL is enabled, should PubNub client ignore all SSL
         * certificate-handshake issues and still continue in SSL mode
         * if it experiences issues handshaking across local proxies,
         * firewalls, etc?
          */
        bool ignoreSSL : 1;
        /** When SSL is enabled, should the client fallback to a

         * non-SSL connection if it experiences issues handshaking
         * across local proxies, firewalls, etc?
         */
        bool fallbackSSL : 1;
#endif
    } options;

#if PUBNUB_THREADSAFE
    pubnub_mutex_t monitor;
#endif

#if PUBNUB_TIMERS_API
    /** Duration of the transaction timeout, in milliseconds */
    int transaction_timeout_ms;

#if defined(PUBNUB_CALLBACK_API)
    struct pubnub_ *previous;
    struct pubnub_ *next;
    int timeout_left_ms;
#endif

#endif

#if defined(PUBNUB_CALLBACK_API)
    pubnub_callback_t cb;
    void *user_data;
#endif

#if PUBNUB_PROXY_API

    /** The type (protocol) of the proxy to use */
    enum pubnub_proxy_type proxy_type;

    /** Hostname (address) of the proxy server to use */
    char proxy_hostname[PUBNUB_MAX_PROXY_HOSTNAME_LENGTH + 1];

    /** The (TCP) port to use on the proxy. */
    uint16_t proxy_port;

    /** Indicates whether this is the "first" HTTP request - that is,
        the `CONNECT` one. The first is sent to the proxy, while the
        second (if the first succeeds) is sent to the "real" HTTP
        server (to which the proxy established a "tunnel".
    */
    bool proxy_tunnel_established;

    /** The saved path part of the URL for the Pubnub transaction.
     */
    char proxy_saved_path[PUBNUB_BUF_MAXLEN];

#endif
};



/** Internal function, to be called when the outcome of a
    REST call / transaction has been reached.
*/
void pbntf_trans_outcome(pubnub_t *pb);

int pbntf_init(void);

int pbntf_got_socket(pubnub_t *pb, pb_socket_t socket);

void pbntf_update_socket(pubnub_t *pb, pb_socket_t socket);

void pbntf_lost_socket(pubnub_t *pb, pb_socket_t socket);

int pbntf_enqueue_for_processing(pubnub_t *pb);

int pbntf_requeue_for_processing(pubnub_t *pb);

int pbntf_watch_in_events(pubnub_t *pb);
int pbntf_watch_out_events(pubnub_t *pb);


/** Internal function. Checks if the given pubnub context pointer
    is valid. 
*/
bool pb_valid_ctx_ptr(pubnub_t const *pb);

/** Internal function, only available in the "static" context
    allocator. Gives a context with the given index.
*/
pubnub_t *pballoc_get_ctx(unsigned idx);

/** Internal function, the "bottom half" of pubnub_free(), which is 
    done asynchronously in the callback mode. */
void pballoc_free_at_last(pubnub_t *pb);


#endif /* !defined INC_PUBNUB_INTERNAL_COMMON */
