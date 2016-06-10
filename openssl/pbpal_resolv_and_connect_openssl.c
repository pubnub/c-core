/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pbpal.h"

#ifdef _WIN32
#include <winsock2.h>
#else
#include <sys/select.h>
#define SOCKET_ERROR -1
#endif

#include "pubnub_internal.h"
#include "pubnub_assert.h"
#include "pubnub_log.h"

#include <openssl/pem.h>
#include <openssl/ssl.h>


static int print_to_pubnub_log(const char *s, size_t len, void *p)
{
    PUBNUB_UNUSED(len);
    PUBNUB_UNUSED(p);

    PUBNUB_LOG_ERROR("%s", s);

    return 0;
}


static enum pbpal_resolv_n_connect_result resolv_and_connect_wout_SSL(pubnub_t *pb)
{
    PUBNUB_LOG_TRACE("resolv_and_connect_wout_SSL\n");
    if (NULL == pb->pal.socket) {
        char const*origin = PUBNUB_ORIGIN_SETTABLE ? pb->origin : PUBNUB_ORIGIN;
        PUBNUB_LOG_TRACE("pb=%p: Don't have BIO\n", pb);
        pb->pal.socket = BIO_new_connect((char*)origin);
    }
    if (NULL == pb->pal.socket) {
        return pbpal_resolv_resource_failure;
    }
    BIO_set_conn_port(pb->pal.socket, "http");

    BIO_set_nbio(pb->pal.socket, !pb->options.use_blocking_io);

    WATCH_ENUM(pb->options.use_blocking_io);
    if (BIO_do_connect(pb->pal.socket) <= 0) {
        if (BIO_should_retry(pb->pal.socket)) {
            return pbpal_connect_wouldblock;
        }
        ERR_print_errors_cb(print_to_pubnub_log, NULL);
        PUBNUB_LOG_ERROR("BIO_do_connect failed\n");
        return pbpal_connect_failed;
    }

    PUBNUB_LOG_TRACE("pb=%p: BIO connected\n", pb);
    {
        int fd = BIO_get_fd(pb->pal.socket, NULL);
        socket_set_rcv_timeout(fd, pb->transaction_timeout_ms);
    }

    return pbpal_connect_success;
}


/* Starfields Inc (Class 2) Root certificate.
   It was used to sign the server certificate of https://pubsub.pubnub.com.
   (at the time of writing this, 2015-06-04).
 */
static char pubnub_cert[] = "-----BEGIN CERTIFICATE-----\n"
"MIIEDzCCAvegAwIBAgIBADANBgkqhkiG9w0BAQUFADBoMQswCQYDVQQGEwJVUzEl\n"
"MCMGA1UEChMcU3RhcmZpZWxkIFRlY2hub2xvZ2llcywgSW5jLjEyMDAGA1UECxMp\n"
"U3RhcmZpZWxkIENsYXNzIDIgQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkwHhcNMDQw\n"
"NjI5MTczOTE2WhcNMzQwNjI5MTczOTE2WjBoMQswCQYDVQQGEwJVUzElMCMGA1UE\n"
"ChMcU3RhcmZpZWxkIFRlY2hub2xvZ2llcywgSW5jLjEyMDAGA1UECxMpU3RhcmZp\n"
"ZWxkIENsYXNzIDIgQ2VydGlmaWNhdGlvbiBBdXRob3JpdHkwggEgMA0GCSqGSIb3\n"
"DQEBAQUAA4IBDQAwggEIAoIBAQC3Msj+6XGmBIWtDBFk385N78gDGIc/oav7PKaf\n"
"8MOh2tTYbitTkPskpD6E8J7oX+zlJ0T1KKY/e97gKvDIr1MvnsoFAZMej2YcOadN\n"
"+lq2cwQlZut3f+dZxkqZJRRU6ybH838Z1TBwj6+wRir/resp7defqgSHo9T5iaU0\n"
"X9tDkYI22WY8sbi5gv2cOj4QyDvvBmVmepsZGD3/cVE8MC5fvj13c7JdBmzDI1aa\n"
"K4UmkhynArPkPw2vCHmCuDY96pzTNbO8acr1zJ3o/WSNF4Azbl5KXZnJHoe0nRrA\n"
"1W4TNSNe35tfPe/W93bC6j67eA0cQmdrBNj41tpvi/JEoAGrAgEDo4HFMIHCMB0G\n"
"A1UdDgQWBBS/X7fRzt0fhvRbVazc1xDCDqmI5zCBkgYDVR0jBIGKMIGHgBS/X7fR\n"
"zt0fhvRbVazc1xDCDqmI56FspGowaDELMAkGA1UEBhMCVVMxJTAjBgNVBAoTHFN0\n"
"YXJmaWVsZCBUZWNobm9sb2dpZXMsIEluYy4xMjAwBgNVBAsTKVN0YXJmaWVsZCBD\n"
"bGFzcyAyIENlcnRpZmljYXRpb24gQXV0aG9yaXR5ggEAMAwGA1UdEwQFMAMBAf8w\n"
"DQYJKoZIhvcNAQEFBQADggEBAAWdP4id0ckaVaGsafPzWdqbAYcaT1epoXkJKtv3\n"
"L7IezMdeatiDh6GX70k1PncGQVhiv45YuApnP+yz3SFmH8lU+nLMPUxA2IGvd56D\n"
"eruix/U0F47ZEUD0/CwqTRV/p2JdLiXTAAsgGh1o+Re49L2L7ShZ3U0WixeDyLJl\n"
"xy16paq8U4Zt3VekyvggQQto8PT7dL5WXXp59fkdheMtlb71cZBDzI0fmgAKhynp\n"
"VSJYACPq4xJDKVtHCN2MQWplBqjlIapBtJUhlbl90TSrE9atvNziPTnNvT51cKEY\n"
"WQPJIrSPnNVeKtelttQKbfi3QBFGmh95DmK/D5fs4C8fF5Q=\n"
    "-----END CERTIFICATE-----\n";


