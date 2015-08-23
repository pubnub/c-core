/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_INTERNAL_COMMON
#define      INC_PUBNUB_INTERNAL_COMMON

#include "pubnub_config.h"
#include "pubnub_ccore.h"
#include "pubnub_netcore.h"

#if defined(PUBNUB_CALLBACK_API)
#include "pubnub_ntf_callback.h"
#endif

#include <stdint.h>


#if VERBOSE_DEBUG
#include <stdio.h>
#define DEBUG_PRINTF(...) printf(__VA_ARGS__)
#define WATCH(x, fmt) DEBUG_PRINTF(#x " = " fmt "\n", x)
#else
#define DEBUG_PRINTF(...) do { } while(0)
#define WATCH(x, fmt) do { } while(0)
#endif

/*
#if defined(FREERTOS)

typedef xSocket pb_socket_t;

#endif
*/

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


/** The Pubnub context */
struct pubnub_ {
    struct pbcc_context core;

    /** Network communication state */
    enum pubnub_state state;
    /** Type of current transaction */
    enum pubnub_trans trans;


    const uint8_t *sendptr;   /* Pointer to the next data to be sent. */
    uint16_t sendlen;         /* The number of bytes left to be sent. */

    uint8_t *ptr;          /* Pointer to next free byte in the read buffer*/
    uint16_t readlen;         /* The number of bytes left to be read. */

    unsigned short left;   /* Number of bytes left (empty) in the read buffer */

    /** The state of the socket. */
    enum PBSocketState sock_state;   

    unsigned len;          /* Number of bytes to read - given by the user */

#if 0
#if defined(FREERTOS)
    pb_socket_t socket;
#else
    /** Process that started last transaction */
    struct process *initiator;

    uint8_t *readptr;         /* Pointer to the next data to be read. */
#endif
#endif

    struct pubnub_pal pal;

#if PUBNUB_BLOCKING_IO_SETTABLE
    bool use_blocking_io;
#endif

#if PUBNUB_USE_SSL
    struct pubnub_ssl_options {
        /** Should the PubNub client establish the connection to
         * PubNub using SSL? */
        bool use : 1;
        /** When SSL is enabled, should PubNub client ignore all SSL
         * certificate-handshake issues and still continue in SSL mode
         * if it experiences issues handshaking across local proxies,
         * firewalls, etc?
          */
        bool ignore : 1;
        /** When SSL is enabled, should the client fallback to a
         * non-SSL connection if it experiences issues handshaking
         * across local proxies, firewalls, etc?
         */
        bool fallback : 1;
    } ssl;
#endif

#if defined(PUBNUB_CALLBACK_API)
    pubnub_callback_t cb;
    void *user_data;
#endif
};



/** Internal function, to be called when the outcome of a
    REST call / transaction has been reached.
*/
void pbntf_trans_outcome(pubnub_t *pb);

int pbntf_init(void);

int pbntf_got_socket(pubnub_t *pb, pb_socket_t socket);

void pbntf_lost_socket(pubnub_t *pb, pb_socket_t socket);


/** Internal function. Checks if the given pubnub context pointer
    is valid. 
*/
bool pb_valid_ctx_ptr(pubnub_t const *pb);

/** Internal function, only available in the "static" context
    allocator. Gives a context with the given index.
*/
pubnub_t *pballoc_get_ctx(unsigned idx);



#endif /* !defined INC_PUBNUB_INTERNAL_COMMON */
