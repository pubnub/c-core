/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pbpal.h"

#include "pubnub_internal.h"

#include <openssl/ssl.h>


int pbpal_set_blocking_io(pubnub_t *pb)
{
    /* You can't just change blocking/non-blocking I/O w/OpenSSL.  It
       is set before connecting to the server. So, at that time, we
       shall read the pb->pb->use_blocking_io and use it accordingly,
       but, this function simply can't work.
    */
    return -1;
}
