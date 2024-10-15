/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_PUBSUBAPI
#define INC_PUBNUB_PUBSUBAPI


#include "pubnub_api_types.h"
#include "lib/pb_deprecated.h"
#include "lib/pb_extern.h"

#include <stdbool.h>
#include <stdint.h>

/** @file pubnub_pubsubapi.h
    This is the "Pub/Sub" API of the Pubnub client library.

    It is the minimal, publish & subscribe only API.
*/


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
PUBNUB_EXTERN pubnub_t* pubnub_init(pubnub_t* p, const char* publish_key, const char* subscribe_key);

/** Set the user identification of PubNub client context @p p to @p
    uuid. Pass NULL to unset.

    @deprecated this is provided as a workaround for existing users.
    Please use `pubnub_set_user_id` instead.

    @see pubnub_set_user_id
*/
PUBNUB_EXTERN PUBNUB_DEPRECATED enum pubnub_res pubnub_set_uuid(pubnub_t* p, const char* uuid);

/** Get the user identification of PubNub client context @p p.
    After pubnub_init(), it will return `NULL` until you change it
    to non-`NULL` via pubnub_set_uuid().
    
    @deprecated this is provided as a workaround for existing users.
    Please use `pubnub_user_id_get` instead.

    @see pubnub_user_id_get
*/
PUBNUB_EXTERN PUBNUB_DEPRECATED char const* pubnub_uuid_get(pubnub_t* p);

/** Set the user identification of PubNub client context @p p to @p
    uuid. Pass NULL to unset.

    @note The @p user_id is expected to be valid (ASCIIZ string) pointers
    throughout the use of context @p p, that is, until either you call
    pubnub_done() on @p p, or the otherwise stop using it (like when
    the whole software/ firmware stops working). So, the contents of
    the @p user_id string is not copied to the Pubnub context @p p.  
*/
PUBNUB_EXTERN enum pubnub_res pubnub_set_user_id(pubnub_t* p, const char* user_id);

/** Get the user identification of PubNub client context @p p.
    After pubnub_init(), it will return `NULL` until you change it
    to non-`NULL` via pubnub_set_user_id().
*/
PUBNUB_EXTERN char const* pubnub_user_id_get(pubnub_t* p);

/** Set the authentication information of PubNub client context @p
    p. Pass NULL to unset.

    @note The @p auth is expected to be valid (ASCIIZ string) pointers
    throughout the use of context @p p, that is, until either you call
    pubnub_done() on @p p, or the otherwise stop using it (like when
    the whole software/ firmware stops working). So, the contents of
    the auth string is not copied to the Pubnub context @p p.  */
PUBNUB_EXTERN void pubnub_set_auth(pubnub_t* p, const char* auth);

/** Returns the current authentication information for the
    context @p p.
    After pubnub_init(), it will return `NULL` until you change it
    to non-`NULL` via pubnub_set_auth().
*/
PUBNUB_EXTERN char const* pubnub_auth_get(pubnub_t* p);

/** Cancels an ongoing API transaction. This will, once it is done,
    close the (TCP/IP) connection to Pubnub (if it was open).  The
    outcome of the transaction in progress, if any, will be
    #PNR_CANCELLED.

    In the sync interface, it's possible that this cancellation will
    finish during the execution of a call to this function. But,
    there's no guarantee, so check the result.

    In the callback interface, it's not likely cancellation will be
    done, but, still, it's possible. So, if this matters to you, it's
    always best to check the result.

    @retval #PN_CANCEL_STARTED cancel started, await the outcome
    @retval #PN_CANCEL_FINISHED cancelled, no need to await
*/

/** Set the auth token information of PubNub client context @p
    p. Pass NULL to unset.

    @note The @p token is expected to be valid (ASCIIZ string) pointers
    throughout the use of context @p pb, that is, until either you call
    pubnub_done() on @p pb, or the otherwise stop using it (like when
    the whole software/ firmware stops working). So, the contents of
    the auth string is not copied to the Pubnub context @p pb.  */
PUBNUB_EXTERN void pubnub_set_auth_token(pubnub_t* pb, const char* token);

/** Returns the current auth token information for the
    context @p pb.
    After pubnub_init(), it will return `NULL` until you change it
    to non-`NULL` via pubnub_set_auth_token().
*/
PUBNUB_EXTERN char const* pubnub_auth_token_get(pubnub_t* pb);

PUBNUB_EXTERN enum pubnub_cancel_res pubnub_cancel(pubnub_t* p);

/** Publish the @p message (in JSON format) on @p p channel, using the
    @p p context. This actually means "initiate a publish
    transaction".

    You can't publish if a transaction is in progress on @p p context.

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
    @param channel The string with the channel to publish to.
    @param message The message to publish, expected to be in JSON format

    @return #PNR_STARTED or #PNR_OK on success, an error otherwise
 */
PUBNUB_EXTERN enum pubnub_res pubnub_publish(pubnub_t* p, const char* channel, const char* message);

/** Sends a signal @p message (in JSON format) on @p channel, using @p pb context.
    This actually means "initiate a signal transaction".

    It has similar behaviour as publish, but unlike publish
    transaction, signal erases previous signal message on server(, on
    a given channel,) and you can not send any metadata.

    There can be only up to one signal message at the time. If it's
    not renewed by another signal, signal message disappears from
    channel history after certain amount of time.

    You can't (send a) 'signal' if a transaction is in progress on @p
    p context.

    If transaction is not successful (@c PNR_PUBLISH_FAILED), you can
    get the string describing the reason for failure by calling
    pubnub_last_publish_result().

    Keep in mind that the time token from the signal operation
    response is _not_ parsed by the library, just relayed to the
    user. Only time-tokens from the subscribe operation are parsed
    by the library.

    Also, for all error codes known at the time of this writing, the
    HTTP error will be set also, so the result of the Pubnub operation
    will not be @c PNR_OK (but you will still be able to get the
    result code and the description).

    @param pb The pubnub context. Can't be NULL
    @param channel The string with the channel to signal to.
    @param message The signal message to send, expected to be in JSON format

    @return #PNR_STARTED or #PNR_OK on success, an error otherwise
 */
