/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pbpal.h"

#include "pubnub_ntf_sync.h"
#include "pubnub_netcore.h"
#include "pubnub_internal.h"
#include "pubnub_assert.h"

#include <sys/types.h>

#include <string.h>
#include <errno.h>


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
        DEBUG_PRINTF("pbpal_send(): pb->sock_state != STATE_NONE (=%d)\n", pb->sock_state);
        return -1;
    }
    pb->sendptr = (uint8_t*)data;
    pb->sendlen = n;
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
        DEBUG_PRINTF("That's some over-achieving socket!\n");
        r = pb->sendlen;
    }
    pb->sendptr += r;
    pb->sendlen -= r;

    return (0 == pb->sendlen) ? 0 : +1;
}


int pbpal_start_read_line(pubnub_t *pb)
{
    if (pb->sock_state != STATE_NONE) {
        DEBUG_PRINTF("pbpal_start_read_line(): pb->sock_state != STATE_NONE: "); WATCH(pb->sock_state, "%d");
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
        int recvres = socket_recv(pb->pal.socket, (char*)pb->ptr, pb->left, 0);
        if (recvres < 0) {
            /* This is error or connection close, but, since it is an
               unexpected close, we treat it like an error.
             */
            if (PUBNUB_BLOCKING_IO_SETTABLE && pb->options.use_blocking_io) {
                return PNR_IO_ERROR;
            }
			return socket_would_block() ? PNR_IN_PROGRESS : PNR_IO_ERROR;
        }
        else if (0 == recvres) {
            return PNR_TIMEOUT;
        }
        DEBUG_PRINTF("have new data of length=%d: %s\n", recvres, pb->ptr);
        pb->sock_state = STATE_READ_LINE;
        pb->readlen = recvres;
    }

    while (pb->left > 0 && pb->readlen > 0) {
        c = *pb->ptr++;

        --pb->readlen;
        --pb->left;
        
        if (c == '\n') {
            DEBUG_PRINTF("\n found: "); WATCH(pbpal_read_len(pb), "%d"); WATCH(pb->readlen, "%d");
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
        DEBUG_PRINTF("pbpal_start_read(): pb->sock_state != STATE_NONE: "); WATCH(pb->sock_state, "%d");
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


bool pbpal_read_over(pubnub_t *pb)
{
    unsigned to_read = 0;
    WATCH(pb->sock_state, "%d");
    WATCH(pb->readlen, "%d");
    WATCH(pb->left, "%d");
    WATCH(pb->len, "%d");

    if (pb->readlen == 0) {
        int recvres;
        to_read =  pb->len - pbpal_read_len(pb);
        if (to_read > pb->left) {
            to_read = pb->left;
        }
        recvres = socket_recv(pb->pal.socket, (char*)pb->ptr, to_read, 0);
        if (recvres <= 0) {
            /* This is error or connection close, which may be handled
               in some way...
             */
            return false;
        }
        pb->sock_state = STATE_READ;
        pb->readlen = recvres;
    } 

    pb->ptr += pb->readlen;
    pb->left -= pb->readlen;
    pb->readlen = 0;

    if (pbpal_read_len(pb) >= (int)pb->len) {
        /* If we have read all that was requested, we're done. */
        DEBUG_PRINTF("Read all that was to be read.\n");
        pb->sock_state = STATE_NONE;
        return true;
    }

    if ((pb->left > 0)) {
        pb->sock_state = STATE_NEWDATA_EXHAUSTED;
        return false;
    }

    /* Otherwise, we just filled the buffer, but we return 'true', to
     * enable the user to copy the data from the buffer to some other
     * storage.
     */
    DEBUG_PRINTF("Filled the buffer, but read %d and should %d\n", pbpal_read_len(pb), pb->len);
    pb->sock_state = STATE_NONE;
    return true;
}


bool pbpal_closed(pubnub_t *pb)
{
    return pb->pal.socket == SOCKET_INVALID;
}


void pbpal_forget(pubnub_t *pb)
{
    /* a no-op under BSD-ish sockets */
	PUBNUB_UNUSED(pb);
}


void pbpal_close(pubnub_t *pb)
{
    DEBUG_PRINTF("pbpal_close()\n");
    pb->readlen = 0;
    if (pb->pal.socket != SOCKET_INVALID) {
        pbntf_lost_socket(pb, pb->pal.socket);
        socket_close(pb->pal.socket);
        pb->pal.socket = SOCKET_INVALID;
		pb->sock_state = STATE_NONE;
    }
}


bool pbpal_connected(pubnub_t *pb)
{
    return pb->pal.socket != SOCKET_INVALID;
}
