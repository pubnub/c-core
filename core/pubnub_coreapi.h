/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_COREAPI
#define	INC_PUBNUB_COREAPI


#include "pubnub_api_types.h"

#include <stdbool.h>


/** @file pubnub_coreapi.h 
    This is the "Core" API of the Pubnub client library.
    It has the functions that are present in all variants and
    platforms and have the same interface in all of them.
    For the most part, they have the same implementation in
    all of them, too.
*/


/** Append this to a name of the channel to form the name of its
    "presence" channel. A "presence" channel is a pseudo-channel on
    which notifications of presence changes of a channel are announced
    (sent by the Pubnub network). These notifications are JSON objects
    with the following keys:

    - "action": the event that happened, can be "leave", "join", "timeout",
    "state-change"
    - "timestamp": the timestamp of the moment the event happened
    - "uuid": ID of the user that the event pertains to
    - "occupancy": current number of present users in the channel

    There is no special support for these (pseudo) channels in our
    Pubnub client. If you wish to receive presence events, simply
    append this suffix to the name of the channel and subscribe to
    that "combined" name. For example, to receive presence events on
    channel "my_channel", subscribe to "my_channel-pnpres".

    Actually, you can subscribe to both a "regular" channel and a
    "presence" channel at the same time, and you'll receive both the
    presence events (on the "presence channel") and the published
    messages (on the "regular" channel).
 */
#define PUBNUB_PRESENCE_SUFFIX "-pnpres"

/** Initialize a given pubnub context @p p to the @p publish_key and @p
    subscribe_key. You can customize other parameters of the context by
    the configuration function calls below.  

    @note The @p publish_key and @p subscribe key are expected to be
    valid (ASCIIZ string) pointers throughout the use of context @p p,
    that is, until either you call pubnub_done(), or the otherwise
    stop using it (like when the whole software/ firmware stops
    working). So, the contents of these keys are not copied to the
    Pubnub context @p p.

    @pre Call this after TCP initialization.
    @pre @p subscribe_key can't be NULL
    @param p The Context to initialize (use pubnub_alloc() to
    obtain it)
    @param publish_key The string of the key to use when publishing
    messages (if you don't want to publish, you can pass NULL)
    @param subscribe_key The string of the key to use when subscribing
    to messages
    @return Returns the @p p context
*/
pubnub_t* pubnub_init(pubnub_t *p, const char *publish_key, const char *subscribe_key);

/** Set the UUID identification of PubNub client context @p p to @p
    uuid. Pass NULL to unset.

    @note The @p uuid is expected to be valid (ASCIIZ string) pointers
    throughout the use of context @p p, that is, until either you call
    pubnub_done() on @p p, or the otherwise stop using it (like when
    the whole software/ firmware stops working). So, the contents of
    the @p uuid string is not copied to the Pubnub context @p p.  */
void pubnub_set_uuid(pubnub_t *p, const char *uuid);

/** Get the UUID identification of PubNub client context @p p.
    After pubnub_init(), it will return `NULL` until you change it
    to non-`NULL` via pubnub_set_uuid().
    */
char const *pubnub_uuid_get(pubnub_t *p);

/** Set the authentication information of PubNub client context @p
    p. Pass NULL to unset.

    @note The @p auth is expected to be valid (ASCIIZ string) pointers
    throughout the use of context @p p, that is, until either you call
    pubnub_done() on @p p, or the otherwise stop using it (like when
    the whole software/ firmware stops working). So, the contents of
    the auth string is not copied to the Pubnub context @p p.  */
void pubnub_set_auth(pubnub_t *p, const char *auth);

/** Returns the current authentication information for the
    context @p p.
    After pubnub_init(), it will return `NULL` until you change it
    to non-`NULL` via pubnub_set_auth().
*/
char const *pubnub_auth_get(pubnub_t *p);

/** Cancel an ongoing API transaction. The outcome of the transaction
    in progress, if any, will be #PNR_CANCELLED. */
