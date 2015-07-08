/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pbpal.h"

#include "pubnub_ntf_contiki.h"
#include "pubnub_netcore.h"
#include "pubnub_internal.h"
#include "pubnub_assert.h"

#include "contiki-net.h"

#include <string.h>


#if PUBNUB_USE_MDNS

#include "mdns.h"
#define pubnub_dns_init() mdns_init()
#define pubnub_dns_query mdns_query
#define pubnub_dns_lookup(name, pIPaddr) pIPaddr = mdns_lookup(name)
#define pubnub_dns_event mdns_event_found

#else

#define pubnub_dns_init() process_start(&resolv_process, NULL)
#define pubnub_dns_query resolv_query
#define pubnub_dns_lookup(name, pIPaddr) (resolv_lookup(PUBNUB_ORIGIN, &pIPaddr) == RESOLV_STATUS_CACHED) ? 0 : (pIPaddr = NULL)
#define pubnub_dns_event resolv_event_found

#endif


PROCESS(pubnub_process, "PubNub process");

#define HTTP_PORT 80


static void buf_setup(pubnub_t *pb)
{
    pb->ptr = (uint8_t*)pb->core.http_buf;
    pb->left = sizeof pb->core.http_buf;
}


void pbpal_init(pubnub_t *pb)
{
    pb->sock_state = STATE_NONE;
    pb->readlen = 0;
    buf_setup(pb);
}


int pbpal_resolv_and_connect(pubnub_t *pb)
{
    uip_ipaddr_t *ipaddrptr;
    
    PUBNUB_ASSERT(pb_valid_ctx_ptr(pb));
    PUBNUB_ASSERT_OPT((pb->state == PBS_IDLE) || (pb->state == PBS_WAIT_DNS));

    if (PBS_IDLE == pb->state) {
        pb->initiator = PROCESS_CURRENT();
    }
    pubnub_dns_lookup(PUBNUB_ORIGIN, ipaddrptr);
    if (NULL == ipaddrptr) {
        DEBUG_PRINTF("Pubnub: DNS Querying for %s\n", PUBNUB_ORIGIN);
        pubnub_dns_query(PUBNUB_ORIGIN);
        return -1;
    }
    
    PROCESS_CONTEXT_BEGIN(&pubnub_process);
    tcp_connect(ipaddrptr, uip_htons(HTTP_PORT), pb);
    PROCESS_CONTEXT_END(&pubnub_process);
    return +1;
}


int pbpal_send(pubnub_t *pb, void *data, size_t n)
{
    if (n == 0) {
        return 0;
    }
    if (pb->sock_state != STATE_NONE) {
        DEBUG_PRINTF("pbpal_send(): pb->sock_state != STATE_NONE (=%d)\n", pb->sock_state);
        return -1;
    }
    pb->sendptr = data;
    pb->sendlen = n;
    pb->sock_state = STATE_NONE;

    return pbpal_sent(pb) ? 0 : +1;
}


int pbpal_send_str(pubnub_t *pb, char *s)
{
    return pbpal_send(pb, s, strlen(s));
}


bool pbpal_sent(pubnub_t *pb)
{
    if (pb->sock_state != STATE_DATA_SENT || uip_rexmit()) {
        if (pb->sendlen > uip_mss()) {
            uip_send(pb->sendptr, uip_mss());
        }
        else {
            uip_send(pb->sendptr, pb->sendlen);
        }
        pb->sock_state = STATE_DATA_SENT;
        return false;
    } 
    else if (pb->sock_state == STATE_DATA_SENT && uip_acked()) {
        if (pb->sendlen > uip_mss()) {
            pb->sendlen -= uip_mss();
            pb->sendptr += uip_mss();
        } 
        else {
            pb->sendptr += pb->sendlen;
            pb->sendlen = 0;
        }
        if (pb->sendlen == 0) {
            pb->sock_state = STATE_NONE;
            return true;
        }
        else {
            pb->state = STATE_ACKED;
        }
    }
    return false;
}


