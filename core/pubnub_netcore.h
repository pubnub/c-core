/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_NETCORE
#define      INC_PUBNUB_NETCORE

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
    /** Ready to start a transaction */
    PBS_READY,
    /** Waiting for sending a DNS request */
    PBS_WAIT_DNS_SEND,
    /** Waiting for DNS resolution (response)  */
    PBS_WAIT_DNS_RCV,
    /** Waiting for TCP connection establishment */
    PBS_WAIT_CONNECT,
    /** TCP connected, can start */
    PBS_CONNECTED,
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
    /** Sending the DNS name (part of the URL) */
    PBS_TX_ORIGIN,
    /** Sending the rest of the HTTP headers */
    PBS_TX_FIN_HEAD,
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
    /* Waiting for a cancelled TCP connection to close */
    PBS_WAIT_CANCEL_CLOSE
};


struct pubnub_;

/** The Finite State Machine (FSM) of the net-core module.  It's
    "universal", in the sense that it is portable to all platforms,
    with the porting being done by implementing some functions it
    needs in the PAL (Platform Abstraction Layer).
 */
int pbnc_fsm(struct pubnub_ *pb);


/** Issues a stop command to the FSM of the net-core module. The
	FSM will close a connection (if it is already open) and then
	inform the user of the outcome (as specified) when it is called.

	@note This function will _not_ call pbnc_fsm().
*/
void pbnc_stop(struct pubnub_ *pb, enum pubnub_res outcome_to_report);


#endif  /* !defined INC_PUBNUB_NETCORE */
