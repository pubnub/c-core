/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pbpal.h"

#include "pbpal_mutex.h"
#include "pubnub_ntf_sync.h"
#include "pubnub_netcore.h"
#include "pubnub_internal.h"
#include "pubnub_assert.h"
#include "pubnub_log.h"

#include "openssl/ssl.h"


#include <string.h>


#define HTTP_PORT 80


/** Locks used by OpenSSL */
static pbpal_mutex_t *m_locks;


static void locking_callback(int mode, int type, const char *file, int line)
{
    PUBNUB_LOG_TRACE("thread=%4lu mode=%s lock=%s %s:%d\n", CRYPTO_thread_id(),
                     (mode & CRYPTO_LOCK) ? "l" : "u",
                     (type & CRYPTO_READ) ? "r" : "w", file, line);
    if (mode & CRYPTO_LOCK) {
        pbpal_mutex_lock(m_locks[type]);
    }
    else {
        pbpal_mutex_unlock(m_locks[type]);
    }
}


#if !defined(_WIN32)
static unsigned long thread_id(void)
{
    return (unsigned long)pbpal_thread_id();
}
#endif


static int locks_setup(void)
{
    int i;
    m_locks = (pbpal_mutex_t*)calloc(CRYPTO_num_locks(), sizeof(pbpal_mutex_t));
    if (NULL == m_locks) {
        return -1;
    }
    for (i = 0; i < CRYPTO_num_locks(); ++i) {
        pbpal_mutex_init_std(m_locks[i]);
    }
#if !defined(_WIN32)
    // On Windows, OpenSSL has a suitable default
    CRYPTO_set_id_callback(thread_id);
#endif
    CRYPTO_set_locking_callback(locking_callback);
    return 0;
}


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
        if (locks_setup()) {
            return -1;
        }

        pbntf_init();
        s_init = true;
    }
    return 0;
}


void pbpal_init(pubnub_t *pb)
{
    pal_init();
    memset(&pb->pal, 0, sizeof pb->pal);
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
        size_t distance = pb->ptr - (uint8_t*)pb->core.http_buf;
        memmove(pb->core.http_buf, pb->ptr, pb->readlen);
        pb->ptr -= distance;
        pb->left += (uint16_t)distance;
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
        size_t distance = pb->ptr - (uint8_t*)pb->core.http_buf;
        memmove(pb->core.http_buf, pb->ptr, pb->readlen);
        pb->ptr -= distance;
        pb->left += (uint16_t)distance;
    }
    else {
        if (pb->left == 0) {
            /* Obviously, our buffer is not big enough, maybe some
               error should be reported */
            buf_setup(pb);
        }
    }
    pb->sock_state = STATE_READ;
    pb->len = (unsigned)n;
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
        return true;
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
        return false;
    }

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
    }

    PUBNUB_LOG_TRACE("pbpal_close() returning 0\n");

    return 0;
}


void pbpal_free(pubnub_t *pb)
{
    if (pb->pal.socket != NULL) {
        /* While this should not happen, it doesn't hurt to be paranoid.
         */
        pbntf_lost_socket(pb, pb->pal.socket);
        BIO_free_all(pb->pal.socket);
    }

    /* The rest, OTOH, is expected */
    if (pb->pal.ctx != NULL) {
        SSL_CTX_free(pb->pal.ctx);
        if (pb->pal.session != NULL) {
            SSL_SESSION_free(pb->pal.session);
        }
    }
    else {
        PUBNUB_ASSERT_OPT(NULL == pb->pal.session);
    }
}
