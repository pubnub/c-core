
#include "pubnub_get_native_socket.h"
#include <windows.h>

int pbpal_set_socket_blocking_io(pbpal_native_socket_t socket, int use_blocking_io)
{
    u_long iMode = !use_blocking_io;
    ioctlsocket(socket, FIONBIO, &iMode);

    return 0;
}
