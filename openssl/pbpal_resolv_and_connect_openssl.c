/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "core/pbpal.h"

#ifdef _WIN32
#include <winsock2.h>
#include <Ws2tcpip.h>
#else
#include <sys/select.h>
#define SOCKET_ERROR -1
#endif

#include "pbpal_add_system_certs.h"
#include "pubnub_internal.h"
#include "core/pubnub_assert.h"
#include "core/pubnub_log.h"
#include "lib/sockets/pbpal_adns_sockets.h"
#include "lib/sockets/pbpal_socket_blocking_io.h"
#include "core/pubnub_dns_servers.h"

#include <sys/types.h>

#include <openssl/pem.h>
#include <openssl/ssl.h>

#include <string.h>


#define HTTP_PORT 80

#define DNS_PORT 53


static int print_to_pubnub_log(const char* s, size_t len, void* p)
{
    PUBNUB_UNUSED(len);

    PUBNUB_LOG_ERROR("From OpenSSL: pb=%p '%s'", p, s);

    return 0;
}


/** Someone removed `BIO_set_conn_int_port` from OpenSSL 1.1.0, but
    failed to update the docs... oh, well... we can make our own
    `BIO_set_conn_int_port`.
*/
static long my_BIO_set_conn_int_port(BIO* bio, int port)
{
    char s[20];
    snprintf(s, sizeof s, "%d", port);
    return BIO_set_conn_port(bio, s);
}


static void save_ip(pubnub_t* pb)
{
#if defined BIO_get_conn_ip
    /** While there existed BIO_get_conn_ip, OpenSSL didn't provide a
        regular way to see that IP version it worked with, so, we had
        to be conservative and read only 4 bytes (IPv4), otherwise we
        might read out of bounds. Of course, this was a problem if it
        actually did use IPv6, as this would not work.  "Luckily", it
        almost always used IPv4.
    */
    memcpy(pb->pal.ip, BIO_get_conn_ip(pb->pal.socket), 4);
#elif defined BIO_get_conn_address
    /* OpenSSL 1.1 provides another macro with some helper APIs that
       make this deterministic and able to support both IPv4 and IPv6
       (it's still ugly, though).
     */
    BIO_ADDR const* ap = BIO_get_conn_address(pb->pal.socket);
    PUBNUB_ASSERT_OPT(ap != NULL);
    BIO_ADDR_rawaddress(ap, NULL, &pb->pal.ip_len);
    pb->pal.ip_family = BIO_ADDR_family(ap);
    PUBNUB_ASSERT_UINT(pb->pal.ip_len, <=, sizeof pb->pal.ip / sizeof pb->pal.ip[0]);
    PUBNUB_ASSERT_OPT(pb->pal.ip_len > 0);
    BIO_ADDR_rawaddress(ap, pb->pal.ip, &pb->pal.ip_len);
#else
#error Don_t have BIO_get_conn_ip nor BIO_get_conn_address - can_t get the IP address of the connection
#endif
}


static void restore_ip(pubnub_t* pb)
{
#if defined BIO_set_conn_ip
    BIO_set_conn_ip(pb->pal.socket, pb->pal.ip);
#elif defined BIO_set_conn_address
    BIO_ADDR* ap = BIO_ADDR_new();
    if (NULL == ap) {
        PUBNUB_LOG_ERROR("Failed to BIO_ADDR_new()\n");
        return;
    }
    BIO_ADDR_rawmake(ap, pb->pal.ip_family, pb->pal.ip, pb->pal.ip_len, 0);
    BIO_set_conn_address(pb->pal.socket, ap);
    BIO_ADDR_free(ap);
#else
#error Don_t have BIO_set_conn_ip nor BIO_set_conn_address - can_t set the IP address of the connection
#endif
}

#ifdef PUBNUB_CALLBACK_API
static void get_dns_ip(struct sockaddr_in* addr)
{
    void* p = &(addr->sin_addr.s_addr);
    if ((pubnub_get_dns_primary_server_ipv4((struct pubnub_ipv4_address*)p) == -1)
        && (pubnub_get_dns_secondary_server_ipv4((struct pubnub_ipv4_address*)p)
            == -1)) {
        inet_pton(AF_INET, PUBNUB_DEFAULT_DNS_SERVER, p);
    }
}


