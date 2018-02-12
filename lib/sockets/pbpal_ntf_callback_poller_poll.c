/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"

#include "pbpal_ntf_callback_poller_poll.h"

#include "pubnub_get_native_socket.h"

#include "pubnub_assert.h"


void pbpal_ntf_callback_save_socket(struct pbpal_poll_data* data, pubnub_t* pb)
{
    size_t                i;
    pbpal_native_socket_t sockt = pubnub_get_native_socket(pb);
    if (INVALID_SOCKET == sockt) {
        return;
    }
    for (i = 0; i < data->size; ++i) {
        PUBNUB_ASSERT_OPT(data->apoll[i].fd != sockt);
    }
    if (data->size == data->cap) {
        size_t         newcap = data->size + 2;
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
                memmove(data->apb + i,
                        data->apb + i + 1,
                        sizeof data->apb[0] * to_move);
            }
            --data->size;
            break;
        }
    }
}


void pbpal_ntf_callback_update_socket(struct pbpal_poll_data* data, pubnub_t* pb)
{
    size_t i;
    int    socket = pubnub_get_native_socket(pb);
    if (-1 == socket) {
        return;
    }

    for (i = 0; i < data->size; ++i) {
        if (data->apb[i] == pb) {
            data->apoll[i].fd = socket;
            return;
        }
    }
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
    return -1;
}
