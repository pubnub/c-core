/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"

#include "core/pbpal.h"

#include "pbpal_mutex.h"
#include "core/pubnub_ntf_sync.h"
#include "core/pubnub_netcore.h"
#include "core/pubnub_assert.h"
#if PUBNUB_USE_LOGGER
#include "core/pbcc_logger_manager.h"
#endif // PUBNUB_USE_LOGGER

#include "lib/msstopwatch/msstopwatch.h"

#include <sys/types.h>
#include <fcntl.h>

#include <string.h>

#include <openssl/opensslv.h>

#define HTTP_PORT 80


PUBNUB_STATIC_ASSERT(PUBNUB_TIMERS_API, need_TIMERS_API);


static int print_to_pubnub_log(const char* s, size_t len, void* p)
{
    PUBNUB_UNUSED(len);

    PUBNUB_LOG_ERROR((pubnub_t*)p, "%s", s);

    return 0;
}

static void buf_setup(pubnub_t* pb)
{
    pb->ptr  = (uint8_t*)pb->core.http_buf;
    pb->left = sizeof pb->core.http_buf / sizeof pb->core.http_buf[0];
}


static int pal_init(pubnub_t* pb)
{
#if !defined(PUBNUB_NTF_RUNTIME_SELECTION)
    static bool s_init = false;
    if (!s_init) {
#else
    bool*       s_init          = NULL;
    static bool s_init_sync     = false;
    static bool s_init_callback = false;

    switch (pb->api_policy) {
    case PNA_SYNC:
        s_init = &s_init_sync;
        break;
    case PNA_CALLBACK:
        s_init = &s_init_callback;
        break;
    }

    if (!*s_init) {
#endif
#if !defined(__UWP__) && (OPENSSL_VERSION_MAJOR < 3)
        ERR_load_BIO_strings(); // Per OpenSSL 3.0 this is deprecated. Allowing
                                // this stmt for non-UWP as it exists.
#endif

        if (0 != socket_platform_init()) { return -1; }
        pbntf_init(pb);
#if !defined(PUBNUB_NTF_RUNTIME_SELECTION)
        s_init = true;
#else
        *s_init = true;
#endif
    }
    return 0;
}


void pbpal_init(pubnub_t* pb)
{
    pal_init(pb);
    memset(&pb->pal, 0, sizeof pb->pal);
    pb->pal.socket     = SOCKET_INVALID;
    pb->options.useSSL = pb->flags.trySSL = pb->options.fallbackSSL = true;
    pb->options.use_system_certificate_store                        = false;
    pb->options.reuse_SSL_session                                   = false;
    pb->ssl_CAfile = pb->ssl_CApath = NULL;
    pb->ssl_userPEMcert             = NULL;
    pb->sock_state                  = STATE_NONE;
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


enum pubnub_res pbpal_handle_socket_condition(
    int         result,
    pubnub_t*   pb,
    char const* file,
    int         line,
    bool*       needRead,
    bool*       needWrite)
{
    SSL* ssl = pb->pal.ssl;

    if (NULL == ssl) {
        return pbpal_handle_socket_error(result, pb, file, line);
    }
    else {
        PUBNUB_ASSERT(pb->options.useSSL);
        const int error = SSL_get_error(ssl, result);

        switch (error) {
        case SSL_ERROR_NONE:
            break;
        case SSL_ERROR_WANT_READ:
        case SSL_ERROR_WANT_WRITE:
        case SSL_ERROR_WANT_CONNECT:
        case SSL_ERROR_WANT_X509_LOOKUP:
            if (!pbms_active(pb->pal.tryconn) ||
                (pbms_elapsed(pb->pal.tryconn) < pb->transaction_timeout_ms)) {
                /* TLS handshake path: needRead/needWrite provided by
                   pbpal_check_tls(). Log only on first occurrence or
                   when the error type changes. Data I/O path: needRead/
                   needWrite are NULL. WANT_READ/WRITE is normal for
                   non-blocking SSL — suppress the generic retry trace
                   entirely (the caller knows the result via return
                   value). */
                const bool tls_handshake =
                    (needRead != NULL || needWrite != NULL);
                if (tls_handshake) {
                    const bool error_changed =
                        (error != pb->pal.tls_connect_last_error);
                    pb->pal.tls_connect_last_error = error;
                    if (needWrite && SSL_ERROR_WANT_WRITE == error) {
                        if (error_changed) {
                            PUBNUB_LOG_TRACE(
                                pb,
                                "TLS/SSL_I/O operation error (retriable):"
                                " want write");
                        }
                        *needWrite = true;
                    }
                    if (needRead && SSL_ERROR_WANT_READ == error) {
                        if (error_changed) {
                            PUBNUB_LOG_TRACE(
                                pb,
                                "TLS/SSL_I/O operation error (retriable):"
                                " want read");
                        }
                        *needRead = true;
                    }
                    if (error_changed) {
                        PUBNUB_LOG_TRACE(
                            pb,
                            "TLS/SSL I/O retry (ssl_error=%d)",
                            error);
                    }
                }
                return PNR_IN_PROGRESS;
            }

            /* Expire the IP for the next connect */
            pb->pal.ip_timeout = 0;
            if ((pb->pal.session != NULL) && pb->options.reuse_SSL_session) {
                SSL_SESSION_free(pb->pal.session);
                pb->pal.session = NULL;
            }
            PUBNUB_LOG_ERROR(pb, "TLS/SSL_I/O operation error: timeout");
            pb->sock_state = STATE_NONE;

            return PNR_TIMEOUT;
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
            PUBNUB_LOG_ERROR(
                pb, "TLS/SSL I/O operation failed with error code: %d", errno);
            pb->sock_state = STATE_NONE;

            return PNR_IO_ERROR;
        }
        PUBNUB_LOG_TRACE(pb, "TLS/SSL I/O operation succeeds.");

        return PNR_OK;
    }
}


int pbpal_send_status(pubnub_t* pb)
{
    int  rslt;
    SSL* ssl = pb->pal.ssl;

    if (0 == pb->len) { return 0; }

    PUBNUB_ASSERT_OPT(pb->sock_state == STATE_SENDING_DATA);

    if (NULL == ssl) {
        rslt = socket_send(pb->pal.socket, (char*)pb->ptr, pb->len);
    }
    else {
        rslt = SSL_write(ssl, pb->ptr, pb->len);
    }

    if (rslt <= 0) {
        rslt =
            (pbpal_handle_socket_condition(
                 rslt, pb, __FILE__, __LINE__, NULL, NULL) == PNR_IN_PROGRESS)
                ? +1
                : -1;
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
        PUBNUB_ASSERT_OPT(
            (char*)pb->ptr + pb->unreadlen <=
            pb->core.http_buf + PUBNUB_BUF_MAXLEN);
        memmove(pb->core.http_buf, pb->ptr, pb->unreadlen);
    }
    distance = pb->ptr - (uint8_t*)pb->core.http_buf;
    PUBNUB_ASSERT_UINT(
        distance + pb->left + pb->unreadlen,
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
       so, we need to call it in a loop to read �ll there is
    */
    for (;;) {
        if (pb->unreadlen == 0) {
            int recvres;
            PUBNUB_ASSERT_OPT(
                (char*)pb->ptr + pb->left ==
                pb->core.http_buf + PUBNUB_BUF_MAXLEN);
            if (NULL == ssl) {
                recvres =
                    socket_recv(pb->pal.socket, (char*)pb->ptr, pb->left, 0);
            }
            else {
                recvres = SSL_read(ssl, (char*)pb->ptr, pb->left);
            }
            if (recvres <= 0) {
                return pbpal_handle_socket_condition(
                    recvres, pb, __FILE__, __LINE__, NULL, NULL);
            }

            PUBNUB_ASSERT_OPT(recvres <= pb->left);
            PUBNUB_LOG_TRACE(pb, "%d bytes received.", recvres);
            pb->unreadlen = recvres;
            pb->left -= recvres;
        }

        while (pb->unreadlen > 0) {
            uint8_t c;

            --pb->unreadlen;

            c = *pb->ptr++;
            if (c == '\n') {
                pb->sock_state = STATE_NONE;
                return PNR_OK;
            }
        }

        if (pb->left == 0) {
            PUBNUB_LOG_ERROR(pb, "Buffer is full but newline not found.");
            pb->sock_state = STATE_NONE;
            return PNR_TX_BUFF_TOO_SMALL;
        }
        if (NULL == ssl) { break; }
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

    if (pb->unreadlen > 0) {
        PUBNUB_ASSERT_OPT(
            (char*)pb->ptr + pb->unreadlen <=
            pb->core.http_buf + PUBNUB_BUF_MAXLEN);
        memmove(pb->core.http_buf, pb->ptr, pb->unreadlen);
    }
    distance = pb->ptr - (uint8_t*)pb->core.http_buf;
    PUBNUB_ASSERT_UINT(
        distance + pb->unreadlen + pb->left,
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
    int  have_read;
    SSL* ssl = pb->pal.ssl;

    PUBNUB_ASSERT_OPT(STATE_READ == pb->sock_state);

    /* OpenSSL reads one TLS record at a time,
       so, we need to call it in a loop to read �ll there is
    */
    for (;;) {
        if (0 == pb->unreadlen) {
            unsigned to_recv = pb->len;
            if (to_recv > pb->left) { to_recv = pb->left; }
            PUBNUB_ASSERT_OPT(to_recv > 0);
            if (NULL == ssl) {
                have_read =
                    socket_recv(pb->pal.socket, (char*)pb->ptr, to_recv, 0);
            }
            else {
                have_read = SSL_read(ssl, pb->ptr, to_recv);
            }
            if (have_read <= 0) {
                return pbpal_handle_socket_condition(
                    have_read, pb, __FILE__, __LINE__, NULL, NULL);
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
        if (NULL == ssl) { break; }
    }

    PUBNUB_LOG_TRACE(pb, "SSL read incomplete, will retry.");
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
    if (pb->pal.socket != SOCKET_INVALID) {
        pbntf_lost_socket(pb);
        socket_close(pb->pal.socket);
        pb->pal.socket = SOCKET_INVALID;
        pb->sock_state = STATE_NONE;
    }
    PUBNUB_LOG_TRACE(pb, "Close TLS/SSL connection and socket.");

    return 0;
}


void pbpal_free(pubnub_t* pb)
{
    /* While this should not happen, it doesn't hurt to 'catch' it, if it
     * happens..
     */
    if (pb->pal.ssl != NULL) {
        PUBNUB_LOG_TRACE(
            pb,
            "Unexpectedly TLS/SSL connection still exists. Shutting down...");
        SSL_shutdown(pb->pal.ssl);
        SSL_free(pb->pal.ssl);
        pb->pal.ssl = NULL;
    }
    if (pb->pal.socket != SOCKET_INVALID) {
        pbntf_lost_socket(pb);
        PUBNUB_LOG_TRACE(pb, "Unexpectedly socket still exists. Closing...");
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
