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
    @return PNR_IN_PROGRESS: DNS not yet resolved, PNR_STARTED: await TCP connection, 
	PNR_OK: TCP connected, other: the actual error
*/
enum pubnub_res pbpal_resolv_and_connect(pubnub_t *pb);


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
 */
#define pbpal_send_literal_str(pb, litstr) {          \
        uint8_t s_[] = litstr;                        \
        pbpal_send((pb), s_, sizeof s_ - 1); }

/** The effect of this is the same as:

    return pbpal_send(pb, s, strlen(s));

    But, it doesn't have to be implemented that way.
*/
int pbpal_send_str(pubnub_t *pb, char const *s);

/** Returns whether sending of data is done. Don't try another
    sending until previous is complete.
*/
bool pbpal_sent(pubnub_t *pb);

/** Starts reading a line from the TCP connection. In other words,
    reading until it finds a newline character.

    @param pb The Pubnub context of the connection 
    @return 0: newline read, else: not yet, try again later
*/
int pbpal_start_read_line(pubnub_t *pb);

/** Returns whether a line was read. Line reading was
    started with pbpal_start_read_line().
*/
bool pbpal_line_read(pubnub_t *pb);

/** Returns the length of the data in the receive buffer
    at this time.
*/
int pbpal_read_len(pubnub_t *pb);

/** Starts reading a given number of octets (bytes) from an
    established TCP connection. Only one reading can take place at any
    given time.

    To check if reading is complete, call pbpal_read_over().

    @param pb The Pubnub context of an established TCP connection
    @param n Number of octets (bytes) to read
    @return 0: OK (started), -1: error (reading already started)
*/
int pbpal_start_read(pubnub_t *pb, size_t n);

/** Returns if reading is done (has the requested number of octets
    (bytes) been read).
*/
bool pbpal_read_over(pubnub_t *pb);

/** Returns whether for the given Pubnub context the TCP
    connection has closed.
*/
bool pbpal_closed(pubnub_t *pb);

/** Breaks the link between a Pubnub context and the TCP connection.
*/
void pbpal_forget(pubnub_t *pb);

/** Closes (or starts the closing of) the TCP connection of the given
 * Pubnub context 
*/
void pbpal_close(pubnub_t *pb);

/** Returns whether a TCP connection has been established for the
    given Pubnub context.
*/
bool pbpal_connected(pubnub_t *pb);

/** Sets blocking I/O option on the context for the communication */
int pbpal_set_blocking_io(pubnub_t *pb);


#endif /* !defined INC_PBPAL */
