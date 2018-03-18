/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined(INC_PBPAL_NTF_CALLBACK_POLLER_POLL)
#define      INC_PBPAL_NTF_CALLBACK_POLLER_POLL

#include "core/pbpal_ntf_callback_poller.h"


#if defined(_WIN32)

#include <winsock2.h>
#define PBPAL_POLLFD WSAPOLLFD

#else

#include <poll.h>
#define PBPAL_POLLFD struct pollfd

#endif


struct pbpal_poll_data {
    PBPAL_POLLFD* apoll;
    size_t         size;
    size_t         cap;
    pubnub_t**     apb;
};



#endif  /* !defined(INC_PBPAL_NTF_CALLBACK_POLLER_POLL) */
