#ifndef PUBNUB_PAL_H
#define PUBNUB_PAL_H

#include "msstopwatch_pal.h"
#include <mbedtls/ssl.h>
#include <mbedtls/net_sockets.h>
#include <mbedtls/entropy.h>
#include <mbedtls/ctr_drbg.h>


/** The Pubnub FreeRTOS context */
struct pubnub_pal {
    mbedtls_ssl_context* ssl;
    mbedtls_ssl_config* ssl_config;
    mbedtls_x509_crt* ca_certificates;
    mbedtls_net_context* net;
    mbedtls_net_context* server_fd;
    mbedtls_entropy_context* entropy;
    mbedtls_ctr_drbg_context* ctr_drbg;
    pbmsref_t connection_timer;
    int socket;
};

#endif /* PUBNUB_PAL_H */
