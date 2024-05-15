#include "msstopwatch/msstopwatch.h"
#include "pubnub_internal.h"
#include "pubnub_log.h"

#if PUBNUB_USE_SSL

#include "pubnub_api_types.h"
#include "pubnub_internal.h"
#include "pubnub_internal_common.h"
#include "pubnub_pal.h"
#include "pubnub_assert.h"
#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#ifdef ESP_PLATFORM
#include "esp_crt_bundle.h"
#endif


static void pbntf_setup(void);
static void options_setup(pubnub_t* pb);
static void buffer_setup(pubnub_t* pb);


void pbpal_init(pubnub_t* pb)
{
    memset(&pb->pal, 0, sizeof pb->pal);

    pbntf_setup();
    options_setup(pb);
    buffer_setup(pb);
}


int pbpal_send(pubnub_t* pb, void const* data, size_t n)
{
    return 0;
}


int pbpal_send_str(pubnub_t* pb, char const* s)
{
    return pbpal_send(pb, s, strlen(s));
}


enum pubnub_res pbpal_handle_socket_condition(int result, pubnub_t* pb, char const* file, int line)
{
    if (pb->pal.ssl == NULL) {
        // TODO: use pbpal_handle_socket_error() here
        return -1;
    }

    PUBNUB_ASSERT(pb->options.useSSL);

    switch(result) {
        case 0: // success
            break;
        case MBEDTLS_ERR_SSL_WANT_READ:
        case MBEDTLS_ERR_SSL_WANT_WRITE:
            if (pbms_active(pb->pal.tryconn) // no field tryconn!?!?
                || (pbms_elapsed(pb->pal.tryconn) < pb->transaction_timeout_ms)) {
                PUBNUB_LOG_TRACE("pb=%p TLS/SSL_I/O operation should retry\n", pb);
                return PNR_IN_PROGRESS;
            }

            pb->pal.ip_timeout = 0; // it seems like a clue to the tryconn field

            // TODO: session if in pbpal_openssl.c
            
            PUBNUB_LOG_ERROR("pb=%p TLS/SSL_I/O operation failed, PNR_TIMEOUT\n", pb);

            return PNR_TIMEOUT;
        default:
            // TODO: error handling
            PUBNUB_LOG_ERROR("pb=%p TLS/SSL_I/O operation failed, PNR_IO_ERROR\n", pb);
            return PNR_IO_ERROR;
    }

    PUBNUB_LOG_TRACE("pb=%p TLS/SSL_I/O operation successful\n", pb);
    return PNR_OK;
}


int pbpal_send_status(pubnub_t* pb)
{
    return 0;
}


int pbpal_start_read_line(pubnub_t* pb)
{
    return 0;
}


enum pubnub_res pbpal_line_read_status(pubnub_t* pb)
{
    return PNR_OK;
}


int pbpal_read_len(pubnub_t* pb)
{
    return (char*)pb->ptr - pb->core.http_buf;
}


int pbpal_start_read(pubnub_t* pb, size_t n)
{
    return 0;
}


enum pubnub_res pbpal_read_status(pubnub_t* pb)
{
}


bool pbpal_closed(pubnub_t* pb)
{
    return (pb->pal.ssl == NULL);
}


void pbpal_forget(pubnub_t* pb)
{
    /* a no-op under OpenSSL */
}


int pbpal_close(pubnub_t* pb)
{
    return 0;
}


void pbpal_free(pubnub_t* pb)
{
}

static void pbntf_setup(void)
{
    static bool init_done = false;

    if (init_done) {
        return;
    }

    pbntf_init();
    init_done = true;
}


static void options_setup(pubnub_t* pb)
{
    pb->options.useSSL = true;
    pb->options.fallbackSSL = true;
    pb->options.use_system_certificate_store = false;
    pb->options.reuse_SSL_session = false;
    pb->flags.trySSL = false;
    pb->ssl_CAfile = NULL;
    pb->ssl_CApath = NULL; 
    pb->ssl_userPEMcert = NULL;
    pb->sock_state = STATE_NONE;
#if PUBNUB_USE_MULTIPLE_ADDRESSES
    pbpal_multiple_addresses_reset_counters(&pb->spare_addresses);
#endif

}


static void buffer_setup(pubnub_t* pb)
{
    pb->ptr  = (uint8_t*)pb->core.http_buf;
    pb->left = sizeof pb->core.http_buf / sizeof pb->core.http_buf[0];
}

#endif /* defined PUBNUB_USE_SSL */
