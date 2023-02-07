/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_keep_alive.h"

#include "pubnub_internal.h"

void pubnub_set_keep_alive_param(pubnub_t* pb, unsigned timeout, unsigned max) {
    pb->keep_alive.timeout = timeout;
    pb->keep_alive.max = max;
}
