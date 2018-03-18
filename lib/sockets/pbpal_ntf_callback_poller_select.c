/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"

#include "lib/sockets/pbpal_ntf_callback_poller_select.h"

#include "core/pubnub_assert.h"
#include "core/pubnub_log.h"

#include <stdlib.h>


struct pbpal_poll_data* pbpal_ntf_callback_poller_init(void)
{
    struct pbpal_poll_data* rslt;

    rslt = (struct pbpal_poll_data*)malloc(sizeof *rslt);
    if (NULL == rslt) {
        return NULL;
    }
    FD_ZERO(&rslt->readfds);
    FD_ZERO(&rslt->writefds);
    FD_ZERO(&rslt->exceptfds);
    rslt->size = rslt->nfds = 0;

    return rslt;
}


static bool we_ve_got_ya(struct pbpal_poll_data const* data, pubnub_t const* pb)
{
    size_t i;
    for (i = 0; i < data->size; ++i) {
        if (data->apb[i] == pb) {
            return true;
        }
    }
    return false;
}


void pbpal_ntf_callback_save_socket(struct pbpal_poll_data* data, pubnub_t* pb)
{
    pbpal_native_socket_t sockt = pubnub_get_native_socket(pb);

    PUBNUB_ASSERT_OPT(data != NULL);

    if (INVALID_SOCKET == sockt) {
        return;
    }
    PUBNUB_ASSERT(!FD_ISSET(sockt, &data->exceptfds));
    PUBNUB_ASSERT(!FD_ISSET(sockt, &data->writefds));
    PUBNUB_ASSERT(!FD_ISSET(sockt, &data->readfds));
    PUBNUB_ASSERT_EX(!we_ve_got_ya(data, pb));

    if ((int)sockt > data->nfds) {
        data->nfds = sockt;
    }
    FD_SET(sockt, &data->exceptfds);
    FD_SET(sockt, &data->writefds);
    data->apb[data->size]     = pb;
    data->asocket[data->size] = sockt;
    ++data->size;
}


void pbpal_ntf_callback_remove_socket(struct pbpal_poll_data* data, pubnub_t* pb)
{
    size_t                i;
    int                   new_nfds = 0;
    pbpal_native_socket_t sockt    = pubnub_get_native_socket(pb);

    PUBNUB_ASSERT_OPT(data != NULL);

    if (INVALID_SOCKET == sockt) {
        return;
    }
    PUBNUB_ASSERT(FD_ISSET(sockt, &data->exceptfds));
    PUBNUB_ASSERT_EX(we_ve_got_ya(data, pb));

    for (i = 0; i < data->size; ++i) {
        pbpal_native_socket_t i_sckt = data->asocket[i];
        PUBNUB_ASSERT(pubnub_get_native_socket(data->apb[i]) == data->asocket[i]);
        if ((int)i_sckt > new_nfds) {
            new_nfds = i_sckt;
        }
        if (data->apb[i] == pb) {
            size_t to_move = data->size - i - 1;
            if (to_move > 0) {
                memmove(data->apb + i, data->apb + i + 1, sizeof data->apb[0] * to_move);
                memmove(data->asocket + i,
                        data->asocket + i + 1,
                        sizeof data->asocket[0] * to_move);
            }
            --data->size;
            break;
        }
    }

    FD_CLR(sockt, &data->exceptfds);
    FD_CLR(sockt, &data->writefds);
    FD_CLR(sockt, &data->readfds);

    for (; i < data->size; ++i) {
        pbpal_native_socket_t i_sckt = data->asocket[i];
        PUBNUB_ASSERT(pubnub_get_native_socket(data->apb[i]) == data->asocket[i]);
        if ((int)i_sckt > new_nfds) {
            new_nfds = i_sckt;
        }
    }
    data->nfds = new_nfds;
}


void pbpal_ntf_callback_update_socket(struct pbpal_poll_data* data, pubnub_t* pb)
{
    size_t i;

    PUBNUB_ASSERT_OPT(data != NULL);
    for (i = 0; i < data->size; ++i) {
        if (data->apb[i] == pb) {
            pbpal_native_socket_t sckt = data->asocket[i];

            FD_CLR(sckt, &data->readfds);
            FD_CLR(sckt, &data->readfds);
            FD_CLR(sckt, &data->readfds);

            sckt = pubnub_get_native_socket(data->apb[i]);
            FD_CLR(sckt, &data->readfds);
            FD_SET(sckt, &data->writefds);
            FD_SET(sckt, &data->exceptfds);
            data->asocket[i] = sckt;
            break;
        }
    }
}


int pbpal_ntf_watch_out_events(struct pbpal_poll_data* data, pubnub_t* pbp)
{
    pbpal_native_socket_t scket = pubnub_get_native_socket(pbp);

    PUBNUB_ASSERT_OPT(data != NULL);
    if (!we_ve_got_ya(data, pbp)) {
        return -1;
    }

    FD_CLR(scket, &data->readfds);
    FD_SET(scket, &data->writefds);

    return 0;
}


int pbpal_ntf_watch_in_events(struct pbpal_poll_data* data, pubnub_t* pbp)
{
    pbpal_native_socket_t scket = pubnub_get_native_socket(pbp);

    PUBNUB_ASSERT_OPT(data != NULL);
    if (!we_ve_got_ya(data, pbp)) {
        return -1;
    }

    FD_SET(scket, &data->readfds);
    FD_CLR(scket, &data->writefds);

    return 0;
}


int pbpal_ntf_poll_away(struct pbpal_poll_data* data, int ms)
{
    int            i;
    int            rslt;
    fd_set         readfds;
    fd_set         writefds;
    fd_set         exceptfds;
    struct timeval timeout;

    if (0 == data->size) {
        return 0;
    }

    timeout.tv_sec  = ms / 1000;
    timeout.tv_usec = (ms % 1000) * 1000;

    memcpy(&readfds, &data->readfds, sizeof readfds);
    memcpy(&writefds, &data->writefds, sizeof writefds);
    memcpy(&exceptfds, &data->exceptfds, sizeof exceptfds);
    rslt = select(data->nfds + 1, &readfds, &writefds, &exceptfds, &timeout);
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
            "poll size = %u, error = %d\n", (unsigned)data->size, last_err);
        return -1;
    }
    for (i = 0; (i < (int)data->size) && (rslt > 0); ++i) {
        bool should_process = false;
        if (FD_ISSET(data->asocket[i], &readfds)) {
            should_process = true;
            --rslt;
        }
        if (FD_ISSET(data->asocket[i], &writefds)) {
            should_process = true;
            --rslt;
            PUBNUB_ASSERT_OPT(rslt >= 0);
        }
        if (FD_ISSET(data->asocket[i], &exceptfds)) {
            should_process = true;
            --rslt;
            PUBNUB_ASSERT_OPT(rslt >= 0);
        }
        if (should_process) {
            pbntf_requeue_for_processing(data->apb[i]);
        }
    }
    PUBNUB_ASSERT_OPT(0 == rslt);

    return rslt;
}


void pbpal_ntf_callback_poller_deinit(struct pbpal_poll_data** data)
{
    PUBNUB_ASSERT_OPT(data != NULL);
    PUBNUB_ASSERT_OPT(*data != NULL);

    free(*data);
    *data = NULL;
}
