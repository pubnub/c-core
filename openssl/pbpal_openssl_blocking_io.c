/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "core/pbpal.h"

#include "pubnub_internal.h"

#include <openssl/ssl.h>


int socket_platform_init(void)
{
#if defined(_WIN32)
    WSADATA wsadata;
    printf("socket_platform_init()\n");
    return WSAStartup(MAKEWORD(2,2), &wsadata);
#endif
    return 0;
}


int pbpal_set_blocking_io(pubnub_t *pb)
{
    /* You can't just change blocking/non-blocking I/O w/OpenSSL.  It
       is set before connecting to the server. So, at that time, we
       shall read the pb->pb->use_blocking_io and use it accordingly,
       but, this function simply can't work.
    */
    return -1;
}
