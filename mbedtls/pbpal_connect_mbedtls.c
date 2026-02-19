#include "pubnub_internal.h"

#if PUBNUB_USE_SSL

#ifndef ESP_PLATFORM
#error                                                                         \
    "MBEDTLS is supported only on ESP32 platform. Contact PubNub support for other platforms."
#endif

#include "pbpal.h"
#include "pubnub_netcore.h"
#include "pubnub_pal.h"
#include "pubnub_api_types.h"
#include "pubnub_internal_common.h"
#if PUBNUB_USE_LOGGER
#include "core/pubnub_logger.h"
#endif // PUBNUB_USE_LOGGER
#include "pubnub_assert.h"
#include "pubnub_helper.h"

#ifdef ESP_PLATFORM
#include "esp_crt_bundle.h"
#endif

#include "mbedtls/ssl.h"

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

#if PUBNUB_USE_LETS_ENCRYPT_CERTIFICATE
/* ISRG Root X1 (Let's Encrypt)
   Used for testing and validation purposes with badssl.com and other Let's
   Encrypt domains. Note: This is NOT used by PubNub production infrastructure.

   Subject: C=US, O=Internet Security Research Group, CN=ISRG Root X1
   Issuer: C=US, O=Internet Security Research Group, CN=ISRG Root X1

   Valid from: June 4, 2015
   Valid to: June 4, 2035

   Key: RSA 4096-bit
   Source: https://letsencrypt.org/certs/isrgrootx1.pem
 */
static char pubnub_cert_ISRG_Root_X1[] =
    "-----BEGIN CERTIFICATE-----\n"
    "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\n"
    "-----END CERTIFICATE-----\n";
#endif

static void alloc_setup(pubnub_t* pb);

#define PUBNUB_PORT "443"

enum pbpal_tls_result pbpal_start_tls(pubnub_t* pb)
{
    struct pubnub_pal* pal = &pb->pal;

    PUBNUB_LOG_TRACE(
        pb, "Establishing TLS/SSL over existing TCP/IP connection.");
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    PUBNUB_ASSERT_OPT(PBS_CONNECTED == pb->state);

    alloc_setup(pb);

    PUBNUB_ASSERT(SOCKET_INVALID != pb->pal.socket);
    PUBNUB_LOG_TRACE(pb, "Prepared socket: %d", pb->pal.socket);

    mbedtls_ssl_init(pal->ssl);
    mbedtls_ssl_config_init(pal->ssl_config);
    mbedtls_entropy_init(pal->entropy);
    mbedtls_ctr_drbg_init(pal->ctr_drbg);
    mbedtls_x509_crt_init(pal->ca_certificates);
    mbedtls_net_init(pal->net);

    // TODO: settable seed
    if (0 != mbedtls_ctr_drbg_seed(
                 pal->ctr_drbg,
                 mbedtls_entropy_func,
                 pal->entropy,
                 (const unsigned char*)"pubnub",
                 6)) {
        PUBNUB_LOG_ERROR(pb, "Failed to seed random number generator.");
        return pbtlsFailed;
    }

    /* Load certificates in priority order:
       1. Amazon Root CA 1 - Primary for PubNub domains (ps.pndsn.com,
       *.pubnubapi.com)
       2. Starfield Root CA G2 - For pubsub.pubnub.com
       3. ISRG Root X1 - For testing with Let's Encrypt domains (badssl.com,
       etc.)
     */
    if (0 != mbedtls_x509_crt_parse(
                 pal->ca_certificates,
                 (const unsigned char*)pubnub_cert_Amazon_Root_CA_1,
                 sizeof(pubnub_cert_Amazon_Root_CA_1)) &&
        0 != mbedtls_x509_crt_parse(
                 pal->ca_certificates,
                 (const unsigned char*)pubnub_cert_Starfield,
                 sizeof(pubnub_cert_Starfield))
#if PUBNUB_USE_LETS_ENCRYPT_CERTIFICATE
        && 0 != mbedtls_x509_crt_parse(
                    pal->ca_certificates,
                    (const unsigned char*)pubnub_cert_ISRG_Root_X1,
                    sizeof(pubnub_cert_ISRG_Root_X1))
#endif
    ) {
        PUBNUB_LOG_ERROR(pb, "Failed to parse CA certificate.");
        return pbtlsFailed;
    }

#ifndef ESP_PLATFORM
#error                                                                         \
    "MBedTLS has been implemented only for ESP32 platform. Contact PubNub support for an implementation on the other ones."
#else
    if (esp_crt_bundle_attach(pal->ssl_config) != 0) {
        PUBNUB_LOG_ERROR(pb, "Failed to attach CRT bundle.");
        return pbtlsFailed;
    }
#endif

    if (mbedtls_ssl_set_hostname(pal->ssl, PUBNUB_ORIGIN) != 0) {
        PUBNUB_LOG_ERROR(pb, "Failed to set hostname: '%s'", PUBNUB_ORIGIN);
        return pbtlsFailed;
    }

    if (mbedtls_ssl_config_defaults(
            pal->ssl_config,
            MBEDTLS_SSL_IS_CLIENT,
            MBEDTLS_SSL_TRANSPORT_STREAM,
            MBEDTLS_SSL_PRESET_DEFAULT) != 0) {
        PUBNUB_LOG_ERROR(pb, "Failed to set SSL config defaults.");
        return pbtlsFailed;
    }

