/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_CCORE
#define      INC_PUBNUB_CCORE

#include "pubnub_config.h"
#include "pubnub_api_types.h"

#include <stdbool.h>


/** @file pubnub_ccore.h 

    The "C core" module is the internal module of the Pubnub C
    clients, shared by all. It has all the "generic" code (formatting,
    parsing, HTTP...)  while individual C clients deal with
    interacting with the environment (OS, frameworks, libraries...).

    The interface of this module is subject to change, so user of a
    Pubnub C client should *not* use this module directly. Use the
    official and documented API for your environment.
*/


/** The Pubnub "(C) core" context, contains context data 
    that is shared among all Pubnub C clients.
 */
struct pbcc_context {
    /** The publish key (to use when publishing) */
    char const *publish_key;
    /** The subscribe key (to use when subscribing) */
    char const *subscribe_key;
    /** The UUID to be sent on to server. If NULL, don't send any */
    char const *uuid;
    /** The `auth` parameter to be sent on to server. If NULL, don't send any */
    char const *auth;
    /** The last used time token. */
    char timetoken[64];

    /** The result of the last Pubnub transaction */
    enum pubnub_res last_result;

    /** The buffer for HTTP data */
    char http_buf[PUBNUB_BUF_MAXLEN];
    /** Last received HTTP (result) code */
    int http_code;
    /** The length of the data in the HTTP buffer */
    unsigned http_buf_len;
    /** The length of total data to be received in a HTTP reply */
    unsigned http_content_len;
    /** Indicates whether we are receiving chunked or regular HTTP response */
    bool http_chunked;
    /** The contents of a HTTP reply/reponse */
    char http_reply[PUBNUB_REPLY_MAXLEN+1];

    /* These in-string offsets are used for yielding messages received
     * by subscribe - the beginning of last yielded message and total
     * length of message buffer, and the same for channels.
     */
    unsigned short msg_ofs, msg_end, chan_ofs, chan_end;

};


/** Initializes the Pubnub C core context */
void pbcc_init(struct pbcc_context *pbcc, const char *publish_key, const char *subscribe_key);

/** Returns the next message from the Pubnub C Core context. NULL if
    there are no (more) messages
*/
char const *pbcc_get_msg(struct pbcc_context *pb);

/** Returns the next channel from the Pubnub C Core context. NULL if
    there are no (more) messages.
*/
char const *pbcc_get_channel(struct pbcc_context *pb);

/** Sets the UUID for the context */
void pbcc_set_uuid(struct pbcc_context *pb, const char *uuid);

/** Sets the `auth` for the context */
void pbcc_set_auth(struct pbcc_context *pb, const char *auth);

/** Parses the string received as a response for a subscribe operation
    (transaction). This checks if the response is valid, and, if it
    is, prepares for giving the messages (and possibly channels) that
    are received in the response to the user (via pbcc_get_msg() and
    pbcc_get_channel()).

    @param p The Pubnub C core context to parse the response "in"
    @return 0: OK, -1: error (invalid response)
*/
int pbcc_parse_subscribe_response(struct pbcc_context *p);

/** Parses the string received as a response for a publish operation
    (transaction). This checks if the response is valid, and, if it
    is, enables getting it as the gotten message (like for
    `subscribe`).

    @param p The Pubnub C core context to parse the response "in"
    @return The result of the parsing, expressed as the "Pubnub
    result" enum
*/
enum pubnub_res pbcc_parse_publish_response(struct pbcc_context *p);

/** Parses the string received as a response for a time operation
    (transaction). This checks if the response is valid, and, if it
    is, enables getting it as the gotten message (like for
    `subscribe`).

    @param p The Pubnub C core context to parse the response "in"
    @return 0: OK, -1: error (invalid response)
*/
int pbcc_parse_time_response(struct pbcc_context *p);

/** Parses the string received as a response for a history operation
    (transaction). This checks if the response is valid, and, if it
    is, enables getting the gotten message (like for `subscribe`).

    @param p The Pubnub C core context to parse the response "in"
    @return 0: OK, -1: error (invalid response)
*/
int pbcc_parse_history_response(struct pbcc_context *p);

