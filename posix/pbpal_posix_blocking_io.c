/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "core/pbpal.h"
#include "lib/sockets/pbpal_socket_blocking_io.h"
#include "core/pubnub_assert.h"

#include "pubnub_internal.h"


int pbpal_set_blocking_io(pubnub_t* pb)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    return pbpal_set_socket_blocking_io(pb->pal.socket, pb->options.use_blocking_io);
}
