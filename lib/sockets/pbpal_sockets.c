/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"

#include "core/pbpal.h"
#include "core/pubnub_ntf_sync.h"
#include "core/pubnub_netcore.h"
#include "core/pubnub_assert.h"
#include "core/pubnub_log.h"

#include <sys/types.h>
#include <fcntl.h>

#include <string.h>


static void buf_setup(pubnub_t* pb)
{
    pb->ptr  = (uint8_t*)pb->core.http_buf;
    pb->left = sizeof pb->core.http_buf / sizeof pb->core.http_buf[0];
}


static int pal_init(void)
{
    static bool s_init = false;
    if (!s_init) {
        if (0 != socket_platform_init()) {
            return -1;
        }
        pbntf_init();
        s_init = true;
    }
    return 0;
}


void pbpal_init(pubnub_t* pb)
{
    pal_init();
    pb->pal.socket = SOCKET_INVALID;
    pb->sock_state = STATE_NONE;
    buf_setup(pb);
#if PUBNUB_USE_MULTIPLE_ADDRESSES
    pbpal_multiple_addresses_reset_counters(&pb->spare_addresses);
#endif
}


int pbpal_send(pubnub_t* pb, void const* data, size_t n)
{
    PUBNUB_ASSERT_INT_OPT(pb->sock_state, ==, STATE_NONE);

    pb->ptr        = (uint8_t*)data;
    pb->len        = (uint16_t)n;
    pb->sock_state = STATE_SENDING_DATA;
    pb->left       = sizeof pb->core.http_buf / sizeof pb->core.http_buf[0];

    return pbpal_send_status(pb);
}


int pbpal_send_str(pubnub_t* pb, char const* s)
{
    return pbpal_send(pb, s, strlen(s));
}


int pbpal_send_status(pubnub_t* pb)
{
    int rslt;

    if (0 == pb->len) {
        return 0;
    }

    PUBNUB_ASSERT_OPT(pb->sock_state == STATE_SENDING_DATA);

    rslt = socket_send(pb->pal.socket, (char*)pb->ptr, pb->len);
    if (rslt <= 0) {
        rslt = (pbpal_handle_socket_error(rslt, pb, __FILE__, __LINE__) == PNR_IN_PROGRESS) ? +1 : -1;
    }
    else {
        PUBNUB_ASSERT_OPT((unsigned)rslt <= pb->len);
        pb->ptr += rslt;
        pb->len -= rslt;
        rslt = (0 == pb->len) ? 0 : +1;
    }

    if (rslt <= 0) {
        pb->ptr        = (uint8_t*)pb->core.http_buf;
        pb->unreadlen  = 0;
        pb->sock_state = STATE_NONE;
    }

    return rslt;
}


int pbpal_start_read_line(pubnub_t* pb)
{
    unsigned distance;

    PUBNUB_ASSERT_INT_OPT(pb->sock_state, ==, STATE_NONE);

    if (pb->unreadlen > 0) {
        PUBNUB_ASSERT_OPT((char*)pb->ptr + pb->unreadlen
                          <= pb->core.http_buf + PUBNUB_BUF_MAXLEN);
        memmove(pb->core.http_buf, pb->ptr, pb->unreadlen);
    }
    distance = pb->ptr - (uint8_t*)pb->core.http_buf;
    PUBNUB_ASSERT_UINT(distance + pb->left + pb->unreadlen,
                       ==,
                       sizeof pb->core.http_buf / sizeof pb->core.http_buf[0]);
    pb->ptr -= distance;
    pb->left += distance;

    pb->sock_state = STATE_READ_LINE;

    return +1;
}


enum pubnub_res pbpal_line_read_status(pubnub_t* pb)
{
    PUBNUB_ASSERT_OPT(STATE_READ_LINE == pb->sock_state);

    if (pb->unreadlen == 0) {
        int recvres;
        PUBNUB_ASSERT_OPT((char*)pb->ptr + pb->left
                          == pb->core.http_buf + PUBNUB_BUF_MAXLEN);
        recvres = socket_recv(pb->pal.socket, (char*)pb->ptr, pb->left, 0);
        if (recvres <= 0) {
            return pbpal_handle_socket_error(recvres, pb, __FILE__, __LINE__);
        }
        PUBNUB_ASSERT_OPT(recvres <= pb->left);
        PUBNUB_LOG_TRACE(
            "pb=%p have new data of length=%d: %.*s\n", pb, recvres, recvres, pb->ptr);
        pb->unreadlen = recvres;
        pb->left -= recvres;
    }

