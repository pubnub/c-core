/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"

#include "lib/sockets/pbpal_ntf_callback_poller_poll.h"

#include "pubnub_get_native_socket.h"
#include "core/pb_sleep_ms.h"

#include "core/pubnub_assert.h"
#include "core/pubnub_log.h"

#include <string.h>


#if defined(_WIN32)
/* Yes, we do know that it's not really that simple, there are subtle,
   yet significant, differences between WSAPoll() and BSD sockets
   poll(), but, for our purposes, this will do.
*/
#define poll(fdarray, nfds, timeout) WSAPoll(fdarray, nfds, timeout)
#endif

#if !defined(INVALID_SOCKET)
#define INVALID_SOCKET -1
#endif

#if !defined(SOCKET_ERROR)
#define SOCKET_ERROR -1
#endif


struct pbpal_poll_data* pbpal_ntf_callback_poller_init(void)
{
    struct pbpal_poll_data* rslt;

    rslt = (struct pbpal_poll_data*)malloc(sizeof *rslt);
    if (NULL == rslt) {
        return NULL;
    }
    rslt->size = rslt->cap = 0;
    rslt->apoll            = NULL;
    rslt->apb              = NULL;

    return rslt;
}


void pbpal_ntf_callback_save_socket(struct pbpal_poll_data* data, pubnub_t* pb)
{
    size_t                i;
    pbpal_native_socket_t sockt = pubnub_get_native_socket(pb);
    if (INVALID_SOCKET == sockt) {
        return;
    }
    for (i = 0; i < data->size; ++i) {
        PUBNUB_ASSERT_OPT(data->apoll[i].fd != sockt);
        PUBNUB_ASSERT_OPT(data->apb[i] != pb);
    }
    if (data->size == data->cap) {
        size_t const   newcap = data->size + 2;
        struct pollfd* npalloc =
            (struct pollfd*)realloc(data->apoll, sizeof data->apoll[0] * newcap);
        pubnub_t** npapb =
            (pubnub_t**)realloc(data->apb, sizeof data->apb[0] * newcap);
        if (NULL == npalloc) {
            if (npapb != NULL) {
                data->apb = npapb;
            }
            return;
        }
        else if (NULL == npapb) {
            data->apoll = npalloc;
            return;
        }
        data->apoll = npalloc;
        data->apb   = npapb;
        data->cap   = newcap;
    }

    data->apoll[data->size].fd     = sockt;
    data->apoll[data->size].events = POLLOUT;
    data->apb[data->size]          = pb;
    ++data->size;
}


void pbpal_ntf_callback_remove_socket(struct pbpal_poll_data* data, pubnub_t* pb)
{
    size_t                i;
    pbpal_native_socket_t sockt = pubnub_get_native_socket(pb);
    if (INVALID_SOCKET == sockt) {
        return;
    }
    for (i = 0; i < data->size; ++i) {
        if (data->apoll[i].fd == sockt) {
            size_t to_move = data->size - i - 1;
            PUBNUB_ASSERT(data->apb[i] == pb);
            if (to_move > 0) {
                memmove(data->apoll + i,
                        data->apoll + i + 1,
                        sizeof data->apoll[0] * to_move);
                memmove(data->apb + i, data->apb + i + 1, sizeof data->apb[0] * to_move);
            }
            --data->size;
            return;
        }
    }
    PUBNUB_LOG_DEBUG(
        "pbpal_ntf_callback_remove_socket(pb=%p) sockt=%d: Not Found!", pb, sockt);
}


void pbpal_ntf_callback_update_socket(struct pbpal_poll_data* data, pubnub_t* pb)
{
    pbpal_native_socket_t sockt = pubnub_get_native_socket(pb);
    if (sockt != INVALID_SOCKET) {
        size_t i;
        for (i = 0; i < data->size; ++i) {
            PUBNUB_ASSERT_OPT((data->apb[i] != pb) ? (data->apoll[i].fd != sockt)
                                                   : true);
        }
        for (i = 0; i < data->size; ++i) {
            if (data->apb[i] == pb) {
                data->apoll[i].fd = sockt;
                return;
            }
        }
    }
    PUBNUB_LOG_WARNING(
        "pbpal_ntf_callback_update_socket(pb=%p) sockt=%d: Not Found!", pb, sockt);
}


int pbpal_ntf_watch_out_events(struct pbpal_poll_data* data, pubnub_t* pbp)
{
    unsigned i;
    for (i = 0; i < data->size; ++i) {
        if (data->apb[i] == pbp) {
            data->apoll[i].events = POLLOUT;
            return 0;
        }
    }
    PUBNUB_LOG_WARNING("pbpal_ntf_watch_out_events(pbp=%p): Not Found!", pbp);
    return -1;
}


int pbpal_ntf_watch_in_events(struct pbpal_poll_data* data, pubnub_t* pbp)
{
    unsigned i;
    for (i = 0; i < data->size; ++i) {
        if (data->apb[i] == pbp) {
            data->apoll[i].events = POLLIN;
            return 0;
        }
    }
    PUBNUB_LOG_WARNING("pbpal_ntf_watch_in_events(pbp=%p): Not Found!", pbp);
    return -1;
}


int pbpal_ntf_poll_away(struct pbpal_poll_data* data, int ms)
{
    int rslt;

    if (0 == data->size) {
        pb_sleep_ms(1);
        return 0;
    }

    rslt = poll(data->apoll, data->size, ms);
    if (SOCKET_ERROR == rslt) {
        int last_err =
#if defined(_WIN32)
            WSAGetLastError()
#else
            errno
#endif
            ;
        /* error? what to do about it? */
        PUBNUB_LOG_WARNING(
            "poll size = %ud, error = %d\n", (unsigned)data->size, last_err);
        return -1;
    }
    if (rslt > 0) {
        size_t i;
        size_t apoll_size = data->size;
        for (i = 0; i < apoll_size; ++i) {
            if (data->apoll[i].revents & (POLLIN | POLLOUT | POLLERR | POLLHUP | POLLNVAL)) {
                pbntf_requeue_for_processing(data->apb[i]);
            }
        }
    }

    return rslt;
}


void pbpal_ntf_callback_poller_deinit(struct pbpal_poll_data** data)
{
    PUBNUB_ASSERT_OPT(data != NULL);
    PUBNUB_ASSERT_OPT(*data != NULL);

    free(*data);
    *data = NULL;
}