static enum pbpal_resolv_n_connect_result start_dns_resolution(pubnub_t*   pb,
                                                               char const* origin)
{
    int                error;
    struct sockaddr_in dest;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));

    PUBNUB_LOG_TRACE("start_dns_resolution(pb=%p)\n", pb);

    if (SOCKET_INVALID == pb->pal.dns_socket) {
        pb->pal.dns_socket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    }
    if (SOCKET_INVALID == pb->pal.dns_socket) {
        return pbpal_resolv_resource_failure;
    }

    pbpal_set_socket_blocking_io(pb->pal.dns_socket, 0);
    dest.sin_family = AF_INET;
    dest.sin_port   = htons(DNS_PORT);
    get_dns_ip(&dest);
    error = send_dns_query(
        pb->pal.dns_socket, (struct sockaddr*)&dest, (unsigned char*)origin);
    if (error < 0) {
        return pbpal_resolv_failed_send;
    }
    else if (error > 0) {
        return pbpal_resolv_send_wouldblock;
    }
    return pbpal_resolv_sent;
}
#endif /* PUBNUB_CALLBACK_API */


static enum pbpal_resolv_n_connect_result finish_resolv_and_connect_wout_SSL(pubnub_t* pb)
{
    int port = HTTP_PORT;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    PUBNUB_LOG_TRACE("finish_resolv_and_connect_wout_SSL(pb=%p)\n", pb);

#if PUBNUB_PROXY_API
    switch (pb->proxy_type) {
    case pbproxyHTTP_CONNECT:
        port = pb->proxy_port;
        break;
    case pbproxyHTTP_GET:
        port = pb->proxy_port;
        PUBNUB_LOG_TRACE("pb=%p Using proxy-port: %d\n", pb, port);
        break;
    default:
        break;
    }
#endif /* PUBNUB_PROXY_API */

    if (NULL == pb->pal.socket) {
        return pbpal_resolv_resource_failure;
    }
    my_BIO_set_conn_int_port(pb->pal.socket, port);

    BIO_set_nbio(pb->pal.socket, !pb->options.use_blocking_io);

    WATCH_ENUM(pb->options.use_blocking_io);
    if (BIO_do_connect(pb->pal.socket) <= 0) {
        if (BIO_should_retry(pb->pal.socket)) {
            return pbpal_connect_wouldblock;
        }
        ERR_print_errors_cb(print_to_pubnub_log, pb);
        PUBNUB_LOG_ERROR("pb=%p BIO_do_connect failed, errno=%d\n", pb, errno);
        return pbpal_connect_failed;
    }

    PUBNUB_LOG_TRACE("pb=%p: BIO connected\n", pb);
    {
        pbpal_native_socket_t fd = BIO_get_fd(pb->pal.socket, NULL);
        socket_set_rcv_timeout(fd, pb->transaction_timeout_ms);
        socket_disable_SIGPIPE(pb->pal.socket);
    }

    return pbpal_connect_success;
}


static enum pbpal_resolv_n_connect_result resolv_and_connect_wout_SSL(pubnub_t* pb)
{
    PUBNUB_LOG_TRACE("resolv_and_connect_wout_SSL(pb=%p)\n", pb);

    if (NULL == pb->pal.socket) {
        char const* origin = PUBNUB_ORIGIN_SETTABLE ? pb->origin : PUBNUB_ORIGIN;
        PUBNUB_LOG_TRACE("pb=%p: Don't have BIO\n", pb);

#if PUBNUB_PROXY_API
        switch (pb->proxy_type) {
        case pbproxyHTTP_CONNECT:
            if (!pb->proxy_tunnel_established) {
                origin = pb->proxy_hostname;
            }
            break;
        case pbproxyHTTP_GET:
            origin = pb->proxy_hostname;
            PUBNUB_LOG_TRACE("pb=%p Using proxy-origin: %s\n", pb, origin);
            break;
        default:
            break;
        }
#endif /* PUBNUB_PROXY_API */
        pb->pal.socket = BIO_new_connect((char*)origin);
#ifdef PUBNUB_CALLBACK_API
        return start_dns_resolution(pb, origin);
#endif /* PUBNUB_CALLBACK_API */
    }
    return finish_resolv_and_connect_wout_SSL(pb);
}


/* Starfields Inc (Class 2) Root certificate.
   It was used to sign the server certificate of https://pubsub.pubnub.com.
   (at the time of writing this, 2015-06-04).
 */
static char pubnub_cert_Starfield[] =
    "-----BEGIN CERTIFICATE-----\n"
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


