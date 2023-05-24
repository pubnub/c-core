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
#include <openssl/err.h>
#include <openssl/ssl.h>

#include <string.h>
#include <stdlib.h>

#define TLS_PORT 443


PUBNUB_STATIC_ASSERT(PUBNUB_TIMERS_API, need_TIMERS_API);


static int print_to_pubnub_log(const char* s, size_t len, void* p)
{
    PUBNUB_UNUSED(len);

    PUBNUB_LOG_ERROR("From OpenSSL: pb=%p '%s'", p, s);

    return 0;
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
    PUBNUB_LOG_TRACE(
        "add_certs(pb=%p): pb->options.use_system_certificate_store=%d, "
        "pb->ssl_userPEMcert=%p, pb->ssl_CAfile='%s', pb->ssl_CApath='%s'.\n",
        pb,
        pb->options.use_system_certificate_store,
        pb->ssl_userPEMcert,
        pb->ssl_CAfile,
        pb->ssl_CApath);

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

enum pbpal_tls_result pbpal_start_tls(pubnub_t* pb)
{
    SSL* ssl;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    PUBNUB_ASSERT_OPT(PBS_CONNECTED == pb->state);
    PUBNUB_ASSERT(SOCKET_INVALID != pb->pal.socket);
    ssl = pb->pal.ssl;
    PUBNUB_ASSERT(NULL == ssl);

    if (NULL == pb->pal.ctx) {
        PUBNUB_LOG_TRACE("pb=%p: Don't have SSL_CTX\n", pb);
        pb->pal.ctx = SSL_CTX_new(SSLv23_client_method());
        if (NULL == pb->pal.ctx) {
            ERR_print_errors_cb(print_to_pubnub_log, pb);
            PUBNUB_LOG_ERROR("pb=%p SSL_CTX_new failed\n", pb);
            return pbtlsResourceFailure;
        }
        PUBNUB_LOG_TRACE("pb=%p: Got SSL_CTX\n", pb);
        add_certs(pb);
    }
    ssl = pb->pal.ssl = SSL_new(pb->pal.ctx);
    if (NULL == ssl) {
        ERR_print_errors_cb(print_to_pubnub_log, pb);
        PUBNUB_LOG_ERROR("pb=%p SSL_new failed\n", pb);
        return pbtlsResourceFailure;
    }
    PUBNUB_LOG_TRACE("pb=%p: Got SSL\n", pb);
    SSL_set_fd(ssl, pb->pal.socket);
    WATCH_ENUM(pb->options.use_blocking_io);
    pb->pal.tryconn = pbms_start();
    if (pb->options.reuse_SSL_session && (pb->pal.session != NULL)) {
        if (!SSL_set_session(ssl, pb->pal.session)) {
            ERR_print_errors_cb(print_to_pubnub_log, NULL);
        }
    }

    return pbpal_check_tls(pb);
}

/** Called after 'pbpal_start_tls()'. Does necessary arrangements called
   repeatedly, if necessary, until TLS/SSL platform connection is established,
   or failed to establish.
*/
enum pbpal_tls_result pbpal_check_tls(pubnub_t* pb)
{
    SSL* ssl;
    int  rslt;
    X509* cert;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    PUBNUB_ASSERT_OPT((PBS_CONNECTED == pb->state)
                      || (PBS_WAIT_TLS_CONNECT == pb->state));
    PUBNUB_ASSERT(SOCKET_INVALID != pb->pal.socket);
    ssl = pb->pal.ssl;
    PUBNUB_ASSERT(NULL != ssl);

    rslt = SSL_connect(ssl);
    rslt = pbpal_handle_socket_condition(rslt, pb, __FILE__, __LINE__);
    if (PNR_OK != rslt) {
        return (rslt == PNR_IN_PROGRESS) ? pbtlsStarted : pbtlsFailed;
    }
    PUBNUB_LOG_TRACE("pb=%p: SSL connected\n", pb);
    socket_set_rcv_timeout(pb->pal.socket, pb->transaction_timeout_ms);

    #if OPENSSL_VERSION_NUMBER < 0x30000000L
    cert = SSL_get_peer_certificate(ssl);
    #else
    cert = SSL_get1_peer_certificate(ssl);
    #endif
    if (cert != NULL) {
        rslt = SSL_get_verify_result(ssl);
        X509_free(cert);
        if (rslt != X509_V_OK) {
            PUBNUB_LOG_WARNING(
                "pb=%p: SSL_get_verify_result() failed == %d(%s)\n",
                pb,
                rslt,
                X509_verify_cert_error_string(rslt));
            ERR_print_errors_cb(print_to_pubnub_log, NULL);

            return pbtlsFailed;
        }
    }
    else {
        PUBNUB_LOG_WARNING(
            "pb=%p: SSL -the peer certificate was not presented.\n", pb);
    }

    if (pb->options.reuse_SSL_session) {
        PUBNUB_LOG_INFO("pb=%p: SSL session reused: %s\n",
                        pb,
                        SSL_session_reused(pb->pal.ssl) ? "yes" : "no");
        if (pb->pal.session != NULL) {
            SSL_SESSION_free(pb->pal.session);
        }
        pb->pal.session = SSL_get1_session(pb->pal.ssl);
        if (0 == pb->pal.ip_timeout) {
            pb->pal.ip_timeout = SSL_SESSION_get_time(pb->pal.session)
                                 + SSL_SESSION_get_timeout(pb->pal.session);
        }
    }

    pbms_stop(&pb->pal.tryconn);

    return pbtlsEstablished;
}
