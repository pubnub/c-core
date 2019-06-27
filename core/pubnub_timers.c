/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_timers.h"

#include "pubnub_assert.h"
#include "pubnub_log.h"
#include "pubnub_internal.h"


int pubnub_set_transaction_timeout(pubnub_t* p, int duration_ms)
{
    PUBNUB_ASSERT_OPT(p != NULL);
    PUBNUB_ASSERT_OPT(duration_ms > 0);
    if (duration_ms < PUBNUB_MIN_TRANSACTION_TIMER) {
        duration_ms = PUBNUB_MIN_TRANSACTION_TIMER;
        PUBNUB_LOG_WARNING(
            "Using minimal transaction timeout %d instead of given %d\n",
            PUBNUB_MIN_TRANSACTION_TIMER,
            duration_ms);
    }
    p->transaction_timeout_ms = duration_ms;
    return 0;
}


int pubnub_transaction_timeout_get(pubnub_t* p)
{
    PUBNUB_ASSERT_OPT(p != NULL);
    return p->transaction_timeout_ms;
}
