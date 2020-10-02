/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PBPAL
#define      INC_PBPAL

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>


#include "pubnub_api_types.h"


/** Initializes the Pubnub PAL for the given Pubnub context.
 */
void pbpal_init(pubnub_t *pb);

/** Results that functions for (DNS) resolving and
    connecting can return.
*/
enum pbpal_resolv_n_connect_result {
    pbpal_resolv_resource_failure,
    pbpal_resolv_failed_send,
    pbpal_resolv_send_wouldblock,
    pbpal_resolv_sent,
    pbpal_resolv_failed_rcv,
    pbpal_resolv_rcv_wouldblock,
    pbpal_resolv_failed_processing,
    pbpal_connect_resource_failure,
    pbpal_connect_failed,
    pbpal_connect_wouldblock,
    pbpal_connect_success
};

/** Results that functions for establishing TLS connection
    can return.
*/
enum pbpal_tls_result {
    pbtlsEstablished,
    pbtlsStarted,
    pbtlsInProgress,
    pbtlsResourceFailure,
    pbtlsFailed
};

/* Handles socket condition on given platform */
enum pubnub_res pbpal_handle_socket_condition(int result, pubnub_t* pb, char const* file, int line);

/** Handles start of a TCP (HTTP) connection. It first handles DNS
    resolving for the context @p pb.  If DNS is already resolved, it
    proceeds to establishing TCP connection. Otherwise, will issue a
    DNS request.
    
    Call this function on start of a transaction or on receiving
    response from DNS server.
    
    It also establishes "a link" between the TCP connection (socket,
    or whatever in a given platform) and the Pubnub context.
    
    @note This uses a "non-conventional" interpretation of #PNR_IN_PROGRESS.
    Here it is not an error, but an indication that "DNS resolution is
    in progress", while #PNR_STARTED means "DNS resolved, TCP connect in
    progress". We could have provided another enum for results specific to
    this function, but then would have to map its errors to the generic
    Pubnub errors, which would take time and code.
    
    @param pb The context for which to handle starting of connection
    @return PNR_IN_PROGRESS: DNS not yet resolved, PNR_STARTED: await
    TCP connection, PNR_OK: TCP connected, other: the actual error
*/
enum pbpal_resolv_n_connect_result pbpal_resolv_and_connect(pubnub_t *pb);

/** This checks for the status of a TCP (HTTP) connection
    establishing, which includes the DNS resolution. It's used if this
    wasn't done synchronously during pbpal_resolve_and_connect().
*/
enum pbpal_resolv_n_connect_result pbpal_check_resolv_and_connect(pubnub_t *pb);

#if defined(PUBNUB_CALLBACK_API)
#if PUBNUB_CHANGE_DNS_SERVERS
/** Rotate DNS servers in following order: 
         primary->secondary->default->primary->...
    
    @note This function used to handle case when one of DNS 
    servers experience issues and response on DNS query never
    arrived.
    
    @param pb The context for which DNS servers should rotate.
    @return DNS servers rotate result (0 - success, 1 - failed).
*/
int pbpal_dns_rotate_server(pubnub_t *pb);
#endif /* PUBNUB_CHANGE_DNS_SERVERS */
#endif /* defined(PUBNUB_CALLBACK_API) */

/** Checks whether a TCP connection is established. Call after
    starting a TCP connection (thus, after DNS resolution is over).
*/
enum pbpal_resolv_n_connect_result pbpal_check_connect(pubnub_t *pb);

/** Starts establishing TLS/SSL over existing TCP/IP connection
*/
enum pbpal_tls_result pbpal_start_tls(pubnub_t* pb);

/** Called after 'pbpal_start_tls()'. Does necessary arrangements called repeatedly, if necessary,
    until TLS/SSL platform connection is established, or failed to establish. 
*/
enum pbpal_tls_result pbpal_check_tls(pubnub_t* pb);

/** Sends data over an established connection (with the Pubnub server).
    At one time, only one sending of data can take place.

    If it cannot send all the data, it will send as much as it can.
    You should check if sending was completed by calling pbpal_sent().

    @param pb The context of an established TCP connection
    @param data Pointer to the first octet(byte) of the data to send
    @param n Number of octets (bytes) to send
    @return 0: sent, -1: error: sending already in progress, 
    +1: sending started, not finished
*/
int pbpal_send(pubnub_t *pb, void const *data, size_t n);

/** Helper macro for optimisation of sending of literal strings.
    We know their length, we don't have to call strlen().

    @note There's no way to detect if @p litstr is really a
    literal string, so, handle with care.
 */
