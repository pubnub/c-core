/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pbpal.h"

#include "pubnub_ntf_sync.h"
#include "pubnub_netcore.h"
#include "pubnub_internal.h"
#include "pubnub_assert.h"
#include "pubnub_log.h"

#include <string.h>


static void buf_setup(pubnub_t *pb)
{
    pb->ptr = (uint8_t*)pb->core.http_buf;
    pb->left = sizeof pb->core.http_buf;
}


static int pal_init(void)
{
    static bool s_init = false;
    if (!s_init) {
        pbntf_init();
        s_init = true;
    }
    return 0;
}


void pbpal_init(pubnub_t *pb)
{
    pal_init();
    pb->options.use_blocking_io = false;
    pb->pal.socket = SOCKET_INVALID;
#if PUBNUB_USE_SSL
    pb->options.useSSL = pb->options.fallbackSSL = true;
    pb->options.use_system_certificate_store = false;
    pb->options.reuse_SSL_session = false;
#endif
    pb->sock_state = STATE_NONE;
    pb->readlen = 0;
    buf_setup(pb);
}


int pbpal_send(pubnub_t *pb, void const *data, size_t n)
{
    if (pb->sock_state != STATE_NONE) {
        PUBNUB_LOG_ERROR("pbpal_send(): pb->sock_state != STATE_NONE (=%d)\n", pb->sock_state);
        return -1;
    }
    pb->sendptr = (uint8_t*)data;
    pb->sendlen = n;

    return pbpal_send_status(pb);
}


int pbpal_send_str(pubnub_t *pb, char const *s)
{
    return pbpal_send(pb, s, strlen(s));
}


int pbpal_send_status(pubnub_t *pb)
{
    uint16_t r;
    if (0 == pb->sendlen) {
        return 0;
    }
    r = socket_send(pb->pal.socket, pb->sendptr, pb->sendlen);
    if (r > pb->sendlen) {
        PUBNUB_LOG_WARNING("That's some over-achieving Harmony!\n");
        r = pb->sendlen;
    }
    pb->sendptr += r;
    pb->sendlen -= r;
    return r >= pb->sendlen;
}


int pbpal_start_read_line(pubnub_t *pb)
{
    if (pb->sock_state != STATE_NONE) {
        PUBNUB_LOG_ERROR("pbpal_start_read_line(): pb->sock_state != STATE_NONE: "); WATCH_ENUM(pb->sock_state);
        return -1;
    }

    if (pb->ptr > (uint8_t*)pb->core.http_buf) {
        unsigned distance = pb->ptr - (uint8_t*)pb->core.http_buf;
        memmove(pb->core.http_buf, pb->ptr, pb->readlen);
        pb->ptr -= distance;
        pb->left += distance;
    }
    else {
        if (pb->left == 0) {
            /* Obviously, our buffer is not big enough, maybe some
               error should be reported */
            buf_setup(pb);
        }
    }

    pb->sock_state = STATE_READ_LINE;
    return +1;
}


enum pubnub_res pbpal_line_read_status(pubnub_t *pb)
{
    uint8_t c;

    if (pb->readlen == 0) {
        uint16_t recvres = socket_recv(pb->pal.socket, pb->ptr, pb->left);
        if (0 == recvres) {
            return PNR_IN_PROGRESS;
        }
        PUBNUB_LOG_TRACE("have new data of length=%d: %s\n", recvres, pb->ptr);
        pb->sock_state = STATE_READ_LINE;
        pb->readlen = recvres;
    } 

    while (pb->left > 0 && pb->readlen > 0) {
        c = *pb->ptr++;

        --pb->readlen;
        --pb->left;
        
        if (c == '\n') {
            int pbpal_read_len_ = pbpal_read_len(pb);
            PUBNUB_LOG_TRACE("\\n found: "); WATCH_INT(pbpal_read_len_); WATCH_USHORT(pb->readlen);
            pb->sock_state = STATE_NONE;
            return PNR_OK;
        }
    }

