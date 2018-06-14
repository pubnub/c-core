
#include "pubnub_get_native_socket.h"
#include "core/pubnub_log.h"
#include "pubnub_internal.h"

#include <fcntl.h>

int pbpal_set_socket_blocking_io(pbpal_native_socket_t socket, int use_blocking_io)
{
    int flags = fcntl((int)socket, F_GETFL, 0);

    PUBNUB_LOG_TRACE("pbpal_set_socket_blocking_io(): before - flags = %X, flags&NONBLOCK = %X\n", flags, flags & O_NONBLOCK);
    if (-1 == flags) {
        flags = 0;
    }
    fcntl((int)socket, F_SETFL, flags | (use_blocking_io ? 0 : O_NONBLOCK));

    flags = fcntl((int)socket, F_GETFL, 0);
    PUBNUB_LOG_TRACE("pbpal_set_socket_blocking_io(): after - flags = %X, flags&NONBLOCK = %X\n", flags, flags & O_NONBLOCK);

    return 0;
}