PUBNUB_EXTERN enum pubnub_res pubnub_signal(pubnub_t* pb,
                              const char* channel,
                              const char* message);

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
PUBNUB_EXTERN char const* pubnub_get(pubnub_t* p);

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
PUBNUB_EXTERN char const* pubnub_get_channel(pubnub_t* pb);

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

    When auto heartbeat is enabled at compile time both @p channel
    and @p channel_group could be passed as NULL which suggests default
    behaviour(unless it is uuid's very first subscription) in which case
    transaction uses channel and channel groups that are already subscribed.

    You can't subscribe if a transaction is in progress on the context.

    Also, you can't subscribe if there are unread messages in the
    context (you read messages with pubnub_get()).

    @param p The pubnub context. Can't be NULL
    @param channel The string with the channel name (or comma-delimited list
    of channel names) to subscribe to.
    @param channel_group The string with the channel group name (or
    comma-delimited list of channel group names) to subscribe to.

    @return #PNR_STARTED or #PNR_OK on success, an error otherwise

    @see pubnub_get
 */
PUBNUB_EXTERN enum pubnub_res pubnub_subscribe(pubnub_t*   p,
                                 const char* channel,
                                 const char* channel_group);

/** Returns the result of the last transaction in the @p p context.
    This _may_ block if using blocking I/O. It will _not_ block if using
    non-blocking I/O.

    @see pubnub_set_blocking_io
    @see pubnub_set_non_blocking_io
*/
PUBNUB_EXTERN enum pubnub_res pubnub_last_result(pubnub_t* p);

/** Returns the HTTP reply code of the last transaction in the @p p
 * context. */
PUBNUB_EXTERN int pubnub_last_http_code(pubnub_t* p);

#if PUBNUB_USE_RETRY_CONFIGURATION
/** Returns the HTTP reply `Retry-After` header value of the last transaction in
 * the @p p context. */
PUBNUB_EXTERN uint16_t pubnub_last_http_retry_header(pubnub_t* pb);
#endif // #if PUBNUB_USE_RETRY_CONFIGURATION

/** Returns the string of the result of the last `publish` transaction,
    as returned from Pubnub. If the last transaction is not a publish,
    or there is some other error, it returns NULL. If the Publish
    was successfull, it will return "Sent", otherwise a description
    of the error.
 */
PUBNUB_EXTERN char const* pubnub_last_publish_result(pubnub_t* p);

/** Returns the string of the last received time token on the
    @c p context. After pubnub_init() this should be "0".
    @param p Pubnub context to get the last received time token from
    @return A read only string of the last received time token
 */
PUBNUB_EXTERN char const* pubnub_last_time_token(pubnub_t* p);

/** Gets the origin to be used for the context @p p.
    If setting of the origin is not enabled, this will return
    the default origin.
    @param p Pubnub context to get the origin from
    @return A read only string of origin used for context @p p
 */
PUBNUB_EXTERN char const* pubnub_get_origin(pubnub_t* p);

/** Sets the origin to be used for the context @p p.  If setting of
    the origin is not enabled, this will fail.  It may also fail if it
    detects an invalid origin, but NULL is not an invalid origin - it
    resets the origin to default.

    @param p Pubnub context to set the origin for
    @param origin The origin to use for context @p p. If NULL,
                  the default origin will be set
    @retval 0 origin set,
    @retval +1 origin set will be applied with new connection,
    @retval -1 setting origin not enabled
*/
PUBNUB_EXTERN int pubnub_origin_set(pubnub_t* p, char const* origin);

/** Sets the port to be used for the context @p p.  If setting of
    the port is not enabled, this will fail.  It may also fail if it
    detects an invalid port.

    @param p Pubnub context to set the port for
    @param origin The port to use for context @p p. 
    @retval 0 port set,
    @retval +1 port set will be applied with new connection,
    @retval -1 setting port not enabled
*/
PUBNUB_EXTERN int pubnub_port_set(pubnub_t* p, uint16_t port);

/** Enables the use of HTTP Keep-Alive ("persistent connections")
    on the context @p p.

    This is the default, but, you can turn it off with
    pubnub_dont_use_http_keep_alive().

    If HTTP Keep-Alive is active, connection to Pubnub will not be
    closed after the transaction ends. This will avoid connecting
    again on next transaction on the same context, making the
    transaction finish (usually much) quicker.

    But, there's a trade-off here, here are the drawbacks:

    * pubnub_free() is more likely to not complete for contexts that
      are in "keep alive" state.
    * Socket in the keep-alive state will be closed by the
      Pubnub network (server) after some period of inactivity.
      While we should be able to handle that, it's possible
      that some race condition causes a transaction to fail
      in this case.
    * Socket in the keep-alive state is "allocated", consuming
      some resources. If you have a constrained number of
      sockets, relative to Pubnub contexts, this may be an
      issue.
 */
PUBNUB_EXTERN void pubnub_use_http_keep_alive(pubnub_t* p);

/** Disables the use of HTTP Keep-Alive ("persistent connections")
    on the context @p p.

    The default is to _use_ the HTTP Keep-Alive, but, you might
    want to turn that off - see pubnub_use_http_keep_alive().
*/
PUBNUB_EXTERN void pubnub_dont_use_http_keep_alive(pubnub_t* p);


#endif /* !defined INC_PUBNUB_PUBSUBAPI */
