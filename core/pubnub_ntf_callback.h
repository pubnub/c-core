/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_NTF_CALLBACK
#define      INC_PUBNUB_NTF_CALLBACK


#include "pubnub_api_types.h"


/** @file pubnub_ntf_callback.h 
    This is the "callback" notification interface. It is the part of
    the user API of the Pubnub C client.

    Pubnub client library offers two "mostly portable" interfaces for
    getting notification about the outcome of a Pubnub transaction.
    Those are:
    - sync (this one)
    - callback

    There are others, which are specific to a platform (like process
    events for Contiki OS). Also, not all platforms support the
    two "mostly portable" interfaces mentioned above.

    The "callback" interface provides a means to register a callback
    function to be called with the outcome of a Pubnub transaction.
    There is only one callback function per a Pubnub context.

    The user should not assume anything about the thread on which the
    callback is called. It is up to the user to synchronize access to
    the Pubnub context for which the callback was called with any
    (other) threads that might be trying to use it at the same time.
*/


/** Pointer to a function to be called on the outcome of a Pubnub
    transaction.
    @param pb The Pubnub context for which the outcome is reported
    @param trans Type of transaction (publish, subscribe...) at hand
    @param result The actual result of the transaction
    @param user_data The pointer provided by the user to pubnub_register_callback()
 */
typedef void (*pubnub_callback_t)(pubnub_t *pb, enum pubnub_trans trans, enum pubnub_res result, void *user_data);

/** Registers a callback function to be called when a transaction
    ends.  While it is OK to register a NULL pointer, which means no
    callback will be called, it is useful only in very specific
    circumstances. Also, `NULL` is the initial value (after calling
    pubnub_init()), so no need to set it.

    Don't make any assumptions about the thread on which this
    function is called.

    @param pb The Pubnub context for which the callback is set
    @param cb Pointer to function to call on end of transaction
    @param user_data Pointer that will be given to the callback function

    @return PNR_OK on success, a value indicating the error otherwise
*/
enum pubnub_res pubnub_register_callback(pubnub_t *pb, pubnub_callback_t cb, void *user_data);

/** Returns the user data set with pubnub_register_callback().
 */
void *pubnub_get_user_data(pubnub_t *pb);


#endif /* !defined INC_PUBNUB_NTF_CALLBACK */