static bool my_newdata(pubnub_t *pb)
{
    if (pb->readlen > 0) {
        /* There is data in the uip_appdata buffer that has not yet been
           read. */
        return true;
    } 
    else if ((pb->sock_state == STATE_READ) || (pb->sock_state == STATE_READ_LINE)) {
        /* All data in uip_appdata buffer already consumed. */
        pb->sock_state = STATE_BLOCKED_NEWDATA;
        return false;
    } 
    else if (uip_newdata()) {
        /* There is new data that has not been consumed. */
        return true;
    } 
    else {
        /* There is no new data. */
        return false;
    }
}


int pbpal_start_read_line(pubnub_t *pb)
{
    buf_setup(pb);

    if (pb->sock_state != STATE_NONE) {
        DEBUG_PRINTF("pbpal_start_read_line(): pb->sock_state != STATE_NONE: "); WATCH(pb->sock_state, "%d");
        return -1;
    }
    pb->sock_state = STATE_READ_LINE;
    //return pbpal_line_read(pb) ? 0 : +1;
    return +1;
}


bool pbpal_line_read(pubnub_t *pb)
{
    DEBUG_PRINTF("pbpal_line_read()\n");
    WATCH(pb->sock_state, "%d");
    if (pb->readlen == 0) {
        if (!my_newdata(pb)) {
            DEBUG_PRINTF("no newdata\n");
            return false;
        }
        DEBUG_PRINTF("have newdata of length=%d: %s\n", uip_datalen(), uip_appdata);
        pb->sock_state = STATE_READ_LINE;
        pb->readptr = (uint8_t *)uip_appdata;
        pb->readlen = uip_datalen();
    } 

    uint8_t c;
    while (pb->left > 0 && pb->readlen > 0) {
        c = *pb->ptr = *pb->readptr;
        ++pb->readptr;
        ++pb->ptr;

        --pb->readlen;
        --pb->left;
        
        if (c == '\n') {
            DEBUG_PRINTF("\\n found: "); WATCH(pbpal_read_len(pb), "%d");
            pb->sock_state = STATE_NONE;
            return true;
        }
    }

    if (pb->left == 0) {
        /* Buffer has been filled, but new-line char has not been
         * found.  We have to "reset" this "mini-fsm", as otherwise we
         * won't read anything any more. This means that we have lost
         * the current contents of the buffer, which is bad. In some
         * general code, that should be reported, as the caller could
         * save the contents of the buffer somewhere else or simply
         * decide to ignore this line (when it does end eventually).
         */
        pb->sock_state = STATE_NONE;
    }
    else {
        pb->sock_state = STATE_NEWDATA_EXHAUSTED;
    }

    return false;
}


int pbpal_read_len(pubnub_t *pb)
{
    return sizeof pb->core.http_buf - pb->left;
}


int pbpal_start_read(pubnub_t *pb, size_t n)
{
    buf_setup(pb);
    
    if (pb->sock_state != STATE_NONE) {
        DEBUG_PRINTF("pbpal_start_read(): pb->sock_state != STATE_NONE: "); WATCH(pb->sock_state, "%d");
        return -1;
    }
    pb->sock_state = STATE_READ;
    pb->len = n;
//    return pbpal_read_over(pb) ? 0 : +1;
    return +1;
}