    while (pb->unreadlen > 0) {
        uint8_t c;

        --pb->unreadlen;

        c = *pb->ptr++;
        if (c == '\n') {
            PUBNUB_LOG_TRACE("pb=%p, newline found, line length: %d, ",
                             pb,
                             pbpal_read_len(pb));
            WATCH_USHORT(pb->unreadlen);
            pb->sock_state = STATE_NONE;
            return PNR_OK;
        }
    }

    if (pb->left == 0) {
        PUBNUB_LOG_ERROR(
            "pbpal_line_read_status(pb=%p): buffer full but newline not found", pb);
        pb->sock_state = STATE_NONE;
        return PNR_TX_BUFF_TOO_SMALL;
    }

    return PNR_IN_PROGRESS;
}


int pbpal_read_len(pubnub_t* pb)
{
    return (char*)pb->ptr - pb->core.http_buf;
}


int pbpal_start_read(pubnub_t* pb, size_t n)
{
    unsigned distance;

    PUBNUB_ASSERT_UINT_OPT(n, >, 0);
    PUBNUB_ASSERT_INT_OPT(pb->sock_state, ==, STATE_NONE);

    WATCH_USHORT(pb->unreadlen);
    WATCH_USHORT(pb->left);
    if (pb->unreadlen > 0) {
        PUBNUB_ASSERT_OPT((char*)pb->ptr + pb->unreadlen
                          <= pb->core.http_buf + PUBNUB_BUF_MAXLEN);
        memmove(pb->core.http_buf, pb->ptr, pb->unreadlen);
    }
    distance = pb->ptr - (uint8_t*)pb->core.http_buf;
    WATCH_UINT(distance);
    PUBNUB_ASSERT_UINT(distance + pb->unreadlen + pb->left,
                       ==,
                       sizeof pb->core.http_buf / sizeof pb->core.http_buf[0]);
    pb->ptr -= distance;
    pb->left += distance;

    pb->sock_state = STATE_READ;
    pb->len        = n;

    return +1;
}


enum pubnub_res pbpal_read_status(pubnub_t* pb)
{
    int have_read;

    PUBNUB_ASSERT_OPT(STATE_READ == pb->sock_state);

    if (0 == pb->unreadlen) {
        unsigned to_recv = pb->len;
        if (to_recv > pb->left) {
            to_recv = pb->left;
        }
        PUBNUB_ASSERT_OPT(to_recv > 0);
        have_read = socket_recv(pb->pal.socket, (char*)pb->ptr, to_recv, 0);
        if (have_read <= 0) {
            return pbpal_handle_socket_error(have_read, pb, __FILE__, __LINE__);
        }
        PUBNUB_ASSERT_OPT(pb->left >= have_read);
        pb->left -= have_read;
    }
    else {
        have_read = (pb->unreadlen >= pb->len) ? pb->len : pb->unreadlen;
        pb->unreadlen -= have_read;
    }

    pb->len -= have_read;
    pb->ptr += have_read;

    if ((0 == pb->len) || (0 == pb->left)) {
        pb->sock_state = STATE_NONE;
        return PNR_OK;
    }

    return PNR_IN_PROGRESS;
}


bool pbpal_closed(pubnub_t* pb)
{
    return pb->pal.socket == SOCKET_INVALID;
}


void pbpal_forget(pubnub_t* pb)
{
    /* `socket_close` pretty much means "forget" in BSD-ish sockets */
    PUBNUB_UNUSED(pb);
}


int pbpal_close(pubnub_t* pb)
{
    pb->unreadlen = 0;
    if (pb->pal.socket != SOCKET_INVALID) {
        pbntf_lost_socket(pb);
        socket_close(pb->pal.socket);
        pb->pal.socket = SOCKET_INVALID;
        pb->sock_state = STATE_NONE;
    }

    PUBNUB_LOG_TRACE("pbpal_close(pb=%p) returning 0\n", pb);

    return 0;
}

void pbpal_free(pubnub_t* pb)
{
    if (pb->pal.socket != SOCKET_INVALID) {
        /* While this should not happen, it doesn't hurt to be paranoid.
         */
        pbntf_lost_socket(pb);
        socket_close(pb->pal.socket);
    }
}