void pubnub_cancel(pubnub_t *p);

/** Publish the @p message (in JSON format) on @p p channel, using the
    @p p context. This actually means "initiate a publish
    transaction". 

    You can't publish if a transaction is in progress in @p p context.

    If transaction is not successful (@c PNR_PUBLISH_FAILED), you can 
    get the string describing the reason for failure by calling
    pubnub_last_publish_result().

    Keep in mind that the time token from the publish operation
    response is _not_ parsed by the library, just relayed to the
    user. Only time-tokens from the subscribe operation are parsed
    by the library.

    Also, for all error codes known at the time of this writing, the
    HTTP error will be set also, so the result of the Pubnub operation
    will not be @c PNR_OK (but you will still be able to get the
    result code and the description).

    @param p The pubnub context. Can't be NULL
    @param channel The string with the channel (or comma-delimited list
    of channels) to publish to.
    @param message The message to publish, expected to be in JSON format

    @return #PNR_STARTED on success, an error otherwise
 */
enum pubnub_res pubnub_publish(pubnub_t *p, const char *channel, const char *message);

/** Publish the @p message (in JSON format) on @p p channel, using the
    @p p context, utilizing the v2 API. This actually means "initiate
    a publish transaction".

    Basically, this is an extension to the pubnub_publish() (v1),
    with some additional options.

    You can't publish if a transaction is in progress in @p p context.

    @param p The pubnub context. Can't be NULL
    @param channel The string with the channel (or comma-delimited list
    of channels) to publish to.
    @param message The message to publish, expected to be in JSON format
    @param store_in_history If `false`, message will not be stored in
    history of the channel
    @param eat_after_reading If `true`, message will not be stored for
    delayed or repeated retrieval or display

    @return #PNR_STARTED on success, an error otherwise
 */
enum pubnub_res pubnub_publishv2(pubnub_t *p, const char *channel, const char *message, bool store_in_history, bool eat_after_reading);

/** Returns a pointer to an arrived message or other element of the
    response to an operation/transaction. Message(s) arrive on finish
    of a subscribe operation or history operation, while for some
    other operations this will give access to the whole response,
    or the next element of the response. That is documented in
    the function that starts the operation.

    Subsequent call to this function will return the next message (if
    any). All messages are from the channel(s) the last operation was
    for.

    @note Context doesn't keep track of the channel(s) you subscribed
    to. This is a memory saving design decision, as most users won't
    change the channel(s) they subscribe too.

    @param p The Pubnub context. Can't be NULL.

    @return Pointer to the message, NULL on error
    @see pubnub_subscribe
 */
char const* pubnub_get(pubnub_t *p);

/** Returns a pointer to an fetched subscribe operation/transaction's
    next channel.  Each transaction may hold a list of channels, and
    this functions provides a way to read them.  Subsequent call to
    this function will return the next channel (if any).

    @note You don't have to read all (or any) of the channels before
    you start a new transaction.

    @param pb The Pubnub context. Can't be NULL.

    @return Pointer to the channel, NULL on error
    @see pubnub_subscribe
    @see pubnub_get
 */
char const *pubnub_get_channel(pubnub_t *pb);

/** Subscribe to @p channel and/or @p channel_group. This actually
    means "initiate a subscribe operation/transaction". The outcome
    will be retrieved by the "notification" API, which is different
    for different platforms. There are two APIs that are widely
    available - "sync" and "callback".

    Messages published on @p channel and/or @p channel_group since the
    last subscribe transaction will be fetched, unless this is the
    first subscribe on this context after initialization or a serious
    error. In that "first" case, this will just retrieve the current
    time token, and that event is called "connect" in many pubnub
    SDKs, but in C-core we don't treat it any different. For that
    "first" case, you will receive a notification that subscribe has
    finished OK, but there will be no messages in the reply.

    The @p channel and @p channel_group strings may contain multiple
    comma-separated channel (channel group) names, so only one call is
    needed to fetch messages from multiple channels (channel groups).

    If @p channel is NULL, then @p channel_group cannot be NULL and
    you will subscribe only to the channel group(s). It goes both
    ways: if @p channel_group is NULL, then @p channel cannot be NULL
    and you will subscribe only to the channel(s).

    You can't subscribe if a transaction is in progress on the context.

    Also, you can't subscribe if there are unread messages in the
    context (you read messages with pubnub_get()).

    @param p The pubnub context. Can't be NULL
    @param channel The string with the channel name (or comma-delimited list
    of channel names) to subscribe to.
    @param channel_group The string with the channel group name (or
    comma-delimited list of channel group names) to subscribe to.

    @return #PNR_STARTED on success, an error otherwise
    
    @see pubnub_get
 */
