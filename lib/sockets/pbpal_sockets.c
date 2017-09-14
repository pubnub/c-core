/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pbpal.h"

#include "pubnub_ntf_sync.h"
#include "pubnub_netcore.h"
#include "pubnub_internal.h"
#include "pubnub_assert.h"
#include "pubnub_log.h"

#include <sys/types.h>
#include <fcntl.h>

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
        if (0 != socket_platform_init()) {
            return -1;
        }
        pbntf_init();
        s_init = true;
    }
    return 0;
}



void pbpal_init(pubnub_t *pb)
{
    if (PUBNUB_BLOCKING_IO_SETTABLE) {
        pb->options.use_blocking_io = true;
    }
    pal_init();
    pb->pal.socket = SOCKET_INVALID;
    pb->sock_state = STATE_NONE;
    pb->readlen = 0;
    buf_setup(pb);
}


int pbpal_send(pubnub_t *pb, void const *data, size_t n)
{
    if (n == 0) {
        return 0;
    }
    if (pb->sock_state != STATE_NONE) {
        PUBNUB_LOG_ERROR("pbpal_send(): pb->sock_state != STATE_NONE (=%d)\n", pb->sock_state);
        return -1;
    }
    pb->sendptr = (uint8_t*)data;
    pb->sendlen = (uint16_t)n;
    pb->sock_state = STATE_NONE;

    return pbpal_send_status(pb);
}


int pbpal_send_str(pubnub_t *pb, char const *s)
{
    return pbpal_send(pb, s, strlen(s));
}


int pbpal_send_status(pubnub_t *pb)
{
    int r;
    if (0 == pb->sendlen) {
        return 0;
    }
    r = socket_send(pb->pal.socket, (char*)pb->sendptr, pb->sendlen, 0);
    if (r < 0) {
        return socket_would_block() ? +1 : -1;
    }
    if (r > pb->sendlen) {
        PUBNUB_LOG_WARNING("That's some over-achieving socket!\n");
        r = pb->sendlen;
    }
    pb->sendptr += r;
    pb->sendlen -= r;

    return (0 == pb->sendlen) ? 0 : +1;
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


static void report_error_from_environment(pubnub_t* pb)
{
    char const* err_str;
                
#if HAVE_STRERROR_R
    char errstr_r[1024];
    strerror_r(errno, errstr_r, sizeof errstr_r / sizeof errstr_r[0]);
    err_str = errstr_r;
#else
    err_str = strerror(errno);
#endif
    PUBNUB_LOG_TRACE("pbpal_line_read_status(pb=%p): errno=%d('%s') use_blocking_io=%d\n", pb, errno, err_str, pb->options.use_blocking_io);
#if defined(_WIN32)
    PUBNUB_LOG_TRACE("pbpal_line_read_status(pb=%p): GetLastErrror()=%d WSAGetLastError()=%d\n", 
                     pb, GetLastError(), WSAGetLastError()
        );
#endif
}


static enum pubnub_res pbrslt_from_socket_error(int socket_result, pubnub_t *pb)
{
    PUBNUB_ASSERT_OPT(socket_result <= 0);
    if (socket_result < 0) {
        if (socket_would_block()) {
            if (PUBNUB_BLOCKING_IO_SETTABLE && pb->options.use_blocking_io) {
                return PNR_TIMEOUT;
            }
            return PNR_IN_PROGRESS;
        }
        else {
            report_error_from_environment(pb);
            return socket_timed_out() ? PNR_CONNECTION_TIMEOUT : PNR_IO_ERROR;
        }
    }
    else if (0 == socket_result) {
        return PNR_TIMEOUT;
    }
    return PNR_INTERNAL_ERROR;
}


enum pubnub_res pbpal_line_read_status(pubnub_t *pb)
{
    uint8_t c;

    if (pb->readlen == 0) {
        int recvres = socket_recv(pb->pal.socket, (char*)pb->ptr, pb->left, 0);
        if (recvres <= 0) {
            return pbrslt_from_socket_error(recvres, pb);
        }
        PUBNUB_LOG_TRACE("pb=%p have new data of length=%d: %.*s\n", pb, recvres, recvres, pb->ptr);
        pb->sock_state = STATE_READ_LINE;
        pb->readlen = recvres;
    }

    while (pb->left > 0 && pb->readlen > 0) {
        c = *pb->ptr++;

        --pb->readlen;
        --pb->left;
        
        if (c == '\n') {
            int read_len = pbpal_read_len(pb);
            PUBNUB_LOG_TRACE("\n found: "); WATCH_INT(read_len); WATCH_USHORT(pb->readlen);
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
            PUBNUB_LOG_ERROR("pbpal_start_read(pb=%p) pb->left=0, dismissing old buffer\n", pb);
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
        int recvres;
        to_read =  pb->len;
        if (to_read > pb->left) {
            to_read = pb->left;
        }
        recvres = socket_recv(pb->pal.socket, (char*)pb->ptr, to_read, 0);
        if (recvres <= 0) {
            return pbrslt_from_socket_error(recvres, pb);
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
    return pb->pal.socket == SOCKET_INVALID;
}


void pbpal_forget(pubnub_t *pb)
{
    /* `socket_close` pretty much means "forget" in BSD-ish sockets */
    PUBNUB_UNUSED(pb);
}


int pbpal_close(pubnub_t *pb)
{
    pb->readlen = 0;
    if (pb->pal.socket != SOCKET_INVALID) {
        pbntf_lost_socket(pb, pb->pal.socket);
        socket_close(pb->pal.socket);
        pb->pal.socket = SOCKET_INVALID;
        pb->sock_state = STATE_NONE;
    }

    PUBNUB_LOG_TRACE("pbpal_close() returning 0\n");

    return 0;
}

void pbpal_free(pubnub_t *pb)
{
    if (pb->pal.socket != SOCKET_INVALID) {
        /* While this should not happen, it doesn't hurt to be paranoid.
         */
        pbntf_lost_socket(pb, pb->pal.socket);
        socket_close(pb->pal.socket);
    }
}