/* GlobalSign Root class 2 Certificate, used at the time of this
   writing (2016-11-26):

 2 s:/C=BE/O=GlobalSign nv-sa/OU=Root CA/CN=GlobalSign Root CA
   i:/C=BE/O=GlobalSign nv-sa/OU=Root CA/CN=GlobalSign Root CA

 */
static char pubnub_cert_GlobalSign[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDdTCCAl2gAwIBAgILBAAAAAABFUtaw5QwDQYJKoZIhvcNAQEFBQAwVzELMAkG\n"
    "A1UEBhMCQkUxGTAXBgNVBAoTEEdsb2JhbFNpZ24gbnYtc2ExEDAOBgNVBAsTB1Jv\n"
    "b3QgQ0ExGzAZBgNVBAMTEkdsb2JhbFNpZ24gUm9vdCBDQTAeFw05ODA5MDExMjAw\n"
    "MDBaFw0yODAxMjgxMjAwMDBaMFcxCzAJBgNVBAYTAkJFMRkwFwYDVQQKExBHbG9i\n"
    "YWxTaWduIG52LXNhMRAwDgYDVQQLEwdSb290IENBMRswGQYDVQQDExJHbG9iYWxT\n"
    "aWduIFJvb3QgQ0EwggEiMA0GCSqGSIb3DQEBAQUAA4IBDwAwggEKAoIBAQDaDuaZ\n"
    "jc6j40+Kfvvxi4Mla+pIH/EqsLmVEQS98GPR4mdmzxzdzxtIK+6NiY6arymAZavp\n"
    "xy0Sy6scTHAHoT0KMM0VjU/43dSMUBUc71DuxC73/OlS8pF94G3VNTCOXkNz8kHp\n"
    "1Wrjsok6Vjk4bwY8iGlbKk3Fp1S4bInMm/k8yuX9ifUSPJJ4ltbcdG6TRGHRjcdG\n"
    "snUOhugZitVtbNV4FpWi6cgKOOvyJBNPc1STE4U6G7weNLWLBYy5d4ux2x8gkasJ\n"
    "U26Qzns3dLlwR5EiUWMWea6xrkEmCMgZK9FGqkjWZCrXgzT/LCrBbBlDSgeF59N8\n"
    "9iFo7+ryUp9/k5DPAgMBAAGjQjBAMA4GA1UdDwEB/wQEAwIBBjAPBgNVHRMBAf8E\n"
    "BTADAQH/MB0GA1UdDgQWBBRge2YaRQ2XyolQL30EzTSo//z9SzANBgkqhkiG9w0B\n"
    "AQUFAAOCAQEA1nPnfE920I2/7LqivjTFKDK1fPxsnCwrvQmeU79rXqoRSLblCKOz\n"
    "yj1hTdNGCbM+w6DjY1Ub8rrvrTnhQ7k4o+YviiY776BQVvnGCv04zcQLcFGUl5gE\n"
    "38NflNUVyRRBnMRddWQVDf9VMOyGj/8N7yy5Y0b2qvzfvGn9LhJIZJrglfCm7ymP\n"
    "AbEVtQwdpf5pLGkkeB6zpxxxYu7KyJesF12KwvhHhm4qxFYxldBniYUr+WymXUad\n"
    "DKqC5JlR3XC321Y9YeRq4VzW9v493kHMB65jUr9TU/Qr6cf9tveCX4XSQRjbgbME\n"
    "HMUfpIBvFSDJ3gyICh3WZlXi/EjJKSZp4A==\n"
    "-----END CERTIFICATE-----\n";


static int add_pem_cert(SSL_CTX* sslCtx, char const* pem_cert)
{
    X509* cert;
    BIO*  mem = BIO_new(BIO_s_mem());
    if (NULL == mem) {
        PUBNUB_LOG_ERROR("SSL_CTX=%p: Failed BIO_new for PEM certificate\n", sslCtx);
        return -1;
    }
    BIO_puts(mem, pem_cert);
    cert = PEM_read_bio_X509(mem, NULL, 0, NULL);
    BIO_free(mem);
    if (NULL == cert) {
        ERR_print_errors_cb(print_to_pubnub_log, NULL);
        PUBNUB_LOG_ERROR("SSL_CTX=%p: Failed to read PEM certificate\n", sslCtx);
        return -1;
    }

    if (0 == X509_STORE_add_cert(SSL_CTX_get_cert_store(sslCtx), cert)) {
        ERR_print_errors_cb(print_to_pubnub_log, NULL);
        PUBNUB_LOG_ERROR("SSL_CTX=%p: Failed to add PEM certificate\n", sslCtx);
        X509_free(cert);
        return -1;
    }
    X509_free(cert);

    return 0;
}


static int add_pubnub_cert(SSL_CTX* sslCtx)
{
    int rslt = add_pem_cert(sslCtx, pubnub_cert_Starfield);
    return rslt || add_pem_cert(sslCtx, pubnub_cert_GlobalSign);
}


static void add_certs(pubnub_t* pb)
{
    if (pb->options.use_system_certificate_store
        && (0 == pbpal_add_system_certs(pb))) {
        return;
    }

    if (NULL != pb->ssl_userPEMcert) {
        add_pem_cert(pb->pal.ctx, pb->ssl_userPEMcert);
    }

    if ((NULL == pb->ssl_CAfile) && (NULL == pb->ssl_CApath)) {
        add_pubnub_cert(pb->pal.ctx);
    }
    else {
        if (!SSL_CTX_load_verify_locations(
                pb->pal.ctx, pb->ssl_CAfile, pb->ssl_CApath)) {
            ERR_print_errors_cb(print_to_pubnub_log, NULL);
            PUBNUB_LOG_ERROR(
                "SSL_CTX_load_verify_locations(CAfile=%s, CApath=%s) failed",
                pb->ssl_CAfile,
                pb->ssl_CApath);
        }
    }
}

static enum pbpal_resolv_n_connect_result finish_resolv_and_connect(pubnub_t* pb)
{
    int  rslt;
    int  port = 443;
    SSL* ssl  = NULL;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    PUBNUB_LOG_TRACE("finish_resolv_and_connect(pb=%p)\n", pb);

#if PUBNUB_PROXY_API
    switch (pb->proxy_type) {
    case pbproxyHTTP_CONNECT:
        port = pb->proxy_port;
        break;
    case pbproxyHTTP_GET:
        port = pb->proxy_port;
        PUBNUB_LOG_TRACE("pb=%p Using proxy-port: %d\n", pb, port);
        break;
    default:
        break;
    }
#endif /* PUBNUB_PROXY_API */

    BIO_get_ssl(pb->pal.socket, &ssl);
    PUBNUB_ASSERT(NULL != ssl);

    my_BIO_set_conn_int_port(pb->pal.socket, port);

    BIO_set_nbio(pb->pal.socket, !pb->options.use_blocking_io);

    if (pb->options.reuse_SSL_session && (pb->pal.session != NULL)) {
        if (!SSL_set_session(ssl, pb->pal.session)) {
            ERR_print_errors_cb(print_to_pubnub_log, NULL);
        }
    }

    WATCH_ENUM(pb->options.use_blocking_io);
    if (BIO_do_connect(pb->pal.socket) <= 0) {
        if (BIO_should_retry(pb->pal.socket) && PUBNUB_TIMERS_API
            && (pb->pal.connect_timeout > time(NULL))) {
            PUBNUB_LOG_TRACE("pb=%p: BIO_should_retry\n", pb);
            return pbpal_connect_wouldblock;
        }
        /* Expire the IP for the next connect */
        pb->pal.ip_timeout = 0;
        ERR_print_errors_cb(print_to_pubnub_log, NULL);
        if ((pb->pal.session != NULL) && pb->options.reuse_SSL_session) {
            SSL_SESSION_free(pb->pal.session);
            pb->pal.session = NULL;
        }
        PUBNUB_LOG_ERROR("pb=%p: BIO_do_connect failed, errno=%d\n", pb, errno);
        return pbpal_connect_failed;
    }

    PUBNUB_LOG_TRACE("pb=%p: BIO connected\n", pb);
    {
        pbpal_native_socket_t fd = BIO_get_fd(pb->pal.socket, NULL);
        socket_set_rcv_timeout(fd, pb->transaction_timeout_ms);
    }

    rslt = SSL_get_verify_result(ssl);
    if (rslt != X509_V_OK) {
        PUBNUB_LOG_WARNING("pb=%p: SSL_get_verify_result() failed == %d(%s)\n",
                           pb,
                           rslt,
                           X509_verify_cert_error_string(rslt));
        ERR_print_errors_cb(print_to_pubnub_log, NULL);
        if (pb->options.fallbackSSL) {
            BIO_free_all(pb->pal.socket);
            pb->pal.socket = NULL;
            return resolv_and_connect_wout_SSL(pb);
        }
        return pbpal_connect_failed;
    }

    if (pb->options.reuse_SSL_session) {
        PUBNUB_LOG_INFO("pb=%p: SSL session reused: %s\n",
                        pb,
                        SSL_session_reused(ssl) ? "yes" : "no");
        if (pb->pal.session != NULL) {
            SSL_SESSION_free(pb->pal.session);
        }
        pb->pal.session = SSL_get1_session(ssl);
        if (0 == pb->pal.ip_timeout) {
            pb->pal.ip_timeout = SSL_SESSION_get_time(pb->pal.session)
                                 + SSL_SESSION_get_timeout(pb->pal.session);
            save_ip(pb);
        }
        PUBNUB_LOG_TRACE("pb=%p: SSL connected to IP: %u.%u.%u.%u\n",
                         pb,
                         (uint8_t)pb->pal.ip[0],
                         (uint8_t)pb->pal.ip[1],
                         (uint8_t)pb->pal.ip[2],
                         (uint8_t)pb->pal.ip[3]);
    }

    return pbpal_connect_success;
}


enum pbpal_resolv_n_connect_result pbpal_resolv_and_connect(pubnub_t* pb)
{
    SSL*        ssl    = NULL;
    char const* origin = PUBNUB_ORIGIN_SETTABLE ? pb->origin : PUBNUB_ORIGIN;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    PUBNUB_ASSERT_OPT((pb->state == PBS_READY) || (pb->state == PBS_WAIT_CONNECT));

    if (!pb->options.useSSL) {
        return resolv_and_connect_wout_SSL(pb);
    }

#if PUBNUB_PROXY_API
    switch (pb->proxy_type) {
    case pbproxyHTTP_CONNECT:
        if (!pb->proxy_tunnel_established) {
            return resolv_and_connect_wout_SSL(pb);
        }
        break;
    case pbproxyHTTP_GET:
        origin = pb->proxy_hostname;
        PUBNUB_LOG_TRACE("pb=%p Using proxy-origin: %s\n", pb, origin);
        break;
    default:
        break;
    }
#endif

    if (NULL == pb->pal.ctx) {
        PUBNUB_LOG_TRACE("pb=%p: Don't have SSL_CTX\n", pb);
        pb->pal.ctx = SSL_CTX_new(SSLv23_client_method());
        if (NULL == pb->pal.ctx) {
            ERR_print_errors_cb(print_to_pubnub_log, NULL);
            PUBNUB_LOG_ERROR("pb=%p SSL_CTX_new failed\n", pb);
            return pbpal_resolv_resource_failure;
        }
        PUBNUB_LOG_TRACE("pb=%p: Got SSL_CTX\n", pb);
        add_certs(pb);
    }

    if (NULL == pb->pal.socket) {
        PUBNUB_LOG_TRACE("pb=%p: Don't have BIO\n", pb);
        pb->pal.socket = BIO_new_ssl_connect(pb->pal.ctx);
        if (PUBNUB_TIMERS_API) {
            pb->pal.connect_timeout = time(NULL) + pb->transaction_timeout_ms / 1000;
        }
    }
    else {
        BIO_get_ssl(pb->pal.socket, &ssl);
        if (NULL == ssl) {
#if PUBNUB_PROXY_API
            if (pbproxyHTTP_CONNECT == pb->proxy_type) {
                BIO* ssl_bio = BIO_new_ssl(pb->pal.ctx, 1);
                if (NULL == ssl_bio) {
                    ERR_print_errors_cb(print_to_pubnub_log, NULL);
                    PUBNUB_LOG_ERROR("pb=%p BIO_new_ssl() failed\n", pb);
                    return pbpal_resolv_resource_failure;
                }
                else {
                    pb->pal.socket = BIO_push(pb->pal.socket, ssl_bio);
                }
            }
            else {
                BIO_free_all(pb->pal.socket);
                pb->pal.socket = NULL;
                return resolv_and_connect_wout_SSL(pb);
            }
#else
            BIO_free_all(pb->pal.socket);
            pb->pal.socket = NULL;
            return resolv_and_connect_wout_SSL(pb);
#endif
        }
        ssl = NULL;
    }

    if (NULL == pb->pal.socket) {
        ERR_print_errors_cb(print_to_pubnub_log, NULL);
        return pbpal_resolv_resource_failure;
    }

    PUBNUB_LOG_TRACE("pb=%p: Using BIO == %p\n", pb, pb->pal.socket);

    BIO_get_ssl(pb->pal.socket, &ssl);
    PUBNUB_ASSERT(NULL != ssl);
    SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY); /* maybe not auto_retry? */

    if (pb->pal.ip_timeout != 0) {
        if (pb->pal.ip_timeout < time(NULL)) {
            pb->pal.ip_timeout = 0;

#ifdef PUBNUB_CALLBACK_API
            return start_dns_resolution(pb, origin);
#else
            BIO_set_conn_hostname(pb->pal.socket, origin);
#endif /* PUBNUB_CALLBACK_API */
        }
        else {
            PUBNUB_LOG_TRACE("pb=%p SSL re-connect to: %u.%u.%u.%u\n",
                             pb,
                             (uint8_t)pb->pal.ip[0],
                             (uint8_t)pb->pal.ip[1],
                             (uint8_t)pb->pal.ip[2],
                             (uint8_t)pb->pal.ip[3]);
            restore_ip(pb);
        }
    }
    else {
#ifdef PUBNUB_CALLBACK_API
        return start_dns_resolution(pb, origin);
#else
        BIO_set_conn_hostname(pb->pal.socket, origin);
#endif /* PUBNUB_CALLBACK_API */
    }
    return finish_resolv_and_connect(pb);
}


