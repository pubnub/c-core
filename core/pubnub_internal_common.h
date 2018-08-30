/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_INTERNAL_COMMON
#define INC_PUBNUB_INTERNAL_COMMON

#include "pubnub_config.h"
#include "core/pubnub_ccore_pubsub.h"
#include "core/pubnub_netcore.h"
#include "core/pubnub_mutex.h"

#if defined(PUBNUB_CALLBACK_API)
#include "core/pubnub_ntf_callback.h"
#endif

#if !defined(PUBNUB_PROXY_API)
#define PUBNUB_PROXY_API 0
#elif PUBNUB_PROXY_API
#include "core/pubnub_proxy.h"
#include "core/pbhttp_digest.h"
#include "core/pubnub_dns_servers.h"
#endif

#if !defined PUBNUB_RECEIVE_GZIP_RESPONSE
#define PUBNUB_RECEIVE_GZIP_RESPONSE 0
#elif PUBNUB_RECEIVE_GZIP_RESPONSE
#include "core/pbgzip_decompress.h"
#endif

#include <stdint.h>
#if PUBNUB_ADVANCED_KEEP_ALIVE
#include <time.h>
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
    STATE_READ_LINE = 7,
    /** Sending data */
    STATE_SENDING_DATA = 8
};


/** Possible states of NTLM mini-FSM */
enum NPBNTLM_State {
    /** Idle */
    pbntlmIdle,
    /** Waiting to send NEGOTIATE (AKA Type1) message (identity) */
    pbntlmSendNegotiate,
    /** Waiting to recieve CHALLENGE (AKA Type2) message (challenge) */
    pbntlmRcvChallenge,
    /** Waiting to send AUTHENTICATE (AKA Type3) message (challenge response) */
    pbntlmSendAuthenticate,
    /** NTLM authentication is done (may have succeded or not,
        does not matter to us).
     */
    pbntlmDone
};

/** Maximum supported length of the NTLM token (message) */
#define PUBNUB_NTLM_MAX_TOKEN 1024

#if PUBNUB_USE_WIN_SSPI
#define SECURITY_WIN32
#include <sspi.h>
#endif

/** The data of the NTLM mini-FSM.
 */
struct pbntlm_context {
    /** Current state of the NTLM mini-FSM */
    enum NPBNTLM_State state;

    /** Token received in Type2 NTLM message */
    uint8_t in_token[PUBNUB_NTLM_MAX_TOKEN];

    /** The length, in bytes, of the token received in Type2 NTLM
     * message */
    unsigned in_token_size;

#if PUBNUB_USE_WIN_SSPI
    /** The SSPI context handle */
    CtxtHandle hcontext;
    /** The SSPI credentials handle */
    CredHandle hcreds;
    /** The authentication identity (from username and password (and
     domain), if supplied. */
    SEC_WINNT_AUTH_IDENTITY identity;
#endif
};

typedef struct pbntlm_context pbntlm_ctx_t;

/** The Pubnub context

    @note Don't declare any members as `bool`, as there may be
    alignment issues when this is included from both C and C++
    compilers, especially pre-C99 C compilers (like MSVC (at least
    until MSVC 2013)).

    Here's a diagram of the "relationship" between the `ptr`,
    `left` and `unreadlen` with `core.http_buf`:

            ptr
             |  unreadlen  |    left         |
             |<----------->|<--------------->|
             v             |                 |
    +----------------------------------------+
    |          core.http_buf                 |
    +----------------------------------------+

*/
struct pubnub_ {
    struct pbcc_context core;

    /** Network communication state */
    enum pubnub_state state;
    /** Type of current transaction */
    enum pubnub_trans trans;

    /** The number of bytes we got (in our buffer) from network but
        have not processed yet. */
    uint16_t unreadlen;

    /** Pointer to next byte to read from our buffer or next byte to
        send in the user-supplied send buffer.
     */
    uint8_t* ptr;

    /** Number of bytes left (empty) in the read buffer */
    uint16_t left;

    /** The state of the socket. */
    enum PBSocketState sock_state;

    /** Number of bytes to send or read - given by the user */
    unsigned len;

    /** Indicates whether we are receiving chunked or regular HTTP
     * response
     */
    uint16_t http_chunked;

    /** Last received HTTP (result) code */
    uint16_t http_code;

#if defined PUBNUB_ORIGIN_SETTABLE
    char const* origin;
#endif

#if 0
    /** Process that started last transaction */
    struct process *initiator;

    uint8_t *readptr;         /* Pointer to the next data to be read. */
#endif

    struct pubnub_pal pal;

    struct pubnub_options {
#if PUBNUB_BLOCKING_IO_SETTABLE
        /** Indicates whether to use blocking I/O. Not implemented if
            choosing between blocking and non-blocking is not supported
            on a platform.
        */
        bool use_blocking_io : 1;
#endif

