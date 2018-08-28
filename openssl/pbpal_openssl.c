/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"

#include "core/pbpal.h"

#include "pbpal_mutex.h"
#include "core/pubnub_ntf_sync.h"
#include "core/pubnub_netcore.h"
#include "core/pubnub_assert.h"
#include "core/pubnub_log.h"

#include <sys/types.h>
#include <fcntl.h>

#include <string.h>


#define HTTP_PORT 80


/** Locks used by OpenSSL */
static pbpal_mutex_t* m_locks;


static int print_to_pubnub_log(const char* s, size_t len, void* p)
{
    PUBNUB_UNUSED(len);

    PUBNUB_LOG_ERROR("From OpenSSL: pb=%p '%s'", p, s);

    return 0;
}


#if OPENSSL_API_COMPAT < 0x10100000L
static void locking_callback(int mode, int type, const char* file, int line)
{
    PUBNUB_LOG_TRACE("thread=%4lu mode=%s lock=%s %s:%d\n",
                     CRYPTO_thread_id(),
                     (mode & CRYPTO_LOCK) ? "l" : "u",
                     (type & CRYPTO_READ) ? "r" : "w",
                     file,
                     line);
    if (mode & CRYPTO_LOCK) {
        pbpal_mutex_lock(m_locks[type]);
    }
    else {
        pbpal_mutex_unlock(m_locks[type]);
    }
}
#endif


#if !defined(_WIN32) && (OPENSSL_API_COMPAT < 0x10000000L)
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
#if !defined(_WIN32) && (OPENSSL_API_COMPAT < 0x10000000L)
    // On Windows, OpenSSL has a suitable default
    CRYPTO_set_id_callback(thread_id);
#endif
#if OPENSSL_API_COMPAT < 0x10100000L
    CRYPTO_set_locking_callback(locking_callback);
#endif
    return 0;
}


static void buf_setup(pubnub_t* pb)
{
    pb->ptr  = (uint8_t*)pb->core.http_buf;
    pb->left = sizeof pb->core.http_buf / sizeof pb->core.http_buf[0];
}