enum pubnub_res pubnub_subscribe(pubnub_t *p, const char *channel, const char *channel_group);

/** Leave the @p channel. This actually means "initiate a leave
    transaction".  You should leave channel(s) when you want to
    subscribe to another in the same context to avoid loosing
    messages. Also, it is useful for tracking presence.

    You can't leave if a transaction is in progress on the context.

    @param p The Pubnub context. Can't be NULL.  
    @param channel The string with the channel name (or
    comma-delimited list of channel names) to leave from.
    @param channel_group The string with the channel group name (or
    comma-delimited list of channel group names) to leave from.

    @return #PNR_STARTED on success, an error otherwise
*/
enum pubnub_res pubnub_leave(pubnub_t *p, const char *channel, const char *channel_group);

/** Get the current Pubnub time token . This actually means "initiate
    a time transaction". Since time token is in the response to most
    Pubnub REST API calls, this is reserved mostly when you want to
    get a high-quality seed for a random number generator, or some
    such thing.

    If transaction is successful, the gotten time will be the only
    message you can get with pubnub_get(). It will be a (large) JSON
    integer.
    
    You can't get time if a transaction is in progress on the context.

    @param p The Pubnub context. Can't be NULL.  

    @return #PNR_STARTED on success, an error otherwise
*/
enum pubnub_res pubnub_time(pubnub_t *p);

/** Get the message history for the @p channel. This actually
    means "initiate a history transaction/operation".

    If transaction is successful, the gotten messages will be
    available via the pubnub_get().  Using pubnub_get() will give
    you exactly three messages (or, rather, elements).  The first will
    be a JSON array of gotten messages, and the second and third will be
    the timestamps of the first and the last message from that array.

    Also, if you select to @c include_token, then the JSON array
    you get will not be a simple array of gotten messages, but
    rather an array of JSON objects, having keys `message` with
    value the actual message, and `timetoken` with the time token
    of that particular message.
    
    You can't get history if a transaction is in progress on the context.

    @param p The Pubnub context. Can't be NULL. 
    @param channel The string with the channel name to get message 
    history for. This _can't_ be a comma separated list of channels.
    @param count Maximum number of messages to get. If there are less
    than this available on the @c channel, you'll get less, but you
    can't get more.
    @param include_token If true, include the time token for every
    gotten message

    @return #PNR_STARTED on success, an error otherwise
*/
enum pubnub_res pubnub_history(pubnub_t *p, const char *channel, unsigned count, bool include_token);

/** Inform Pubnub that we're still working on @p channel and/or @p
    channel_group.  This actually means "initiate a heartbeat
    transaction". It can be thought of as an update against the
    "presence database".

    If transaction is successful, the response will be a available
    via pubnub_get() as one message, a JSON object. Following keys
    are always present:
    - "status": the HTTP status of the operation (200 OK, 40x error, etc.)
    - "message": the string/message describing the status ("OK"...)
    - "service": should be "Presence"

    If @p channel is NULL, then @p channel_group cannot be NULL and
    you will subscribe only to the channel group(s). It goes both ways:
    if @p channel_group is NULL, then @p channel cannot be NULL and
    you will subscribe only to the channel(s).
    
    You can't get list of currently present users if a transaction is
    in progress on the context.

    @param p The Pubnub context. Can't be NULL. 
    @param channel The string with the channel name (or
    comma-delimited list of channel names) to get presence info for.
    @param channel_group The string with the channel name (or
    comma-delimited list of channel group names) to get presence info for.

    @return #PNR_STARTED on success, an error otherwise
*/
enum pubnub_res pubnub_heartbeat(pubnub_t *p, const char* channel, const char* channel_group);