static int add_pubnub_cert(SSL_CTX *sslCtx)
{
    X509 *cert;
    BIO *mem;

    mem = BIO_new(BIO_s_mem());
    BIO_puts(mem, pubnub_cert);
    cert = PEM_read_bio_X509(mem, NULL, 0, NULL);
    BIO_free(mem);

    // set certificate to sslCtx
    X509_STORE_add_cert(SSL_CTX_get_cert_store(sslCtx), cert);
    X509_free(cert);

    /* In principle, it would be nice to use this instead, if we
       had a way to find out what is the file and/or path
       of the trusted root certificates. It would fix a problem
       that would happen if the root certificate used on
       `https://pubsub.pubnub.com` changes...

       One way to "fix" this would be to add some #defines
       for these, though it's not a user-friendly solution, so
       we're not doing it for now.

//    SSL_CTX_load_verify_locations(sslCtx, NULL, "/etc/ssl/certs");
    */

    return 0;
}


enum pbpal_resolv_n_connect_result pbpal_resolv_and_connect(pubnub_t *pb)
{
    SSL *ssl;
    int rslt;
    char const* origin = PUBNUB_ORIGIN_SETTABLE ? pb->origin : PUBNUB_ORIGIN;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    PUBNUB_ASSERT_OPT((pb->state == PBS_READY) || (pb->state == PBS_WAIT_CONNECT));

    if (!pb->options.useSSL) {
        return resolv_and_connect_wout_SSL(pb);
    }

    if (NULL == pb->pal.ctx) {
        PUBNUB_LOG_TRACE("pb=%p: Don't have SSL_CTX\n", pb);
        pb->pal.ctx = SSL_CTX_new(SSLv23_client_method());
        if (NULL == pb->pal.ctx) {
            ERR_print_errors_cb(print_to_pubnub_log, NULL);
            PUBNUB_LOG_ERROR("pb=%p SSL_CTX_new failed\n", pb);
            return pbpal_resolv_resource_failure;
        }
        PUBNUB_LOG_TRACE("pb=%p: Got SSL_CTX\n", pb);
        add_pubnub_cert(pb->pal.ctx);
    }

    if (NULL == pb->pal.socket) {
        PUBNUB_LOG_TRACE("pb=%p: Don't have BIO\n", pb);
        pb->pal.socket = BIO_new_ssl_connect(pb->pal.ctx);
        if (PUBNUB_TIMERS_API) {
            pb->pal.connect_timeout = time(NULL)  + pb->transaction_timeout_ms/1000;
        }
    }
    if (NULL == pb->pal.socket) {
        ERR_print_errors_cb(print_to_pubnub_log, NULL);
        return pbpal_resolv_resource_failure;
    }

    PUBNUB_LOG_TRACE("pb=%p: Using BIO == %p\n", pb, pb->pal.socket);

