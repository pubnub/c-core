/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_blocking_io.h"

#include "pubnub_assert.h"
#include "pubnub_internal.h"




int pubnub_set_non_blocking_io(pubnub_t *p)
{
    if (PUBNUB_BLOCKING_IO_SETTABLE) {
        p->options.use_blocking_io = false;
        return 0;
    }
    return -1;
}


int  pubnub_set_blocking_io(pubnub_t *p)
{
    if (PUBNUB_BLOCKING_IO_SETTABLE) {
        p->options.use_blocking_io = true;
        return 0;
    }
    return -1;
}