enum pbpal_resolv_n_connect_result pbpal_check_resolv_and_connect(pubnub_t* pb)
{
#ifdef PUBNUB_CALLBACK_API
    char                  s[20];
    SSL*                  ssl = NULL;
    struct sockaddr_in    dns_server;
    struct sockaddr_in    dest;
    pbpal_native_socket_t skt = pb->pal.dns_socket;

    dns_server.sin_family = AF_INET;
    dns_server.sin_port   = htons(DNS_PORT);
    get_dns_ip(&dns_server);
    switch (read_dns_response(skt, (struct sockaddr*)&dns_server, &dest)) {
    case -1:
        return pbpal_resolv_failed_rcv;
    case +1:
        return pbpal_resolv_rcv_wouldblock;
    case 0:
        break;
    }
    socket_close(skt);
    pb->pal.dns_socket = SOCKET_INVALID;

    memcpy(pb->pal.ip, &dest.sin_addr.s_addr, 4);
    BIO_set_conn_hostname(pb->pal.socket, inet_ntop(AF_INET, pb->pal.ip, s, sizeof s));

    BIO_get_ssl(pb->pal.socket, &ssl);
    if (NULL == ssl) {
        return finish_resolv_and_connect_wout_SSL(pb);
    }
    return finish_resolv_and_connect(pb);
#else
    /* Under OpenSSL, this function should never be called with synchrnous api
       in which case we're just connected or not, and, pbpal_check_connect()
       will be called.
     */
    PUBNUB_ASSERT_OPT(pb == NULL);
    return pbpal_connect_failed;
#endif /* PUBNUB_CALLBACK_API */
}


