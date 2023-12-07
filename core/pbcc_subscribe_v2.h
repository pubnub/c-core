/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#if PUBNUB_USE_SUBSCRIBE_V2

#if !defined INC_PBCC_SUBSCRIBE_V2
#define INC_PBCC_SUBSCRIBE_V2

#include "pubnub_subscribe_v2_message.h"

struct pbcc_context;

/** Prepares the Subscribe_v2 operation (transaction), mostly by
    formatting the URI of the HTTP request.
  */
enum pubnub_res pbcc_subscribe_v2_prep(struct pbcc_context* p,
                                       char const*          channel,
                                       char const*          channel_group,
                                       unsigned*            heartbeat,
                                       char const*          filter_expr);


/** Parses the string received as a response for a subscribe_v2 operation
    (transaction). This checks if the response is valid, and, if it
    is, prepares for giving the v2 messages that are received in the
    response to the user (via pbcc_get_msg_v2()).

    @param p The Pubnub C core context to parse the response "in"
    @return PNR_OK: OK, PNR_FORMAT_ERROR: error (invalid response)
  */
enum pubnub_res pbcc_parse_subscribe_v2_response(struct pbcc_context* p);


/** Returns the next v2 message from the Pubnub C Core context.
    Empty structure if there are no (more) v2 messages(checked through presence
    of valid timetoken, or payload) 
  */
struct pubnub_v2_message pbcc_get_msg_v2(struct pbcc_context* p);


#endif /* !defined INC_PBCC_SUBSCRIBE_V2 */

#endif /* PUBNUB_USE_SUBSCRIBE_V2 */