/** Get the currently present users on a @p channel and/or @p
    channel_group. This actually means "initiate a here_now
    transaction". It can be thought of as a query against the
    "presence database".

    If transaction is successful, the response will be a available
    via pubnub_get() as one message, a JSON object. Following keys
    are always present:
    - "status": the HTTP status of the operation (200 OK, 40x error, etc.)
    - "message": the string/message describing the status ("OK"...)
    - "service": should be "Presence"

    If doing a query on a single channel, following keys are present:
    - "uuids": an array of UUIDs of currently present users
    - "occupancy": the number of currently present users in the channel

    If doing a query on more channels, a key "payload" is present,
    which is a JSON object whose keys are:

    - "channels": a JSON object with keys being the names of the
    channels and their values JSON objects with keys "uuids" and
    "occupancy" with the meaning the same as for query on a single
    channel
    - "total_channels": the number of channels for which the
    presence is given (in "payload")
    - "total_occupancy": total number of users present in all channels

    If @p channel is NULL, then @p channel_group cannot be NULL and
    you will subscribe only to the channel group(s). It goes both ways:
    if @p channel_group is NULL, then @p channel cannot be NULL and
    you will subscribe only to the channel(s).
    
    You can't get list of currently present users if a transaction is
    in progress on the context.

    @param p The Pubnub context. Can't be NULL. 
    @param channel The string with the channel name (or
    comma-delimited list of channel names) to get presence info for.
    @param channel_group The string with the channel name (or
    comma-delimited list of channel group names) to get presence info for.

    @return #PNR_STARTED on success, an error otherwise
*/
enum pubnub_res pubnub_here_now(pubnub_t *p, const char *channel, const char *channel_group);


/** Get the currently present users on all channel. This actually
    means "initiate a global here_now transaction". It can be thought
    of as a query against the "presence database".

    If transaction is successful, the response will be the same
    as for "multi-channel" response for pubnub_here_now(), if
    we queried against all currently available channels.
    
    You can't get list of currently present users if a transaction is
    in progress on the context.

    @param p The Pubnub context. Can't be NULL. 

    @return #PNR_STARTED on success, an error otherwise
*/
enum pubnub_res pubnub_global_here_now(pubnub_t *p);

/** Get the currently present users on a @p channel and/or @p
    channel_group. This actually means "initiate a here_now
    transaction". It can be thought of as a query against the
    "presence database".

    If transaction is successful, the response will be a available
    via pubnub_get() as one message, a JSON object with keys:

    - "status": the HTTP status of the operation (200 OK, 40x error, etc.)
    - "message": the string/message describing the status ("OK"...)
    - "service": should be "Presence"
    - "payload": JSON object with a key "channels" which is an
    array of channels this user is present in

    You can't get channel presence for the user if a transaction is
    in progress on the context.

    @param p The Pubnub context. Can't be NULL. 
    @param uuid The UUID of the user to get the channel presence.
    If NULL, the current UUID of the @c p context will be used.

    @return #PNR_STARTED on success, an error otherwise
*/
enum pubnub_res pubnub_where_now(pubnub_t *p, const char *uuid);