enum pbpal_resolv_n_connect_result pbpal_check_connect(pubnub_t* pb)
{
    SSL*           ssl = NULL;
    fd_set         read_set, write_set;
    int            socket;
    int            rslt;
    struct timeval timev = { 0, 300000 };

    if (SOCKET_INVALID == BIO_get_fd(pb->pal.socket, &socket)) {
        PUBNUB_LOG_ERROR("pbpal_check_connect(pb=%p): Uninitialized BIO!\n", pb);
        return pbpal_connect_resource_failure;
    }
    FD_ZERO(&read_set);
    FD_ZERO(&write_set);
    FD_SET(socket, &read_set);
    FD_SET(socket, &write_set);
    rslt = select(socket + 1, &read_set, &write_set, NULL, &timev);
    if (SOCKET_ERROR == rslt) {
        PUBNUB_LOG_ERROR("pbpal_check_connect(pb=%p): select() Error!\n", pb);
        return pbpal_connect_resource_failure;
    }
    else if (rslt > 0) {
        PUBNUB_LOG_TRACE("pbpal_check_connect(pb=%p): select() event\n", pb);
        BIO_get_ssl(pb->pal.socket, &ssl);
        if (NULL == ssl) {
            return finish_resolv_and_connect_wout_SSL(pb);
        }
        return finish_resolv_and_connect(pb);
    }
    PUBNUB_LOG_TRACE("pbpal_check_connect(pb=%p): no select() events\n", pb);
    return pbpal_connect_wouldblock;
}