static int pal_init(void)
{
    static bool s_init = false;
    if (!s_init) {
        ERR_load_BIO_strings();
        SSL_load_error_strings();
        SSL_library_init();
        OpenSSL_add_all_algorithms();
        PUBNUB_LOG_TRACE("SSLEAY_VERSION_NUMBER=%lx SSLeay()=%lx "
                         "SSLeay_version(SSLEAY_VERSION)='%s'\n",
                         SSLEAY_VERSION_NUMBER,
                         SSLeay(),
                         SSLeay_version(SSLEAY_VERSION));
        if (locks_setup()) {
            return -1;
        }
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
    memset(&pb->pal, 0, sizeof pb->pal);
    pb->pal.socket = SOCKET_INVALID;
    pb->options.useSSL = pb->flags.trySSL = pb->options.fallbackSSL = true;
    pb->options.use_system_certificate_store = false;
    pb->options.reuse_SSL_session            = false;
    pb->ssl_CAfile = pb->ssl_CApath = NULL;
    pb->ssl_userPEMcert             = NULL;
    pb->sock_state                  = STATE_NONE;
    buf_setup(pb);
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

static void report_error_from_environment(pubnub_t* pb)
{
    char const* err_str;

#if HAVE_STRERROR_R
    char errstr_r[1024];
    strerror_r(errno, errstr_r, sizeof errstr_r / sizeof errstr_r[0]);
    err_str = errstr_r;
#elif HAVE_STRERROR_S
    char errstr_s[1024];
    strerror_s(errstr_s, sizeof errstr_s / sizeof errstr_s[0], errno);
    err_str = errstr_s;
#else
    err_str = strerror(errno);
#endif
    PUBNUB_LOG_DEBUG(
        "report_error_from_environment(pb=%p): errno=%d('%s') use_blocking_io=%d\n",
        pb,
        errno,
        err_str,
        (int)pb->options.use_blocking_io);
#if defined(_WIN32)
    PUBNUB_LOG_DEBUG("report_error_from_environment(pb=%p): GetLastErrror()=%lu "
                     "WSAGetLastError()=%d\n",
                     pb,
                     GetLastError(),
                     WSAGetLastError());
#endif
}

enum pubnub_res pbpal_handle_socket_condition(int result, pubnub_t* pb)
{
    SSL* ssl = pb->pal.ssl;

    if (NULL == ssl) {
        PUBNUB_ASSERT_INT_OPT(result, <=, 0);
        if (result < 0) {
            if (socket_would_block()) {
                if (PUBNUB_BLOCKING_IO_SETTABLE && pb->options.use_blocking_io) {
                    pb->sock_state = STATE_NONE;
                    return PNR_TIMEOUT;
                }
                return PNR_IN_PROGRESS;
            }
            else {
                pb->sock_state = STATE_NONE;
                report_error_from_environment(pb);
                return socket_timed_out() ? PNR_CONNECTION_TIMEOUT : PNR_IO_ERROR;
            }
        }
        else if (0 == result) {
            pb->sock_state = STATE_NONE;
            return PNR_TIMEOUT;
        }
        pb->sock_state = STATE_NONE;
        return PNR_INTERNAL_ERROR;
    }
    else {
        PUBNUB_ASSERT(pb->options.useSSL);
        switch(SSL_get_error(ssl, result)) {
        case SSL_ERROR_NONE:
            break;
        case SSL_ERROR_WANT_READ:
        case SSL_ERROR_WANT_WRITE:
        case SSL_ERROR_WANT_CONNECT:
        case SSL_ERROR_WANT_X509_LOOKUP:
            if (PUBNUB_TIMERS_API && (pb->pal.connect_timeout > time(NULL))) {
                PUBNUB_LOG_TRACE("pb=%p: TLS/SSL_I/O operation should retry\n", pb);

                return PNR_IN_PROGRESS;
            }
            else {
                /* Expire the IP for the next connect */
                pb->pal.ip_timeout = 0;
                if ((pb->pal.session != NULL) && pb->options.reuse_SSL_session) {
                   SSL_SESSION_free(pb->pal.session);
                   pb->pal.session = NULL;
                }
                PUBNUB_LOG_ERROR("pb=%p: TLS/SSL_I/O operation failed, PNR_TIMEOUT\n", pb);
                pb->sock_state = STATE_NONE;

                return PNR_TIMEOUT;
            }
        case SSL_ERROR_SYSCALL:
        case SSL_ERROR_SSL:
        case SSL_ERROR_ZERO_RETURN:
        default:
            /* Expire the IP for the next connect */
            pb->pal.ip_timeout = 0;
            ERR_print_errors_cb(print_to_pubnub_log, pb);
            if ((pb->pal.session != NULL) && pb->options.reuse_SSL_session) {
                SSL_SESSION_free(pb->pal.session);
                pb->pal.session = NULL;
            }
            PUBNUB_LOG_ERROR("pb=%p: TLS/SSL_I/O operation failed, errno=%d\n", pb, errno);
            pb->sock_state = STATE_NONE;

            return PNR_IO_ERROR;
        }
        PUBNUB_LOG_TRACE("pb=%p: TLS/SSL_I/O operation succeeds.\n", pb);

        return  PNR_OK;
    }
}


int pbpal_send_status(pubnub_t* pb)
{
    int rslt;
    SSL* ssl = pb->pal.ssl;

    if (0 == pb->len) {
        return 0;
    }
    PUBNUB_ASSERT_OPT(pb->sock_state == STATE_SENDING_DATA);

    if (NULL == ssl) {
//
        printf("%*s", pb->len, (char*)pb->ptr);
//
        rslt = socket_send(pb->pal.socket, (char*)pb->ptr, pb->len);
    }
    else { 
        rslt = SSL_write(ssl, pb->ptr, pb->len);
    }
    if (rslt <= 0) {
        rslt = (pbpal_handle_socket_condition(rslt, pb) == PNR_IN_PROGRESS) ? +1 : -1;
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
    SSL* ssl = pb->pal.ssl;

    PUBNUB_ASSERT_OPT(STATE_READ_LINE == pb->sock_state);

    /* OpenSSL reads one TLS record at a time,
       so, we need to call it in a loop to read äll there is
    */
    for (;;) {
        if (pb->unreadlen == 0) {
            int recvres;
            PUBNUB_ASSERT_OPT((char*)pb->ptr + pb->left
                              == pb->core.http_buf + PUBNUB_BUF_MAXLEN);
            if (NULL == ssl) {
                recvres = socket_recv(pb->pal.socket, (char*)pb->ptr, pb->left, 0);
            }
            else {
                recvres = SSL_read(ssl, (char*)pb->ptr, pb->left);
            }
            if (recvres <= 0) {
                return pbpal_handle_socket_condition(recvres, pb);
            }

            PUBNUB_ASSERT_OPT(recvres <= pb->left);
            PUBNUB_LOG_TRACE("pb=%p have new data of length=%d: %.*s\n",
                             pb,
                             recvres,
                             recvres,
                             pb->ptr);
            pb->unreadlen = recvres;
            pb->left -= recvres;
        }

        while (pb->unreadlen > 0) {
            uint8_t c;

            --pb->unreadlen;

            c = *pb->ptr++;
            if (c == '\n') {
                WATCH_USHORT(pb->unreadlen);
                pb->sock_state = STATE_NONE;
                return PNR_OK;
            }
        }

        if (pb->left == 0) {
            PUBNUB_LOG_ERROR("pbpal_line_read_status(pb=%p): buffer full but "
                             "newline not found",
                             pb);
            pb->sock_state = STATE_NONE;
            return PNR_TX_BUFF_TOO_SMALL;
        }
        if (NULL == ssl) {
            break;
        }
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
    SSL* ssl = pb->pal.ssl;

    PUBNUB_ASSERT_OPT(STATE_READ == pb->sock_state);

    /* OpenSSL reads one TLS record at a time,
       so, we need to call it in a loop to read äll there is
    */
    for(;;) {
        if (0 == pb->unreadlen) {
            unsigned to_recv = pb->len;
            if (to_recv > pb->left) {
                to_recv = pb->left;
            }
            PUBNUB_ASSERT_OPT(to_recv > 0);
            if (NULL == ssl) {
                have_read = socket_recv(pb->pal.socket, (char*)pb->ptr, to_recv, 0);
            }
            else {
                have_read = SSL_read(ssl, pb->ptr, to_recv);
            }
            if (have_read <= 0) {
                return pbpal_handle_socket_condition(have_read, pb);
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
        if (NULL == ssl) {
            break;
        }
    }

    return PNR_IN_PROGRESS;
}


bool pbpal_closed(pubnub_t* pb)
{
    return (pb->pal.ssl == NULL) && (pb->pal.socket == SOCKET_INVALID);
}


void pbpal_forget(pubnub_t* pb)
{
    /* a no-op under OpenSSL */
}


int pbpal_close(pubnub_t* pb)
{
    pb->unreadlen = 0;
    if (pb->pal.ssl != NULL) {
        SSL_shutdown(pb->pal.ssl);
        SSL_free(pb->pal.ssl);
        pb->pal.ssl = NULL;
    }
    if(pb->pal.socket != SOCKET_INVALID) {
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
    /* While this should not happen, it doesn't hurt to 'catch' it, if it happens..
     */
    if (pb->pal.ssl != NULL) {
        PUBNUB_LOG_TRACE("pbpal_free(%p): Unexpected pb->pal.ssl == %p\n",
                         pb,
                         pb->pal.ssl);
        SSL_shutdown(pb->pal.ssl);
        SSL_free(pb->pal.ssl);
        pb->pal.ssl = NULL;
    }
    if (pb->pal.socket != SOCKET_INVALID) {
        pbntf_lost_socket(pb);
        PUBNUB_LOG_TRACE("pbpal_free(%p): Unexpected pb->pal.socket == %d\n",
                         pb,
                         pb->pal.socket);
        socket_close(pb->pal.socket);
        pb->pal.socket = SOCKET_INVALID;
        pb->sock_state = STATE_NONE;
    }
    /* The rest, OTOH, is expected */
    if (pb->pal.ctx != NULL) {
        SSL_CTX_free(pb->pal.ctx);
        if (NULL != pb->pal.session) {
            SSL_SESSION_free(pb->pal.session);
            pb->pal.session = NULL;
        }
    }
    else {
        PUBNUB_ASSERT_OPT(NULL == pb->pal.session);
    }
}