bool pbpal_read_over(pubnub_t *pb)
{
    WATCH(pb->sock_state, "%d");
    WATCH(pb->readlen, "%d");
    WATCH(pb->left, "%d");
    WATCH(pb->len, "%d");

    if (pb->readlen == 0) {
        if (!my_newdata(pb)) {
            return false;
        }
        pb->sock_state = STATE_READ;
        pb->readptr = (uint8_t *)uip_appdata;
        pb->readlen = uip_datalen();
    } 

    uint16_t cp_len =  pb->len - pbpal_read_len(pb);
    if (cp_len > pb->readlen) {
        cp_len = pb->readlen;
    }
    if (cp_len > pb->left) {
        cp_len = pb->left;
    }
    DEBUG_PRINTF("About to copy %d bytes w/pb->readlen=%d\n", cp_len, pb->readlen);
    memcpy(pb->ptr, pb->readptr, cp_len);
    pb->ptr += cp_len;
    pb->left -= cp_len;
    pb->readptr += cp_len;
    pb->readlen -= cp_len;

    if (pbpal_read_len(pb) >= pb->len) {
        /* If we have read all that was requested, we're done. */
        DEBUG_PRINTF("Read all that was to be read.\n");
        pb->sock_state = STATE_NONE;
        return true;
    }

    if ((pb->left > 0)) {
        PUBNUB_ASSERT_OPT(pb->readlen == 0);
        if (pb->readlen == 0) {
            pb->sock_state = STATE_NEWDATA_EXHAUSTED;
        }
        return false;
    }

    /* Otherwise, we just filled the buffer, but we return 'true', to
     * enable the user to copy the data from the buffer to some other
     * storage.
     */
    DEBUG_PRINTF("Filled the buffer, but read %d and should %d\n", pbpal_read_len(pb), pb->len);
    pb->sock_state = STATE_NONE;
    return true;
}


static void handle_dns_found(char const* name)
{
    unsigned i;
    
    DEBUG_PRINTF("Pubnub: DNS event '%s' - origin: '%s'\n", name, PUBNUB_ORIGIN);
    
    if (0 != strcmp(name, PUBNUB_ORIGIN)) {
        return;
    }
    for (i = 0; i < PUBNUB_CTX_MAX; ++i) {
        pubnub_t *pb = pballoc_get_ctx(i);
        PUBNUB_ASSERT_OPT(pb != NULL);
        if (pb->state == PS_WAIT_DNS) {
            pbnc_fsm(pb);
        }
    }
}


static void handle_tcpip(pubnub_t *pb)
{
    DEBUG_PRINTF("handle_tcpip() pb->state=%d\n", pb->state);
    switch (pb->state) {
    case PBS_NULL:
    case PBS_IDLE:
    case PBS_WAIT_DNS:
        return;
    default:
        break;
    }

    if (uip_aborted()) {
        pbntf_trans_outcome(pb, PNR_ABORTED);
        return;
    }
    else if (uip_timedout()) {
        pbntf_trans_outcome(pb, PNR_TIMEOUT);
        return;
    }
    
    switch (pb->state) {
    case PBS_WAIT_CLOSE:
    case PBS_WAIT_CANCEL:
    case PBS_WAIT_CANCEL_CLOSE:
        pbnc_fsm(pb);
        break;
    default:
        if (uip_closed()) {
            DEBUG_PRINTF("if uip_closed() outcome PNR_IO_ERROR\n");
            WATCH(uip_flags, "%d");
            pbntf_trans_outcome(pb, PNR_IO_ERROR);
        }
        else {
            pbnc_fsm(pb);
        }
        break;
    }
}


PROCESS_THREAD(pubnub_process, ev, data)
{
    PROCESS_BEGIN();
    
    DEBUG_PRINTF("Pubnub: process started...\n");
    pubnub_publish_event = process_alloc_event();
    pubnub_subscribe_event = process_alloc_event();
    pubnub_time_event = process_alloc_event();
    pubnub_history_event = process_alloc_event();
    pubnub_presence_event = process_alloc_event();
    
    pubnub_dns_init();
    
    while (ev != PROCESS_EVENT_EXIT) {
        PROCESS_WAIT_EVENT();
        
        if (ev == tcpip_event) {
            if (data != NULL) {
                handle_tcpip(data);
            }            
        }
        else if (ev == pubnub_dns_event) {
            if (data != NULL) {
                handle_dns_found(data);
            }                
        }
    }
    
    PROCESS_END();
}


bool pbpal_closed(pubnub_t *pb)
{
    return uip_closed();
}


void pbpal_forget(pubnub_t *pb)
{
    tcp_markconn(uip_conn, NULL);
}


void pbpal_close(pubnub_t *pb)
{
    DEBUG_PRINTF("pbpal_close()\n");
    pb->readlen = 0;
    uip_close();
}


bool pbpal_connected(pubnub_t *pb)
{
    return uip_connected();
}