/** Sets some state for the @p channel and/or @channel_group for a
    user, identified by @p uuid. This actually means "initiate a set
    state transaction". It can be thought of as an update against the
    "presence database".

    "State" has to be a JSON object (IOW, several "key-value" pairs).

    If transaction is successful, the response will be a available
    via pubnub_get() as one message, a JSON object with following keys:
    - "status": the HTTP status of the operation (200 OK, 40x error, etc.)
    - "message": the string/message describing the status ("OK"...)
    - "service": should be "Presence"
    - "payload" the state

    This will set the same state to all channels identified by
    @p channel and @p channel_group.

    If @p channel is NULL, then @p channel_group cannot be NULL and
    you will set state only to the channel group(s). It goes both
    ways: if @p channel_group is NULL, then @p channel cannot be NULL
    and you will set state only to the channel(s).

    You can't set state of channels if a transaction is in progress on
    the context.

    @param p The Pubnub context. Can't be NULL. 
    @param channel The string with the channel name (or
    comma-delimited list of channel names) to set state for.
    @param channel_group The string with the channel name (or
    comma-delimited list of channel group names) to set state for.
    @param uuid The UUID of the user for which to set state for.
    If NULL, the current UUID of the @c p context will be used.
    @param state Has to be a JSON object

    @return #PNR_STARTED on success, an error otherwise
*/
enum pubnub_res pubnub_set_state(pubnub_t *p, char const *channel, char const *channel_group, const char *uuid, char const *state);


/** Gets some state for the @p channel and/or @p channel_group for a
    user, identified by @p uuid. This actually means "initiate a get
    state transaction". It can be thought of as a query against the
    "presence database".

    If transaction is successful, the response will be a available
    via pubnub_get() as one message, a JSON object with following keys:
    - "status": the HTTP status of the operation (200 OK, 40x error, etc.)
    - "message": the string/message describing the status ("OK"...)
    - "service": should be "Presence"
    - "payload": if querying against one channel the gotten state 
    (a JSON object), otherwise a JSON object with the key "channels"
    whose value is a JSON object with keys the name of the channels
    and their respective values JSON objects of the gotten state

    If @p channel is NULL, then @p channel_group cannot be NULL and
    you will get state only for the channel group(s). It goes both
    ways: if @p channel_group is NULL, then @p channel cannot be NULL
    and you will get state only for the channel(s).

    You can't get state of channel(s) if a transaction is in progress
    on the context.

    @param p The Pubnub context. Can't be NULL. 
    @param channel The string with the channel name (or
    comma-delimited list of channel names) to get state from.
    @param channel_group The string with the channel name (or
    comma-delimited list of channel group names) to get state from.
    @param uuid The UUID of the user for which to get state for.
    If NULL, the current UUID of the @p p context will be used.

    @return #PNR_STARTED on success, an error otherwise
*/
enum pubnub_res pubnub_state_get(pubnub_t *p, char const *channel, char const *channel_group, const char *uuid);

/** Removes a @p channel_group and all its channels. This actually
    means "initiate a remove_channel_group transaction". It can be
    thought of as an update against the "channel group database".

    If transaction is successful, the response will be a available via
    pubnub_get_channel() as one "channel", a JSON object with keys:

    - "service": should be "channel-registry"
    - "status": the HTTP status of the operation (200 OK, 40x error, etc.)
    - "error": true on error, false on success
    - "message": the string/message describing the status ("OK"...)

    You can't remove a channel group if a transaction is in progress
    on the context.

    @param p The Pubnub context. Can't be NULL. 
    @param channel_group The channel group to remove

    @return #PNR_STARTED on success, an error otherwise
*/
enum pubnub_res pubnub_remove_channel_group(pubnub_t *p, char const *channel_group);

/** Removes a @p channel from the @p channel_group . This actually
    means "initiate a remove_channel_from_channel_group
    transaction". It can be thought of as an update against the
    "channel group database".

    You can't remove the last channel from a channel group. To do
    that, remove the channel group itself.

    If transaction is successful, the response will be a available via
    pubnub_get_channel() as one "channel", a JSON object with keys:

    - "service": should be "channel-registry"
    - "status": the HTTP status of the operation (200 OK, 40x error, etc.)
    - "error": true on error, false on success
    - "message": the string/message describing the status ("OK"...)

    You can't remove a channel from a channel group if a transaction
    is in progress on the context.

    @param p The Pubnub context. Can't be NULL. 
    @param channel_group The channel to remove
    @param channel_group The channel group to remove from

    @return #PNR_STARTED on success, an error otherwise
*/
enum pubnub_res pubnub_remove_channel_from_group(pubnub_t *p, char const *channel, char const *channel_group);

