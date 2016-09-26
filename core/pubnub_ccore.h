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

/** The 3-state bool. For Electrical Enginners among you, this would
    be a digital line utilizing high impedance ("High Z"). For
    mathematicians, it would be a "Maybe monad". For the rest of us,
    it has `true`, `false` and the third `not set` or `indeterminate`
    state.
*/
enum pbcc_tribool {
    pbccFalse,
    pbccTrue,
    pbccNotSet
};


/** The Pubnub "(C) core" context, contains context data 
    that is shared among all Pubnub C clients.
 */
struct pbcc_context {
    /** The publish key (to use when publishing) */
    char const *publish_key;
    /** The subscribe key (to use when subscribing) */
    char const *subscribe_key;
    /** The UUID to be sent to server. If NULL, don't send any */
    char const *uuid;
    /** The `auth` parameter to be sent to server. If NULL, don't send
     * any */
    char const *auth;

    /** The last used time token. */
    char timetoken[20];

    /** The result of the last Pubnub transaction */
    enum pubnub_res last_result;

    /** The "scratch" buffer for HTTP data */
    char http_buf[PUBNUB_BUF_MAXLEN];

    /** The length of the data currently in the HTTP buffer ("scratch"
        or reply, depending on the state).
     */
    unsigned http_buf_len;

    /** The total length of data to be received in a HTTP reply or
        chunk of it.
     */
    unsigned http_content_len;

#if PUBNUB_DYNAMIC_REPLY_BUFFER
    char *http_reply;
#else
    /** The contents of a HTTP reply/reponse */
    char http_reply[PUBNUB_REPLY_MAXLEN+1];
#endif

    /* These in-string offsets are used for yielding messages received
     * by subscribe - the beginning of last yielded message and total
     * length of message buffer.
     */
    unsigned msg_ofs, msg_end;

    /* Like the offsets for the messages, these are the offsets for
       the channels. Unlikey the message(s), the channels don't have
       to be received at all.
    */
    unsigned chan_ofs, chan_end;

#if PUBNUB_CRYPTO_API
    /** Secret key to use for encryption/decryption */
    char const *secret_key;
#endif
};


/** Initializes the Pubnub C core context */
void pbcc_init(struct pbcc_context *pbcc, const char *publish_key, const char *subscribe_key);

/** Deinitializes the Pubnub C core context */
void pbcc_deinit(struct pbcc_context *p);

/** Reallocates the reply buffer in the C core context @p p to have
    @p bytes.
    @return 0: OK, allocated, -1: failed
*/
int pbcc_realloc_reply_buffer(struct pbcc_context *p, unsigned bytes);

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

/** Parses the string received as a response for a history v2
    operation (transaction). This checks if the response is valid,
    and, if it is, enables getting the gotten message, as a JSON
    array, and the timestamps for the first and last of them.

    @param p The Pubnub C core context to parse the response "in"
    @return 0: OK, -1: error (invalid response)
*/
int pbcc_parse_history_response(struct pbcc_context *p);

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
enum pubnub_res pbcc_subscribe_prep(struct pbcc_context *p, const char *channel, const char *channel_group, unsigned *heartbeat);

/** Prepares the Leave operation (transaction), mostly by
    formatting the URI of the HTTP request.
 */
enum pubnub_res pbcc_leave_prep(struct pbcc_context *p, const char *channel, const char *channel_group);

/** Prepares the Time operation (transaction), mostly by
    formatting the URI of the HTTP request.
 */
enum pubnub_res pbcc_time_prep(struct pbcc_context *p);

/** Prepares the History v2 operation (transaction), mostly by
    formatting the URI of the HTTP request.
 */
enum pubnub_res pbcc_history_prep(struct pbcc_context *p, const char *channel, unsigned count, bool include_token);

/** Prepares the Heartbeat operation (transaction), mostly by
    formatting the URI of the HTTP request.
 */
enum pubnub_res pbcc_heartbeat_prep(struct pbcc_context *p, const char *channel, const char *channel_group);

/** Prepares the Here-now operation (transaction), mostly by
    formatting the URI of the HTTP request.
 */
enum pubnub_res pbcc_here_now_prep(struct pbcc_context *p, const char *channel, const char *channel_group, enum pbcc_tribool disable_uuids, enum pbcc_tribool state);

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
