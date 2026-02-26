
#include "pubnub_get_native_socket.h"
#include "pubnub_internal.h"

#include <fcntl.h>

int pbpal_set_socket_blocking_io(
    pubnub_t*             pb,
    pbpal_native_socket_t socket,
    int                   use_blocking_io)
{
    int flags = fcntl((int)socket, F_GETFL, 0);

    PUBNUB_LOG_TRACE(
        pb,
        "%s blocking IO for socket (%s, %X)",
        use_blocking_io ? "Enable" : "Disable",
        flags & O_NONBLOCK ? "non-blocking" : "blocking",
        flags);
    if (-1 == flags) { flags = 0; }
    fcntl((int)socket, F_SETFL, flags | (use_blocking_io ? 0 : O_NONBLOCK));

    flags = fcntl((int)socket, F_GETFL, 0);
    PUBNUB_LOG_TRACE(
        pb,
        "%s blocking IO for socket (%s, %X)",
        use_blocking_io ? "Enabled" : "Disabled",
        flags & O_NONBLOCK ? "non-blocking" : "blocking",
        flags);

    return 0;
}
