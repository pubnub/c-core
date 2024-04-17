#ifndef PUBNUB_PAL_H
#define PUBNUB_PAL_H

#include <mbedtls/ssl.h>
#include <mbedtls/net_sockets.h>


/** The Pubnub FreeRTOS context */
struct pubnub_pal {
    mbedtls_ssl_context* ssl;
    mbedtls_ssl_config* ssl_config;
    mbedtls_x509_crt* ca_certificates;
    mbedtls_net_context* net;
};

#endif /* PUBNUB_PAL_H */
