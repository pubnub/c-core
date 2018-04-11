/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined(INC_PBPAL_NTF_CALLBACK_POLLER_SELECT)
#define INC_PBPAL_NTF_CALLBACK_POLLER_SELECT

#include "core/pbpal_ntf_callback_poller.h"

#include "pubnub_get_native_socket.h"


#if defined(_WIN32)

/* Winsock explicitly mentions that changing FD_SET_SIZE is OK.  So,
   if you want/need to, you can do it here, before including Winsock2
   header.
 */
#include <winsock2.h>

#endif


struct pbpal_poll_data {
    fd_set    readfds;
    fd_set    writefds;
    fd_set    exceptfds;
    int       nfds;
    size_t    size;
    pubnub_t* apb[FD_SETSIZE];
    pbpal_native_socket_t asocket[FD_SETSIZE];
};


#endif /* !defined(INC_PBPAL_NTF_CALLBACK_POLLER_SELECT) */
