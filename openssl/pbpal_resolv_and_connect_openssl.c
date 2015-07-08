/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pbpal.h"

#include "pubnub_internal.h"
#include "pubnub_assert.h"

#include <openssl/pem.h>
#include <openssl/ssl.h>



#define HTTP_PORT_STRING "80"


static int resolv_and_connect_wout_SSL(pubnub_t *pb)
{
    DEBUG_PRINTF("resolv_and_connect_wout_SSL\n");
    pb->pal.bio = BIO_new_connect(PUBNUB_ORIGIN ":" HTTP_PORT_STRING);
    if (NULL == pb->pal.bio) {
        return -1;
    }

    if (BIO_do_connect(pb->pal.bio) <= 0) {
        ERR_print_errors_fp(stderr);
        BIO_free_all(pb->pal.bio);
        return -1;
    }
    
    return pbntf_got_socket(pb, pb->pal.bio);
}


/* Starfield Inc (Class 2) Root certificate.
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


int pbpal_resolv_and_connect(pubnub_t *pb)
{
    SSL *ssl;

    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    PUBNUB_ASSERT_OPT((pb->state == PBS_IDLE) || (pb->state == PBS_WAIT_DNS));

    if (!pb->ssl.use) {
        return resolv_and_connect_wout_SSL(pb);
    }
    
    pb->pal.ctx = SSL_CTX_new(SSLv23_client_method());
    if (NULL == pb->pal.ctx) {
        ERR_print_errors_fp(stderr);
        DEBUG_PRINTF("SSL_CTX_new failed\n");
        return -1;
    }

    DEBUG_PRINTF("Got SSL_CTX\n");
    add_pubnub_cert(pb->pal.ctx);

    pb->pal.bio = BIO_new_ssl_connect(pb->pal.ctx);
    if (NULL == pb->pal.bio) {
        SSL_CTX_free(pb->pal.ctx);
        return -1;
    }

    DEBUG_PRINTF("Got BIO_new_ssl\n");

    BIO_get_ssl(pb->pal.bio, &ssl);
    SSL_set_mode(ssl, SSL_MODE_AUTO_RETRY); /* maybe not auto_retry? */

    BIO_set_conn_hostname(pb->pal.bio, PUBNUB_ORIGIN ":https");

    if (BIO_do_connect(pb->pal.bio) <= 0) {
        ERR_print_errors_fp(stderr);
        BIO_free_all(pb->pal.bio);
        SSL_CTX_free(pb->pal.ctx);
        return -1;
    }

    DEBUG_PRINTF("BIO connected\n");

    int rslt = SSL_get_verify_result(ssl);
    if (rslt != X509_V_OK) {
        DEBUG_PRINTF("SSL_get_verify_result() failed == %d(%s)\n", rslt, X509_verify_cert_error_string(rslt));
        ERR_print_errors_fp(stderr);
        BIO_free_all(pb->pal.bio);
        SSL_CTX_free(pb->pal.ctx);
        if (pb->ssl.fallback) {
            pb->pal.bio = NULL;
            pb->pal.ctx = NULL;
            return resolv_and_connect_wout_SSL(pb);
        }
        return -1;
    }
    
    return pbntf_got_socket(pb, pb->pal.bio);
}
