/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"

#include "pubnub_timers.h"
#include "pubnub_assert.h"
#include "pubnub_log.h"


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
    pubnub_mutex_lock(p->monitor);
    p->transaction_timeout_ms = duration_ms;
    pubnub_mutex_unlock(p->monitor);
    return 0;
}


int pubnub_set_wait_connect_timeout(pubnub_t* p, int duration_ms)
{
    PUBNUB_ASSERT_OPT(p != NULL);
    PUBNUB_ASSERT_OPT(duration_ms > 0);
    if (duration_ms < PUBNUB_MIN_WAIT_CONNECT_TIMER) {
        duration_ms = PUBNUB_MIN_WAIT_CONNECT_TIMER;
        PUBNUB_LOG_WARNING(
            "Using minimal 'wait_connect_TCP_socket' timeout %d instead of given %d\n",
            PUBNUB_MIN_WAIT_CONNECT_TIMER,
            duration_ms);
    }
    pubnub_mutex_lock(p->monitor);
    p->wait_connect_timeout_ms = duration_ms;
    pubnub_mutex_unlock(p->monitor);
    return 0;
}


int pubnub_transaction_timeout_get(pubnub_t* p)
{
    int timeout_ms;

    PUBNUB_ASSERT_OPT(p != NULL);
    pubnub_mutex_lock(p->monitor);
    timeout_ms = p->transaction_timeout_ms;
    pubnub_mutex_unlock(p->monitor);

    return timeout_ms;
}


int pubnub_wait_connect_timeout_get(pubnub_t* p)
{
    int timeout_ms;

    PUBNUB_ASSERT_OPT(p != NULL);
    pubnub_mutex_lock(p->monitor);
    timeout_ms = p->wait_connect_timeout_ms;
    pubnub_mutex_unlock(p->monitor);

    return timeout_ms;
}
