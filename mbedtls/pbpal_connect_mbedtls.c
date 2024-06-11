#include "pubnub_internal.h"

#if PUBNUB_USE_SSL

#ifndef ESP_PLATFORM
#error "MBEDTLS is supported only on ESP32 platform. Contact PubNub support for other platforms."
#endif

#include "pbpal.h"
#include "pubnub_netcore.h"
#include "pubnub_pal.h"
#include "pubnub_api_types.h"
#include "pubnub_internal_common.h"
#include "pubnub_log.h"
#include "pubnub_assert.h"

#ifdef ESP_PLATFORM
#include "esp_crt_bundle.h"
#endif

#include "mbedtls/ssl.h"

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

static const char* get_origin(pubnub_t* pb)
{
#ifdef PUBNUB_ORIGIN_SETTABLE
    return pb->origin;
#else
    PUBNUB_UNUSED(pb);
    return PUBNUB_ORIGIN;
#endif
}

#define PUBNUB_PORT "443"

// TODO: https://github.com/espressif/esp-idf/blob/v5.2.1/examples/protocols/https_mbedtls/main/https_mbedtls_example_main.c 
// reference for mbedtls usage
enum pbpal_tls_result pbpal_start_tls(pubnub_t* pb)
{
    struct pubnub_pal* pal = &pb->pal;
    int net_result;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    PUBNUB_ASSERT_OPT(PBS_CONNECTED == pb->state);
    PUBNUB_LOG_TRACE("pbpal_start_tls(pb=%p)\n", pb);
// TODO: Think about pubnub_config.h and where or which to use
//    PUBNUB_ASSERT(SOCKET_INVALID != pb->pal.socket);

    mbedtls_ssl_init(pal->ssl);
    mbedtls_ssl_config_init(pal->ssl_config);

#ifndef ESP_PLATFORM
#error "MBedTLS has been implemented only for ESP32 platform. Contact PubNub support for an implementation on the other ones."
#else
    if(esp_crt_bundle_attach(pal->ssl_config) != 0) {
        PUBNUB_LOG_ERROR("Failed to attach CRT bundle\n");
        return pbtlsFailed;
    }
#endif

    if (mbedtls_ssl_set_hostname(pal->ssl, get_origin(pb)) != 0) {
        PUBNUB_LOG_ERROR("Failed to set hostname\n");
        return pbtlsFailed;
    }

    if (mbedtls_ssl_config_defaults(
                pal->ssl_config,
                MBEDTLS_SSL_IS_CLIENT,
                MBEDTLS_SSL_TRANSPORT_STREAM,
                MBEDTLS_SSL_PRESET_DEFAULT
            ) != 0) {
        PUBNUB_LOG_ERROR("Failed to set SSL config defaults\n");
        return pbtlsFailed;
    }

    mbedtls_ssl_conf_authmode(pal->ssl_config, MBEDTLS_SSL_VERIFY_REQUIRED);
    mbedtls_ssl_conf_ca_chain(pal->ssl_config, pal->ca_certificates, NULL);

    if (mbedtls_ssl_setup(pal->ssl, pal->ssl_config) != 0) {
        PUBNUB_LOG_ERROR("Failed to setup SSL\n");
        return pbtlsFailed;
    }

    mbedtls_net_init(pb->pal.net);

    PUBNUB_LOG_DEBUG("Connecting to %s:%s...\n", get_origin(pb), PUBNUB_PORT);
    if (0 != mbedtls_net_connect(pb->pal.net, get_origin(pb), PUBNUB_PORT, MBEDTLS_NET_PROTO_TCP)) {
        PUBNUB_LOG_ERROR("Failed to connect to %s:%s\n", get_origin(pb), PUBNUB_PORT);
        return pbtlsFailed;
    }

    PUBNUB_LOG_DEBUG("Connected to %s:%s\n", get_origin(pb), PUBNUB_PORT);

    // TODO: HOW TO SET PEM CERTS?
    mbedtls_ssl_set_bio(pal->ssl, pb->pal.net, mbedtls_net_send, mbedtls_net_recv, NULL);

    return pbpal_check_tls(pb);
}

enum pbpal_tls_result pbpal_check_tls(pubnub_t* pb) {
    int result;
    int tls_flags;
    char error_buf[512]; // 512 bytes according to mbedtls example

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    PUBNUB_ASSERT_OPT(PBS_CONNECTED == pb->state);
    PUBNUB_LOG_TRACE("pbpal_check_tls(pb=%p)\n", pb);

    result = mbedtls_ssl_handshake(pb->pal.ssl);

    if (PNR_OK != (result = pbpal_handle_socket_condition(result, pb, __FILE__, __LINE__))) {
        PUBNUB_LOG_TRACE("pbpal_check_tls(pb=%p) result = %d\n", pb, result);
        return (result == PNR_IN_PROGRESS) ? pbtlsStarted : pbtlsFailed;
    }

    PUBNUB_LOG_DEBUG("TLS connection established\n");

    if ((0 != (tls_flags = mbedtls_ssl_get_verify_result(pb->pal.ssl)))) {
        mbedtls_x509_crt_verify_info(error_buf, sizeof error_buf, "  ! ", tls_flags);
        PUBNUB_LOG_ERROR("Certificate verification failed: %s\n", error_buf);

        return pbtlsFailed;
    }

    PUBNUB_LOG_INFO("TLS Certificate verification passed\n");
    PUBNUB_LOG_DEBUG("Cipher suite is %s\n", mbedtls_ssl_get_ciphersuite(pb->pal.ssl));

    return pbtlsEstablished;
}


#endif /* PUBNUB_USE_SSL */
