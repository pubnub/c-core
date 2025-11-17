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

/* Amazon Root CA 1
   Used by PubNub domains: ps.pndsn.com, *.pubnubapi.com

   Subject: C=US, O=Amazon, CN=Amazon Root CA 1
   Issuer: C=US, O=Amazon, CN=Amazon Root CA 1

   Valid from: May 26, 2015
   Valid to: January 17, 2038

   Key: RSA 2048-bit
   Source: https://www.amazontrust.com/repository/AmazonRootCA1.pem
 */
static char pubnub_cert_Amazon_Root_CA_1[] =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIDQTCCAimgAwIBAgITBmyfz5m/jAo54vB4ikPmljZbyjANBgkqhkiG9w0BAQsF\n"
    "ADA5MQswCQYDVQQGEwJVUzEPMA0GA1UEChMGQW1hem9uMRkwFwYDVQQDExBBbWF6\n"
    "b24gUm9vdCBDQSAxMB4XDTE1MDUyNjAwMDAwMFoXDTM4MDExNzAwMDAwMFowOTEL\n"
    "MAkGA1UEBhMCVVMxDzANBgNVBAoTBkFtYXpvbjEZMBcGA1UEAxMQQW1hem9uIFJv\n"
    "b3QgQ0EgMTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBALJ4gHHKeNXj\n"
    "ca9HgFB0fW7Y14h29Jlo91ghYPl0hAEvrAIthtOgQ3pOsqTQNroBvo3bSMgHFzZM\n"
    "9O6II8c+6zf1tRn4SWiw3te5djgdYZ6k/oI2peVKVuRF4fn9tBb6dNqcmzU5L/qw\n"
    "IFAGbHrQgLKm+a/sRxmPUDgH3KKHOVj4utWp+UhnMJbulHheb4mjUcAwhmahRWa6\n"
    "VOujw5H5SNz/0egwLX0tdHA114gk957EWW67c4cX8jJGKLhD+rcdqsq08p8kDi1L\n"
    "93FcXmn/6pUCyziKrlA4b9v7LWIbxcceVOF34GfID5yHI9Y/QCB/IIDEgEw+OyQm\n"
    "jgSubJrIqg0CAwEAAaNCMEAwDwYDVR0TAQH/BAUwAwEB/zAOBgNVHQ8BAf8EBAMC\n"
    "AYYwHQYDVR0OBBYEFIQYzIU07LwMlJQuCFmcx7IQTgoIMA0GCSqGSIb3DQEBCwUA\n"
    "A4IBAQCY8jdaQZChGsV2USggNiMOruYou6r4lK5IpDB/G/wkjUu0yKGX9rbxenDI\n"
    "U5PMCCjjmCXPI6T53iHTfIUJrU6adTrCC2qJeHZERxhlbI1Bjjt/msv0tadQ1wUs\n"
    "N+gDS63pYaACbvXy8MWy7Vu33PqUXHeeE6V/Uq2V8viTO96LXFvKWlJbYK8U90vv\n"
    "o/ufQJVtMVT8QtPHRh8jrdkPSHCa2XV4cdFyQzR1bldZwgJcJmApzyMZFo6IQ6XU\n"
    "5MsI+yMRQ+hDKXJioaldXgjUkK642M4UwtBV8ob2xJNDd2ZhwLnoQdeXeGADbkpy\n"
    "rqXRfboQnoZsG4q5WTP468SQvvG5\n"
    "-----END CERTIFICATE-----\n";


/* Starfield Class 2 Certification Authority
   Used by PubNub domains: pubsub.pubnub.com, pubnub.pubnub.com, *.pubnub.com
   Validates certificates issued by Starfield Root Certificate Authority - G2
   through cross-certification.

   Subject: C=US, O=Starfield Technologies, Inc.,
            OU=Starfield Class 2 Certification Authority
   Issuer: C=US, O=Starfield Technologies, Inc.,
           OU=Starfield Class 2 Certification Authority

   Valid from: June 29, 2004
   Valid to: June 29, 2034

   Key: RSA 2048-bit
   Source: Extracted from pubsub.pubnub.com certificate chain (2015-06-04)
           Available at https://certs.starfieldtech.com/repository/
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
    /* Load certificates in priority order:
       1. Amazon Root CA 1 - Primary for PubNub domains (ps.pndsn.com, *.pubnubapi.com)
       2. Starfield Root CA G2 - For pubsub.pubnub.com
     */
    int rslt = add_pem_cert(sslCtx, pubnub_cert_Amazon_Root_CA_1);
    rslt = rslt || add_pem_cert(sslCtx, pubnub_cert_Starfield);
    return rslt;
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

    /** Enable SNI (Server Name Indication) for virtual hosting support. */
    if (!SSL_set_tlsext_host_name(ssl, pb->origin)) {
        PUBNUB_LOG_WARNING(
            "pb=%p: SSL_set_tlsext_host_name() failed to set SNI hostname '%s'\n",
            pb,
            pb->origin);
        ERR_print_errors_cb(print_to_pubnub_log, pb);
        return pbtlsFailed;
    }

    /** Enable hostname verification. */
    X509_VERIFY_PARAM *param = SSL_get0_param(ssl);
    if (param != NULL) {
        char const* hostname = pb->origin;
        if (!X509_VERIFY_PARAM_set1_host(param, hostname, 0)) {
            PUBNUB_LOG_WARNING(
                "pb=%p: X509_VERIFY_PARAM_set1_host() failed to set hostname '%s' for verification\n",
                pb,
                hostname);
            return pbtlsFailed;
        }
        PUBNUB_LOG_DEBUG("pb=%p: Hostname verification configured for '%s'\n",
                         pb, hostname);
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

    bool needRead = false, needWrite = false;
    rslt = SSL_connect(ssl);
    rslt = pbpal_handle_socket_condition(rslt, pb, __FILE__, __LINE__, &needRead, &needWrite);
    if (PNR_OK != rslt) {
        if (rslt != PNR_IN_PROGRESS) return pbtlsFailed;
        return needRead ? pbtlsStartedWaitRead : needWrite ? pbtlsStartedWaitWrite : pbtlsStarted;
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
