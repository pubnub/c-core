/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_CCORE
#define INC_PUBNUB_CCORE

#include "pubnub_api_types.h"
#include "lib/pb_extern.h"

#include <stdbool.h>

typedef struct pubnub_char_mem_block pubnub_chamebl_t;


/** @file pubnub_ccore.h

    This has the functions for formating and parsing the
    requests and responses for transactions other than
    the publish and subscribe - which have a separate module;
*/


struct pbcc_context;


/** Parses the string received as a response for a time operation
    (transaction). This checks if the response is valid, and, if it
    is, enables getting it as the gotten message (like for
    `subscribe`).

    @param p The Pubnub C core context to parse the response "in"
    @return 0: OK, -1: error (invalid response)
*/
PUBNUB_EXTERN enum pubnub_res pbcc_parse_time_response(struct pbcc_context* p);

/** Parses the string received as a response for a history v2
    operation (transaction). This checks if the response is valid,
    and, if it is, enables getting the gotten message, as a JSON
    array, and the timestamps for the first and last of them.

    @param p The Pubnub C core context to parse the response "in"
    @return 0: OK, -1: error (invalid response)
*/
PUBNUB_EXTERN enum pubnub_res pbcc_parse_history_response(struct pbcc_context* p);

/** Parses the string received as a response for a presence query
    operation (transaction). Presence query is done on several
    user requests: "where-now", "here-now", etc.

    This checks if the response is valid (a JSON object), and, if it
    is, enables getting it, as a whole, in one pubnub_get().

    @param p The Pubnub C core context to parse the response "in"
    @return 0: OK, -1: error (invalid response)
*/
PUBNUB_EXTERN enum pubnub_res pbcc_parse_presence_response(struct pbcc_context* p);

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
PUBNUB_EXTERN enum pubnub_res pbcc_parse_channel_registry_response(struct pbcc_context* p);

/** Prepares the Leave operation (transaction), mostly by
    formatting the URI of the HTTP request.
 */
PUBNUB_EXTERN enum pubnub_res pbcc_leave_prep(struct pbcc_context* p,
                                const char*          channel,
                                const char*          channel_group);

/** Prepares the Time operation (transaction), mostly by
    formatting the URI of the HTTP request.
 */
PUBNUB_EXTERN enum pubnub_res pbcc_time_prep(struct pbcc_context* p);

/** Prepares the History v2 operation (transaction), mostly by
    formatting the URI of the HTTP request.
 */
PUBNUB_EXTERN enum pubnub_res pbcc_history_prep(struct pbcc_context* p,
                                  const char*          channel,
                                  unsigned             count,
                                  bool                 include_token,
                                  enum pubnub_tribool  string_token,
                                  enum pubnub_tribool  reverse,
                                  enum pubnub_tribool  include_meta,
                                  char const*          start,
                                  char const*          end);

/** Prepares the Heartbeat operation (transaction), mostly by
    formatting the URI of the HTTP request.
 */
PUBNUB_EXTERN enum pubnub_res pbcc_heartbeat_prep(struct pbcc_context* p,
                                    const char*          channel,
                                    const char*          channel_group);

/** Prepares the Here-now operation (transaction), mostly by
    formatting the URI of the HTTP request.
 */
PUBNUB_EXTERN enum pubnub_res pbcc_here_now_prep(struct pbcc_context* p,
                                   const char*          channel,
                                   const char*          channel_group,
                                   enum pubnub_tribool  disable_uuids,
                                   enum pubnub_tribool  state);

/** Prepares the Where-now operation (transaction), mostly by
    formatting the URI of the HTTP request.
 */
PUBNUB_EXTERN enum pubnub_res pbcc_where_now_prep(struct pbcc_context* p, const char* uuid);

/** Prepares the Set state operation (transaction), mostly by
    formatting the URI of the HTTP request.
 */
PUBNUB_EXTERN enum pubnub_res pbcc_set_state_prep(struct pbcc_context* p,
                                    char const*          channel,
                                    char const*          channel_group,
                                    const char*          uuid,
                                    char const*          state);

/** Prepares the Get state operation (transaction), mostly by
    formatting the URI of the HTTP request.
*/
PUBNUB_EXTERN enum pubnub_res pbcc_state_get_prep(struct pbcc_context* p,
                                    char const*          channel,
                                    char const*          channel_group,
                                    const char*          uuid);

/** Preparse the Remove channel group operation (transaction) , mostly by
    formatting the URI of the HTTP request.
*/
PUBNUB_EXTERN enum pubnub_res pbcc_remove_channel_group_prep(struct pbcc_context* p,
                                               char const* channel_group);

/** Preparse an operation (transaction) against the channel registry,
    mostly by formatting the URI of the HTTP request.
*/
PUBNUB_EXTERN enum pubnub_res pbcc_channel_registry_prep(struct pbcc_context* p,
                                           char const*          channel_group,
                                           char const*          param,
                                           char const*          channel);

#endif /* !defined INC_PUBNUB_CCORE */
