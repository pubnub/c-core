/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"

#include "pubnub_assert.h"
#include "pubnub_blocking_io.h"


int pubnub_set_non_blocking_io(pubnub_t *p)
{
#if PUBNUB_BLOCKING_IO_SETTABLE
    pubnub_mutex_lock(p->monitor);
    p->options.use_blocking_io = false;
    pubnub_mutex_unlock(p->monitor);
    return 0;
#endif
    return -1;
}


int  pubnub_set_blocking_io(pubnub_t *p)
{
#if PUBNUB_BLOCKING_IO_SETTABLE
    pubnub_mutex_lock(p->monitor);
    p->options.use_blocking_io = true;
    pubnub_mutex_unlock(p->monitor);
    return 0;
#endif
    return -1;
}
