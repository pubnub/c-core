/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_CREATE
#define	INC_PUBNUB_CREATE


#include "pubnub_api_types.h"


/** @file pubnub_create.h 

    A helper module to simplify creating a Pubnub context. It is separated
	because it ties the Core API module and the Allocate module.
*/

/** This calls pubnub_alloc() and if it succeeds, then calls pubnub_init()
 * on the returned context. If pubnub_alloc() fails, it just returns NULL.
 * @pre keysub != NULL
 */
pubnub_t *pubnub_create(char const *pubkey, char const *keysub);


#endif /* !defined INC_PUBNUB_CREATE */
