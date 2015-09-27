/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_create.h"

#include "pubnub_assert.h"
#include "pubnub_alloc.h"
#include "pubnub_coreapi.h"


pubnub_t *pubnub_create(char const *pubkey, char const *keysub)
{
    pubnub_t *result = pubnub_alloc();
	PUBNUB_ASSERT(keysub != NULL);

    if (NULL == result) {
        return NULL;
    }
    return pubnub_init(result, pubkey, keysub);
}