/** Adds a @p channel to the @p channel_group . This actually means
    "initiate a add_channel_to_channel_group transaction". It can be
    thought of as an update against the "channel group database".

    If the channel group doesn't exist, this implicitly adds (creates)
    it.

    If transaction is successful, the response will be a available
    via pubnub_get_channel() as one "channel", a JSON object with keys:

    - "service": should be "channel-registry"
    - "status": the HTTP status of the operation (200 OK, 40x error, etc.)
    - "error": true on error, false on success
    - "message": the string/message describing the status ("OK"...)

    You can't add a channel to a channel group if a transaction
    is in progress on the context.

    @param p The Pubnub context. Can't be NULL. 
    @param channel The channel to add
    @param channel_group The channel group to add to

    @return #PNR_STARTED on success, an error otherwise
*/
enum pubnub_res pubnub_add_channel_to_group(pubnub_t *p, char const *channel, char const *channel_group);

/** Lists all channels of a @p channel_group. This actually
    means "initiate a list_channel_group transaction". It can be
    thought of as a query against the "channel group database".

    If transaction is successful, the response will be a available via
    pubnub_get_channel() as one "channel", a JSON object with keys:

    - "service": should be "channel-registry"
    - "status": the HTTP status of the operation (200 OK, 40x error, etc.)
    - "error": true on error, false on success
    - "payload": JSON object with keys "group" with value the string
    of the channel group name and "channels" with value a JSON array
    of strings with names of the channels that belong to the group

    You can't remove a channel group if a transaction is in progress
    on the context.

    @param p The Pubnub context. Can't be NULL. 
    @param channel_group The channel group to list

    @return #PNR_STARTED on success, an error otherwise
*/
enum pubnub_res pubnub_list_channel_group(pubnub_t *p, char const *channel_group);

/** Returns the result of the last transaction in the @p p context.
    This _may_ block if using blocking I/O. It will _not_ block if using
    non-blocking I/O.  

    @see pubnub_set_blocking_io
    @see pubnub_set_non_blocking_io
*/
enum pubnub_res pubnub_last_result(pubnub_t *p);

/** Returns the HTTP reply code of the last transaction in the @p p
 * context. */
int pubnub_last_http_code(pubnub_t *p);

/** Returns the string of the result of the last `publish` transaction,
    as returned from Pubnub. If the last transaction is not a publish,
    or there is some other error, it returns NULL. If the Publish
    was successfull, it will return "Sent", otherwise a description
    of the error.
 */
char const *pubnub_last_publish_result(pubnub_t *p);

/** Returns the string of the last received time token on the
    @c p context. After pubnub_init() this should be "0".
    @param p Pubnub context to get the last received time token from
    @return A read only string of the last received time token
 */
char const *pubnub_last_time_token(pubnub_t *p);

/** Gets the origin to be used for the context @p p.
    If setting of the origin is not enabled, this will return
    the default origin.
    @param p Pubnub context to get the origin from
    @return A read only string of origin used for context @p p
 */
char const *pubnub_get_origin(pubnub_t *p);

/** Sets the origin to be used for the context @p p.  If setting of
    the origin is not enabled, this will fail.  It may also fail if it
    detects an invalid origin, but NULL is not an invalid origin - it
    resets the origin to default.

    @param p Pubnub context to set the origin for
    @param origin The origin to use for context @p p. If NULL,
    the default origin will be set
    @return 0: success, -1: fail
*/
int pubnub_origin_set(pubnub_t *p, char const *origin);


#endif /* defined INC_PUBNUB_COREAPI */