    mbedtls_ssl_conf_authmode(pal->ssl_config, MBEDTLS_SSL_VERIFY_REQUIRED);
    mbedtls_ssl_conf_ca_chain(pal->ssl_config, pal->ca_certificates, NULL);

    mbedtls_ssl_conf_rng(
        pal->ssl_config, mbedtls_ctr_drbg_random, pal->ctr_drbg);
    mbedtls_ssl_conf_dbg(pal->ssl_config, NULL, NULL);

    if (mbedtls_ssl_setup(pal->ssl, pal->ssl_config) != 0) {
        PUBNUB_LOG_ERROR(pb, "Failed to setup SSL.");
        return pbtlsFailed;
    }

    PUBNUB_LOG_DEBUG(pb, "Connecting to %s:%s", PUBNUB_ORIGIN, PUBNUB_PORT);
    if (0 !=
        mbedtls_net_connect(
            pb->pal.net, PUBNUB_ORIGIN, PUBNUB_PORT, MBEDTLS_NET_PROTO_TCP)) {
        PUBNUB_LOG_ERROR(
            pb, "Unable to connect to %s:%s", PUBNUB_ORIGIN, PUBNUB_PORT);
        return pbtlsFailed;
    }

    PUBNUB_LOG_DEBUG(pb, "Connected to %s:%s", PUBNUB_ORIGIN, PUBNUB_PORT);

    mbedtls_ssl_set_bio(
        pal->ssl, pb->pal.net, mbedtls_net_send, mbedtls_net_recv, NULL);

    return pbpal_check_tls(pb);
}

enum pbpal_tls_result pbpal_check_tls(pubnub_t* pb)
{
    int  result;
    int  tls_flags;
    char error_buf[512]; // 512 bytes according to mbedtls example

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    PUBNUB_ASSERT_OPT(PBS_CONNECTED == pb->state);

    bool needRead = false, needWrite = false;
    result = mbedtls_ssl_handshake(pb->pal.ssl);

    if (PNR_OK !=
        (result = pbpal_handle_socket_condition(
             result, pb, __FILE__, __LINE__, &needRead, &needWrite))) {
        PUBNUB_LOG_TRACE(
            pb,
            "Socket condition: %d (%s)",
            result,
            pubnub_res_2_string(result));
        if (result != PNR_IN_PROGRESS) return pbtlsFailed;
        return needRead    ? pbtlsStartedWaitRead
               : needWrite ? pbtlsStartedWaitWrite
                           : pbtlsStarted;
    }

    PUBNUB_LOG_DEBUG(pb, "TLS connection established.");

    if ((0 != (tls_flags = mbedtls_ssl_get_verify_result(pb->pal.ssl)))) {
        mbedtls_x509_crt_verify_info(
            error_buf, sizeof error_buf, "  ! ", tls_flags);
        PUBNUB_LOG_ERROR(pb, "Certificate verification failed: %s", error_buf);

        return pbtlsFailed;
    }

    PUBNUB_LOG_DEBUG(pb, "TLS Certificate verification passed.");
    PUBNUB_LOG_TRACE(
        pb, "Cipher suite is %s", mbedtls_ssl_get_ciphersuite(pb->pal.ssl));

    return pbtlsEstablished;
}

static void alloc_setup(pubnub_t* pb)
{
    pb->pal.ssl = (mbedtls_ssl_context*)malloc(sizeof(mbedtls_ssl_context));
    if (pb->pal.ssl == NULL) {
        PUBNUB_LOG_ERROR(pb, "Failed to allocate memory for SSL context.");
        return;
    }

    pb->pal.ssl_config =
        (mbedtls_ssl_config*)malloc(sizeof(mbedtls_ssl_config));
    if (pb->pal.ssl_config == NULL) {
        PUBNUB_LOG_ERROR(
            pb, "Failed to allocate memory for SSL configuration.");
        return;
    }

    pb->pal.net = (mbedtls_net_context*)malloc(sizeof(mbedtls_net_context));
    if (pb->pal.net == NULL) {
        PUBNUB_LOG_ERROR(pb, "Failed to allocate memory for net context.");
        return;
    }

    pb->pal.ca_certificates =
        (mbedtls_x509_crt*)malloc(sizeof(mbedtls_x509_crt));
    if (pb->pal.ca_certificates == NULL) {
        PUBNUB_LOG_ERROR(pb, "Failed to allocate memory for x509.");
        return;
    }

    pb->pal.server_fd =
        (mbedtls_net_context*)malloc(sizeof(mbedtls_net_context));
    if (pb->pal.server_fd == NULL) {
        PUBNUB_LOG_ERROR(
            pb, "Failed to allocate memory for next cnotext (server_fd)");
        return;
    }

    pb->pal.entropy =
        (mbedtls_entropy_context*)malloc(sizeof(mbedtls_entropy_context));
    if (pb->pal.entropy == NULL) {
        PUBNUB_LOG_ERROR(pb, "Failed to allocate memory for entropy context.");
        return;
    }

    pb->pal.ctr_drbg =
        (mbedtls_ctr_drbg_context*)malloc(sizeof(mbedtls_ctr_drbg_context));
    if (pb->pal.ctr_drbg == NULL) {
        PUBNUB_LOG_ERROR(pb, "Failed to allocate memory for drbg context");
        return;
    }
}
#endif /* PUBNUB_USE_SSL */