#define pbpal_send_literal_str(pb, litstr) \
    pbpal_send((pb), litstr, sizeof litstr - 1)

/** The effect of this is the same as:

    return pbpal_send(pb, s, strlen(s));

    But, it doesn't have to be implemented that way.
*/
int pbpal_send_str(pubnub_t *pb, char const *s);

/** Returns the status of sending. Don't try another
    sending until previous is complete.

    @return 0: sending finished, +1: sending still in progress
    -1: sending failed
*/
int pbpal_send_status(pubnub_t *pb);

/** Starts reading a line from the TCP connection. In other words,
    reading until it finds a newline character. In general, it's used
    to read HTTP response header lines.

    @precondition Previous read (or write) on the context was finished

    @param pb The Pubnub context of the connection

    @return 0: newline read, else: not yet, try again later
*/
int pbpal_start_read_line(pubnub_t *pb);

/** Returns the status of reading a line. Line reading was
    started with pbpal_start_read_line().

    @param pb The Pubnub context to get line reading status for

    @retval PNR_IN_PROGRESS reading a line not over/done yet

    @retval PNR_TX_BUFFER_TOO_SMALL Well, it's not the _TX_ buffer,
    but, it's the same buffer and we don't want to add a new `PNR`
    just for internal purposes. Client may examine the contents of the
    buffer for some further processing and can continue reading (with
    the current buffer contents dropped) by calling
    pbpal_start_read_line().

    @retval PNR_OK Reading a line done (newline char found)

    @retval otherwise The error that happened during the reading
*/
enum pubnub_res pbpal_line_read_status(pubnub_t *pb);

/** Returns the length of the data in the receive buffer
    at this time.
*/
int pbpal_read_len(pubnub_t *pb);

/** Starts reading a given number of octets (bytes) from an
    established TCP connection. Only one reading can take place at any
    given time.

    To check if reading is complete, call pbpal_read_status().

    @precondition Previous read (or write) on the context was finished

    @param pb The Pubnub context of an established TCP connection
    @param n Number of octets (bytes) to read
    @return 0: OK (started), -1: error (reading already started)
*/
int pbpal_start_read(pubnub_t *pb, size_t n);

/** Returns the status of reading a chunk of data. In general, it's
    used to receive the body (or chunk of it) of the HTTP response.

    @param pb The Pubnub context for which to get the current read
    (chunk) status

    @retval PNR_IN_PROGRESS reading a chunk not over/done yet (AKA
    "call me later")

    @retval PNR_OK Reading a chunk done - it _doesn't_ mean that
    all of the `n` octets from the pbpal_start_read() were read.
    It just means that we have read _some_ data and "client" needs
    to use it somehow (copy, probably). The amout of octets 
    actually read client can get by calling pbpal_read_len(). 
    If not all data was read, client should call pbpal_start_read()
    with `n = old n - pbpal_read_len()`.
    
    @retval otherwise The error that happened during the reading
*/
enum pubnub_res pbpal_read_status(pubnub_t *pb);

/** Essentialy prints out an error code detected by the environment
    during transaction, that was going on given @p pb context,
    for debug purposes.
*/ 
void pbpal_report_error_from_environment(pubnub_t* pb, char const* file, int line);

/** Determines an operation outcome depending on socket error occurred
*/
enum pubnub_res pbpal_handle_socket_error(int socket_result,
                                          pubnub_t* pb,
                                          char const* file,
                                          int line);

/** Returns whether for the given Pubnub context the TCP
    connection has closed.
*/
bool pbpal_closed(pubnub_t *pb);

/** Breaks the link between a Pubnub context and the TCP connection.
*/
void pbpal_forget(pubnub_t *pb);

/** Closes (or starts the closing of) the TCP connection of the given
    Pubnub context 

    @return 0: OK, closed; +1: close initiated, call pbpal_closed()
    later to check; -1: error, can't close socket
*/
int pbpal_close(pubnub_t *pb);

/** Sets blocking I/O option on the context for the communication */
int pbpal_set_blocking_io(pubnub_t *pb);

/** Frees-up any resources allocated by the PAL for the given
    context. After this call, context is not safe for use by PAL any
    more (it is assumed it will be freed-up by the caller).
*/
void pbpal_free(pubnub_t *pb);

#if PUBNUB_USE_MULTIPLE_ADDRESSES
struct pubnub_multi_addresses;
void pbpal_multiple_addresses_reset_counters(struct pubnub_multi_addresses* spare_addresses);
#endif /* PUBNUB_USE_MULTIPLE_ADDRESSES */
#endif /* !defined INC_PBPAL */
