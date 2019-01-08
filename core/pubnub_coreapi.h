/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_COREAPI
#define	INC_PUBNUB_COREAPI


#include "pubnub_api_types.h"

#include <stdbool.h>


/** @file pubnub_coreapi.h 

    This is the "Core" API of the Pubnub client library.

    It has support for the transactions other than publish and
    subscribe (which have a separate module/API), but only for those
    that can be used ony any platform - that is, don't require special
    support.
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
    - "payload": if querying against one channel - the state we got
    (a JSON object), otherwise a JSON object with the key "channels"
    whose value is a JSON object having keys the name of each channels
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

/** Checks if a transaction can be started on @p pb context. In other
    words, checks if previous transaction is finished.

    @retval true can start new transaction
    @retval false otherwise (cannot start a new transaction)
 */
bool pubnub_can_start_transaction(pubnub_t* pb);


#endif /* defined INC_PUBNUB_COREAPI */
