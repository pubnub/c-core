/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pbpal.h"

#include "pubnub_internal.h"

#include <fcntl.h>


int pbpal_set_blocking_io(pubnub_t *pb)
{
    int flags = fcntl(pb->pal.socket, F_GETFL, 0);
    
    if (-1 == flags) {
        flags = 0;
    }
    fcntl(pb->pal.socket, F_SETFL, flags | (pb->options.use_blocking_io ? 0 : O_NONBLOCK));

    return 0;
}