        /** Indicates whether to use HTTP keep-alive. If used,
            subsequent transactions will be faster, unless the (TCP/IP
            and TLS/SSL) connection drops. OTOH, the "async"
            drop/close of the connection may be a problem and cause
            some transactions to fail.
        */
        bool use_http_keep_alive : 1;

#if PUBNUB_USE_SSL
        /** Should the PubNub client establish the connection to

         * PubNub using SSL? */
        bool useSSL : 1;
        /** When SSL is enabled, should the client fallback to a
         * non-SSL connection if it experiences issues handshaking
         * across local proxies, firewalls, etc?
         */
        bool fallbackSSL : 1;
        /** Use system certificate store (if available) */
        bool use_system_certificate_store : 1;
        /** Re-use SSL session on a new connection */
        bool reuse_SSL_session : 1;
#endif
    } options;

    struct pubnub_flags {
#if PUBNUB_USE_SSL
        /** Try to establish TLS/SSL over existing TCP/IP connection: yes/no */
        bool trySSL : 1;
#endif
        /** Should close connection */
        bool should_close : 1;
        
        /** Retry the same Pubnub request after closing current TCP
            connection.
        */
        bool retry_after_close : 1;
    } flags;

#if PUBNUB_ADVANCED_KEEP_ALIVE
    struct pubnub_keep_alive_data {
        time_t   timeout;
        time_t   t_connect;
        unsigned max;
        unsigned count;
    } keep_alive;
#endif

#if PUBNUB_RECEIVE_GZIP_RESPONSE
    enum pubnub_data_compressionType data_compressed;
#endif

#if PUBNUB_USE_SSL
    /** Certificate store file */
    char const* ssl_CAfile;
    /** Certificate store directory */
    char const* ssl_CApath;
    /** User-defined, in-memory, PEM certificate to use */
    char const* ssl_userPEMcert;
#endif

#if PUBNUB_THREADSAFE
    pubnub_mutex_t monitor;
#endif

#if PUBNUB_TIMERS_API
    /** Duration of the transaction timeout, in milliseconds */
    int transaction_timeout_ms;

#if defined(PUBNUB_CALLBACK_API)
    struct pubnub_* previous;
    struct pubnub_* next;
    int             timeout_left_ms;
#endif

#endif

#if defined(PUBNUB_CALLBACK_API)
    pubnub_callback_t cb;
    void*             user_data;
#endif

#if PUBNUB_PROXY_API

    /** The type (protocol) of the proxy to use */
    enum pubnub_proxy_type proxy_type;

    /** Hostname (address) of the proxy server to use */
    char proxy_hostname[PUBNUB_MAX_PROXY_HOSTNAME_LENGTH + 1];

    /** Proxy IP address, if and when available through hostname string in 'numbers
        and dots' notation. If proxy IP address is not available structure array is
        filled with zeros.
     */
    struct pubnub_ipv4_address proxy_ip_address;

    /** The (TCP) port to use on the proxy. */
    uint16_t proxy_port;

    /** Indicates whether this is the "first" HTTP request - that is,
        the `CONNECT` one. The first is sent to the proxy, while the
        second (if the first succeeds) is sent to the "real" HTTP
        server (to which the proxy established a "tunnel".
    */
    int proxy_tunnel_established;

    /** The saved path part of the URL for the Pubnub transaction.
     */
    char proxy_saved_path[PUBNUB_BUF_MAXLEN];

    /** The length, in characters, of the saved proxy path */
    unsigned proxy_saved_path_len;

    /** The authentication scheme to use for the proxy - in
        general, it's selected by the proxy.
    */
    enum pubnub_http_authentication_scheme proxy_auth_scheme;

    /** The username to use for proxy authentication */
    char const* proxy_auth_username;

    /** The password to use for proxy authentication */
    char const* proxy_auth_password;

    /** Indicates whether the proxy authorization has already been
        sent.  Some proxy servers will, on failed authorization, ask
        for "second attempt", which we do not support. So, we use this
        flag to know not to send the same authorization again, thus
        avoiding a sort of "endless loop".
    */
    int proxy_authorization_sent;

    /** Data about NTLM authentication */
    struct pbntlm_context ntlm_context;

    /** Data about (HTTP) Digest authentication */
    struct pbhttp_digest_context digest_context;

#endif
};


/** Internal function, to be called when the outcome of a REST call /
    transaction has been reached. The context @p pb will go to @p
    state, which must be such that one can start a new transaction
    from it. The state will be set before any callback is called.
*/
void pbntf_trans_outcome(pubnub_t* pb, enum pubnub_state state);

int pbntf_init(void);

int pbntf_got_socket(pubnub_t* pb);

void pbntf_update_socket(pubnub_t* pb);

void pbntf_lost_socket(pubnub_t* pb);

int pbntf_enqueue_for_processing(pubnub_t* pb);

int pbntf_requeue_for_processing(pubnub_t* pb);

int pbntf_watch_in_events(pubnub_t* pb);
int pbntf_watch_out_events(pubnub_t* pb);


/** Internal function. Checks if the given pubnub context pointer
    is valid.
*/
bool pb_valid_ctx_ptr(pubnub_t const* pb);

/** Internal function, only available in the "static" context
    allocator. Gives a context with the given index.
*/
pubnub_t* pballoc_get_ctx(unsigned idx);

/** Internal function, the "bottom half" of pubnub_free(), which is
    done asynchronously in the callback mode. */
void pballoc_free_at_last(pubnub_t* pb);


#endif /* !defined INC_PUBNUB_INTERNAL_COMMON */
