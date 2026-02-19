
#include "core/pubnub_api_types.h"
#include "pubnub_get_native_socket.h"
#include <windows.h>

int pbpal_set_socket_blocking_io(pubnub_t* pb, pbpal_native_socket_t socket, int use_blocking_io)
{
    (void)pb;
    u_long iMode = !use_blocking_io;
    ioctlsocket(socket, FIONBIO, &iMode);

    return 0;
}
