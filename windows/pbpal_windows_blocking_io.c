/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pbpal.h"

#include "pubnub_internal.h"

#include <windows.h>


int pbpal_set_blocking_io(pubnub_t *pb)
{
    u_long iMode = pb->use_blocking_io;
    ioctlsocket(pb->pal.socket, FIONBIO, &iMode);

    return 0;
}
