/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_NETCORE
#define INC_PUBNUB_NETCORE

#include "stdbool.h"

/** @file pubnub_netcore.h

    This is the interface of the Pubnub C-core network module.
    It is internal, users should not include this or use any
    of its definitions.
 */


/** States of a context */
enum pubnub_state {
    /** Context not allocated */
    PBS_NULL,
    /** No transaction ongoing */
    PBS_IDLE,
    /** About to retry the transaction.  Currently used for TLS/SSL 'fallback' to TCP/IP
        and in proxy authentication, where we "retry" after receiving HTTP status
        407, sending (some more) authentication data.
     */
    PBS_RETRY,
#if PUBNUB_USE_RETRY_CONFIGURATION
    /** Request retry timer is active and FSM await for it to timeout. */
    PBS_WAIT_RETRY,
#endif // #if PUBNUB_USE_RETRY_CONFIGURATION
    /** Ready to start a transaction */
    PBS_READY,
    /** Waiting for sending a DNS request */
    PBS_WAIT_DNS_SEND,
    /** Waiting for DNS resolution (response)  */
    PBS_WAIT_DNS_RCV,
    /** Waiting for TCP connection establishment */
    PBS_WAIT_CONNECT,
    /** TCP connected */
    PBS_CONNECTED,
#if PUBNUB_USE_SSL
    /** Waiting for TLS connection establishment */
    PBS_WAIT_TLS_CONNECT,
#endif
    /** Sending HTTP "GET" */
    PBS_TX_GET,
    /** Sending the path (part of the URL) */
    PBS_TX_PATH,
    /** Sending the scheme (part of the URL - the `http:` part) -
     utilized when using the "HTTP GET" proxy */
    PBS_TX_SCHEME,
    /** Sending the host (part of the URL - the `www.pubnub.com` part)
     - utilized when using the "HTTP GET" proxy */
    PBS_TX_HOST,
    /** Sending the "port number" (part of the URL - the `:80` part) -
     utilized when using the "HTTP CONNECT" proxy */
    PBS_TX_PORT_NUM,
    /** Sending the HTTP version */
    PBS_TX_VER,
    /** Sending specific additional HTTP header lines (on proxy authorization,
     or 'publish via POST') */
    PBS_TX_EXTRA_HEADERS,
    /** Sending the DNS name (part of the URL) */
    PBS_TX_ORIGIN,
    /** Sending the rest of the HTTP headers */
    PBS_TX_FIN_HEAD,
    /** Sending the HTTP message body if there is one(exmpl: 'publish' via POST has it.) */
    PBS_TX_BODY,
    /** Waiting for HTTP version in response */
    PBS_RX_HTTP_VER,
    /** Reading the HTTP response headers */
    PBS_RX_HEADERS,
    /** Reading one HTTP response header line */
    PBS_RX_HEADER_LINE,
    /** Reading the HTTP response body */
    PBS_RX_BODY,
    /** Waiting for new data in HTTP response body */
    PBS_RX_BODY_WAIT,
    /** Reading the length of the chunk in HTTP chunked response body */
    PBS_RX_CHUNK_LEN,
    /** Waiting to receive whole line of the chunk length*/
    PBS_RX_CHUNK_LEN_LINE,
    /** Reading the chunk in HTTP chunked response body */
    PBS_RX_BODY_CHUNK,
    /** Waiting for new data in HTTP chunked response body */
    PBS_RX_BODY_CHUNK_WAIT,
    /** Waiting for the TCP connection to close */
    PBS_WAIT_CLOSE,
    /** Waiting to cancel (close before the end of transaction) the
     * TCP connection (in some TCP/IP implementations, we don't want to
     * cancel right away, but wait).
     */
    PBS_WAIT_CANCEL,
    /** Waiting for a cancelled TCP connection to close */
    PBS_WAIT_CANCEL_CLOSE,
    /** HTTP Heep-Alive active, we're in IDLE, waiting for the user to
        start a new stransaction, while keeping the TCP/IP (and
        TLS/SSL) connection "open".
     */
    PBS_KEEP_ALIVE_IDLE,
    /** HTTP Heep-Alive active, user has requested a new transaction,
        we're ready to go.
     */
    PBS_KEEP_ALIVE_READY,
    /** HTTP Heep-Alive was active, we started to close the connection and
        are awating for it to be actually closed before starting the transaction
        via a new connection.
     */
    PBS_KEEP_ALIVE_WAIT_CLOSE,
    /** Waiting to cancel (close 'connection'/socket) if DNS fails due
     * to timeout, when we want to try again (but first close the
     * socket, to have a fresh start, maybe it failed because the
     * socket became corrupt).
     */
    PBS_WAIT_CANCEL_DNS,
    /** Waiting to cancel (close connection) if the transaction timeout
     * ocurred while in HTTP Keep-Alive.
     */
    PBS_WAIT_CANCEL_KEEPALIVE
};


struct pubnub_;

/** The Finite State Machine (FSM) of the net-core module.  It's
    "universal", in the sense that it is portable to all platforms,
    with the porting being done by implementing some functions it
    needs in the PAL (Platform Abstraction Layer).
 */
int pbnc_fsm(struct pubnub_* pb);


/** Issues a stop command to the FSM of the net-core module. The
    FSM will close a connection (if it is already open) and then
    inform the user of the outcome (as specified) when it is called.

    @note This function will _not_ call pbnc_fsm().
*/
void pbnc_stop(struct pubnub_* pb, enum pubnub_res outcome_to_report);


/** Returns whether it's OK to start a new transaction. The FSM needs
    to be in an "idle" state. In general, that means that either there
    is no current connection to the Pubnub network (server), or there
    is one, but it's "idle" (previous transaction completed).

    @retval true Can start a new transaction
    @retval false Cannot start a new transaction (await the finish
    of current one)
 */
bool pbnc_can_start_transaction(struct pubnub_ const* pbp);


#endif /* !defined INC_PUBNUB_NETCORE */