    BIO_get_ssl(pb->pal.socket, &ssl);
    SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY); /* maybe not auto_retry? */
    if (pb->pal.session != NULL) {
        SSL_set_session(ssl, pb->pal.session);
    }

    BIO_set_conn_hostname(pb->pal.socket, origin);
    BIO_set_conn_port(pb->pal.socket, "https");
    if (pb->pal.ip_timeout != 0) {
        if (pb->pal.ip_timeout < time(NULL)) {
            pb->pal.ip_timeout = 0;
        }
        else {
            PUBNUB_LOG_TRACE("SSL re-connect to: %d.%d.%d.%d\n", pb->pal.ip[0], pb->pal.ip[1], pb->pal.ip[2], pb->pal.ip[3]);
            BIO_set_conn_ip(pb->pal.socket, pb->pal.ip);
        }
    }
    
    BIO_set_nbio(pb->pal.socket, !pb->options.use_blocking_io);
    
    WATCH_ENUM(pb->options.use_blocking_io);
    if (BIO_do_connect(pb->pal.socket) <= 0) {
        if (BIO_should_retry(pb->pal.socket) && PUBNUB_TIMERS_API && (pb->pal.connect_timeout > time(NULL))) {
            PUBNUB_LOG_TRACE("pb=%p: BIO_should_retry\n", pb);
            return pbpal_connect_wouldblock;
        }
        /* Expire the IP for the next connect */
        pb->pal.ip_timeout = 0;
        ERR_print_errors_cb(print_to_pubnub_log, NULL);
        if (pb->pal.session != NULL) {
            SSL_SESSION_free(pb->pal.session);
            pb->pal.session = NULL;
        }
        PUBNUB_LOG_ERROR("pb=%p: BIO_do_connect failed\n", pb);
        return pbpal_connect_failed;
    }

    PUBNUB_LOG_TRACE("pb=%p: BIO connected\n", pb);
    {
        int fd = BIO_get_fd(pb->pal.socket, NULL);
        socket_set_rcv_timeout(fd, pb->transaction_timeout_ms);
    }

    rslt = SSL_get_verify_result(ssl);
    if (rslt != X509_V_OK) {
        PUBNUB_LOG_WARNING("pb=%p: SSL_get_verify_result() failed == %d(%s)\n", pb, rslt, X509_verify_cert_error_string(rslt));
        ERR_print_errors_cb(print_to_pubnub_log, NULL);
        if (pb->options.fallbackSSL) {
            BIO_free_all(pb->pal.socket);
            pb->pal.socket = NULL;
            return resolv_and_connect_wout_SSL(pb);
        }
        return pbpal_connect_failed;
    }

    PUBNUB_LOG_INFO("pb=%p: SSL session reused: %s\n", pb, SSL_session_reused(ssl) ? "yes" : "no");
    if (pb->pal.session != NULL) {
        SSL_SESSION_free(pb->pal.session);
    }
    pb->pal.session = SSL_get1_session(ssl);
    if (0 == pb->pal.ip_timeout) {
        pb->pal.ip_timeout = SSL_SESSION_get_time(pb->pal.session) + SSL_SESSION_get_timeout(pb->pal.session);
        memcpy(pb->pal.ip, BIO_get_conn_ip(pb->pal.socket), 4);
    }
    PUBNUB_LOG_TRACE("pb=%p: SSL connected to IP: %d.%d.%d.%d\n", pb, pb->pal.ip[0], pb->pal.ip[1], pb->pal.ip[2], pb->pal.ip[3]);

    return pbpal_connect_success;
}


enum pbpal_resolv_n_connect_result pbpal_check_resolv_and_connect(pubnub_t *pb)
{
    /* Under OpenSSL, this function should never be called.  Either
       we're synchrnous and just connected or not, or we're async, in
       which case, pbpal_check_connect() will be called.
     */
    PUBNUB_ASSERT_OPT(pb == NULL);
    return pbpal_connect_failed;
}


enum pbpal_resolv_n_connect_result pbpal_check_connect(pubnub_t *pb)
{
    fd_set read_set, write_set;
    int socket;
    int rslt;
    struct timeval timev = { 0, 300000 };

    if (-1 == BIO_get_fd(pb->pal.socket, &socket)) {
        PUBNUB_LOG_ERROR("pbpal_connected(): Uninitialized BIO!\n");
        return pbpal_connect_resource_failure;
    }
    FD_ZERO(&read_set);
    FD_ZERO(&write_set);
    FD_SET(socket, &read_set);
    FD_SET(socket, &write_set);
    rslt = select(socket + 1, &read_set, &write_set, NULL, &timev);
    if (SOCKET_ERROR == rslt) {
        PUBNUB_LOG_ERROR("pbpal_connected(): select() Error!\n");
        return pbpal_connect_resource_failure;
    }
    else if (rslt > 0) {
        PUBNUB_LOG_TRACE("pbpal_connected(): select() event\n");
        return pbpal_resolv_and_connect(pb);
    }
    PUBNUB_LOG_TRACE("pbpal_connected(): no select() events\n");
    return pbpal_connect_wouldblock;
}
