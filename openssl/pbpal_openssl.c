/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pbpal.h"

#include "pubnub_ntf_sync.h"
#include "pubnub_netcore.h"
#include "pubnub_internal.h"
#include "pubnub_assert.h"
#include "pubnub_log.h"

#include "openssl/ssl.h"


#include <string.h>


#define HTTP_PORT 80


static void buf_setup(pubnub_t *pb)
{
    pb->ptr = (uint8_t*)pb->core.http_buf;
    pb->left = sizeof pb->core.http_buf;
}


static int pal_init(void)
{
    static bool s_init = false;
    if (!s_init) {
        ERR_load_BIO_strings();
        SSL_load_error_strings();
        SSL_library_init();
        OpenSSL_add_all_algorithms();

        pbntf_init();
        s_init = true;
    }
    return 0;
}


void pbpal_init(pubnub_t *pb)
{
    pal_init();
    pb->pal.socket = NULL;
    pb->pal.ctx = NULL;
    pb->options.use_blocking_io = true;
    pb->options.useSSL = pb->options.fallbackSSL = pb->options.ignoreSSL = true;
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
    r = BIO_write(pb->pal.socket, pb->sendptr, pb->sendlen);
    if (r < 0) {
        if (BIO_should_retry(pb->pal.socket)) {
            return +1;
        }
        ERR_print_errors_fp(stderr);
        return -1;
    }
    if (r > pb->sendlen) {
        PUBNUB_LOG_WARNING("That's some over-achieving BIO!\n");
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
        int recvres = BIO_read(pb->pal.socket, pb->ptr, pb->left);
        if (recvres < 0) {
            /* This is error or connection close, but, since it is an
               unexpected close, we treat it like an error.
             */
            if (PUBNUB_BLOCKING_IO_SETTABLE && pb->options.use_blocking_io) {
                return PNR_IO_ERROR;
            }
            if (BIO_should_retry(pb->pal.socket)) {
                return PNR_IN_PROGRESS;
            }
            ERR_print_errors_fp(stderr);
            return PNR_IO_ERROR;
        }
        else if (0 == recvres) {
            return PNR_TIMEOUT;
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


bool pbpal_read_over(pubnub_t *pb)
{
    unsigned to_read = 0;
    WATCH_ENUM(pb->sock_state);
    WATCH_USHORT(pb->readlen);
    WATCH_USHORT(pb->left);
    WATCH_UINT(pb->len);

    if (pb->readlen == 0) {
        int recvres;
        to_read =  pb->len - pbpal_read_len(pb);
        if (to_read > pb->left) {
            to_read = pb->left;
        }
        recvres = BIO_read(pb->pal.socket, pb->ptr, to_read);
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

    if (pbpal_read_len(pb) >= pb->len) {
        /* If we have read all that was requested, we're done. */
        PUBNUB_LOG_TRACE("Read all that was to be read.\n");
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
    PUBNUB_LOG_WARNING("Filled the buffer, but read %d and should %d\n", pbpal_read_len(pb), pb->len);
    pb->sock_state = STATE_NONE;
    return true;
}


bool pbpal_closed(pubnub_t *pb)
{
    return pb->pal.socket == NULL;
}


void pbpal_forget(pubnub_t *pb)
{
    /* a no-op under OpenSSL */
}


int pbpal_close(pubnub_t *pb)
{
    pb->readlen = 0;
    pb->sock_state = STATE_NONE;
    if (pb->pal.socket != NULL) {
        pbntf_lost_socket(pb, pb->pal.socket);
        BIO_free_all(pb->pal.socket);
        pb->pal.socket = NULL;
        if (pb->pal.ctx != NULL) {
            SSL_CTX_free(pb->pal.ctx);
            pb->pal.ctx = NULL;
        }
    }

    PUBNUB_LOG_TRACE("pbpal_close() returning 0\n");

    return 0;
}


bool pbpal_connected(pubnub_t *pb)
{
    return pb->pal.socket != NULL;
}