    if (pb->left == 0) {
        /* Buffer has been filled, but new-line char has not been
         * found.  We have to "reset" this "mini-fsm", as otherwise we
         * won't read anything any more. This means that we have lost
         * the current contents of the buffer, which is bad. In some
         * general code, that should be reported, as the caller could
         * save the contents of the buffer somewhere else or simply
         * decide to ignore this line (when it does end eventually).
         */
        pb->sock_state = STATE_NONE;
    }
    else {
        pb->sock_state = STATE_NEWDATA_EXHAUSTED;
    }

    return PNR_IN_PROGRESS;
}


int pbpal_read_len(pubnub_t *pb)
{
    return sizeof pb->core.http_buf - pb->left;
}


int pbpal_start_read(pubnub_t *pb, size_t n)
{
    if (pb->sock_state != STATE_NONE) {
        PUBNUB_LOG_ERROR("pbpal_start_read(): pb->sock_state != STATE_NONE: "); WATCH_ENUM(pb->sock_state);
        return -1;
    }
    if (pb->ptr > (uint8_t*)pb->core.http_buf) {
        unsigned distance = pb->ptr - (uint8_t*)pb->core.http_buf;
        memmove(pb->core.http_buf, pb->ptr, pb->readlen);
        pb->ptr -= distance;
        pb->left += distance;
    }
    else {
        if (pb->left == 0) {
            /* Obviously, our buffer is not big enough, maybe some
               error should be reported */
            buf_setup(pb);
        }
    }
    pb->sock_state = STATE_READ;
    pb->len = n;
    return +1;
}


enum pubnub_res pbpal_read_status(pubnub_t *pb)
{
    unsigned to_read = 0;
    WATCH_ENUM(pb->sock_state);
    WATCH_USHORT(pb->readlen);
    WATCH_USHORT(pb->left);
    WATCH_UINT(pb->len);

    if (pb->readlen == 0) {
        uint16_t recvres;
        to_read =  pb->len - pbpal_read_len(pb);
        if (to_read > pb->left) {
            to_read = pb->left;
        }
        recvres = socket_recv(pb->pal.socket, pb->ptr, to_read);
        if (0 == recvres) {
            return PNR_IN_PROGRESS;
        }
        pb->sock_state = STATE_READ;
        pb->readlen = recvres;
    } 

    to_read = pb->len;
    if (pb->readlen < to_read) {
        to_read = pb->readlen;
    }
    pb->ptr += to_read;
    pb->readlen -= to_read;
    PUBNUB_ASSERT_OPT(pb->left >= to_read);
    pb->left -= to_read;
    pb->len -= to_read;

    if (pb->len == 0) {
        pb->sock_state = STATE_NONE;
        return PNR_OK;
    }

    if (pb->left == 0) {
        /* Buffer has been filled, but the requested block has not been
         * read.  We have to "reset" this "mini-fsm", as otherwise we
         * won't read anything any more. This means that we have lost
         * the current contents of the buffer, which is bad. In some
         * general code, that should be reported, as the caller could
         * save the contents of the buffer somewhere else or simply
         * decide to ignore this block (when it does end eventually).
         */
        pb->sock_state = STATE_NONE;
    }
    else {
        pb->sock_state = STATE_NEWDATA_EXHAUSTED;
    }

    return PNR_IN_PROGRESS;
}


bool pbpal_closed(pubnub_t *pb)
{
    if (pb->pal.socket != SOCKET_INVALID) {
        if (socket_close(pb->pal.socket)) {
            pb->pal.socket = SOCKET_INVALID;
        }
    }
    return SOCKET_INVALID == pb->pal.socket;
}


void pbpal_forget(pubnub_t *pb)
{
    /* a no-op under Harmony */
}


int pbpal_close(pubnub_t *pb)
{
    pb->readlen = 0;
    pb->sock_state = STATE_NONE;
    if (pb->pal.socket != SOCKET_INVALID) {
        pbntf_lost_socket(pb, pb->pal.socket);
        if (socket_close(pb->pal.socket)) {
            pb->pal.socket = SOCKET_INVALID;
        }
    }

    PUBNUB_LOG_TRACE("pbpal_close() returning 0\n");

    return 0;
}
