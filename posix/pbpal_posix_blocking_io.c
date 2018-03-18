/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "core/pbpal.h"

#include "pubnub_internal.h"
#include "core/pubnub_log.h"

#include <fcntl.h>


int pbpal_set_blocking_io(pubnub_t *pb)
{
    int flags = fcntl(pb->pal.socket, F_GETFL, 0);

    PUBNUB_LOG_TRACE("pbpal_set_blocking_io(): before - flags = %X, flags&NONBLOCK = %X\n", flags, flags & O_NONBLOCK);
    if (-1 == flags) {
        flags = 0;
    }
    fcntl(pb->pal.socket, F_SETFL, flags | (pb->options.use_blocking_io ? 0 : O_NONBLOCK));

    flags = fcntl(pb->pal.socket, F_GETFL, 0);
    PUBNUB_LOG_TRACE("pbpal_set_blocking_io(): after - flags = %X, flags&NONBLOCK = %X\n", flags, flags & O_NONBLOCK);

    return 0;
}
