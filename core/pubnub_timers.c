/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_timers.h"

#include "pubnub_assert.h"
#include "pubnub_internal.h"


int pubnub_set_transaction_timeout(pubnub_t* p, int duration_ms)
{
    PUBNUB_ASSERT_OPT(p != NULL);
    PUBNUB_ASSERT_OPT(duration_ms > 0);
    if (duration_ms < PUBNUB_MIN_TRANSACTION_TIMER) {
        return -1;
    }
    p->transaction_timeout_ms = duration_ms;
    return 0;
}


int pubnub_transaction_timeout_get(pubnub_t* p)
{
    PUBNUB_ASSERT_OPT(p != NULL);
    return p->transaction_timeout_ms;
}
