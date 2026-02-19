/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_SOCKET_BLOCKING_IO
#define INC_SOCKET_BLOCKING_IO

#include "core/pubnub_api_types.h"
#include "pubnub_get_native_socket.h"

int pbpal_set_socket_blocking_io(pubnub_t* pb, pbpal_native_socket_t socket, int use_blocking_io);

#endif /* INC_SOCKET_BLOCKING_IO */