/** Parses the string received as a response for a history v2
    operation (transaction). This checks if the response is valid,
    and, if it is, enables getting the gotten message, as a JSON
    array, and the timestamps for the first and last of them.

    @param p The Pubnub C core context to parse the response "in"
    @return 0: OK, -1: error (invalid response)
*/
int pbcc_parse_historyv2_response(struct pbcc_context *p);

/** Parses the string received as a response for a presence query
    operation (transaction). Presence query is done on several
    user requests: "where-now", "here-now", etc. 

    This checks if the response is valid (a JSON object), and, if it
    is, enables getting it, as a whole, in one pubnub_get().

    @param p The Pubnub C core context to parse the response "in"
    @return 0: OK, -1: error (invalid response)
*/
int pbcc_parse_presence_response(struct pbcc_context *p);

/** Parses the string received as a response for a channel-registry
    operation (transaction). It is done on several user requests
    (add/remove channel (from/to) channel group, list (channels
    in a) channel group, remove channel group).

    This checks if the response is valid (a JSON object), and, if it
    is, enables getting it, as a whole, in one pubnub_get().

    @param p The Pubnub C core context to parse the response "in"
    @return The result of the parsing, expressed as the "Pubnub
    result" enum
*/
enum pubnub_res pbcc_parse_channel_registry_response(struct pbcc_context *p);

/** Prepares the Publish operation (transaction), mostly by
    formatting the URI of the HTTP request.
 */
enum pubnub_res pbcc_publish_prep(struct pbcc_context *pb, const char *channel, const char *message, bool store_in_history, bool eat_after_reading);

/** Prepares the Subscribe operation (transaction), mostly by
    formatting the URI of the HTTP request.
 */
enum pubnub_res pbcc_subscribe_prep(struct pbcc_context *p, const char *channel, const char *channel_group);

/** Prepares the Leave operation (transaction), mostly by
    formatting the URI of the HTTP request.
 */
enum pubnub_res pbcc_leave_prep(struct pbcc_context *p, const char *channel, const char *channel_group);

/** Prepares the Time operation (transaction), mostly by
    formatting the URI of the HTTP request.
 */
enum pubnub_res pbcc_time_prep(struct pbcc_context *p);

/** Prepares the History operation (transaction), mostly by
    formatting the URI of the HTTP request.
 */
enum pubnub_res pbcc_history_prep(struct pbcc_context *p, const char *channel, const char *channel_group, unsigned count);

/** Prepares the History v2 operation (transaction), mostly by
    formatting the URI of the HTTP request.
 */
enum pubnub_res pbcc_historyv2_prep(struct pbcc_context *p, const char *channel, const char *channel_group, unsigned count, bool include_token);

/** Prepares the Here-now operation (transaction), mostly by
    formatting the URI of the HTTP request.
 */
enum pubnub_res pbcc_here_now_prep(struct pbcc_context *p, const char *channel, const char *channel_group);

/** Prepares the Where-now operation (transaction), mostly by
    formatting the URI of the HTTP request.
 */
enum pubnub_res pbcc_where_now_prep(struct pbcc_context *p, const char *uuid);

/** Prepares the Set state operation (transaction), mostly by
    formatting the URI of the HTTP request.
 */
enum pubnub_res pbcc_set_state_prep(struct pbcc_context *p, char const *channel, char const *channel_group, const char *uuid, char const *state);

/** Prepares the Get state operation (transaction), mostly by
    formatting the URI of the HTTP request.
*/
enum pubnub_res pbcc_state_get_prep(struct pbcc_context *p, char const *channel, char const *channel_group, const char *uuid);

/** Preparse the Remove channel group operation (transaction) , mostly by
    formatting the URI of the HTTP request.
*/
enum pubnub_res pbcc_remove_channel_group_prep(struct pbcc_context *p, char const *channel_group);

/** Preparse an operation (transaction) against the channel registry,
    mostly by formatting the URI of the HTTP request.
*/
enum pubnub_res pbcc_channel_registry_prep(struct pbcc_context *p, char const *channel_group, char const *param, char const *channel);

#endif /* !defined INC_PUBNUB_CCORE */
