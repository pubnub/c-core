#include "pubnub_timers.h"

#include "pubnub_assert.h"
#include "pubnub_internal.h"


int pubnub_set_transaction_timeout(pubnub_t *p, unsigned duration_ms)
{
    PUBNUB_ASSERT_OPT(p != NULL);
    p->transaction_timeout_ms = duration_ms;
    return 0;
}


unsigned pubnub_transaction_timeout_get(pubnub_t *p)
{
    PUBNUB_ASSERT_OPT(p != NULL);
    return p->transaction_timeout_ms;
}

