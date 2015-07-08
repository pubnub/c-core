/* -*- c-file-style:"stroustrup" -*- */
#include "pubnub_contiki.h"

#include "contiki-net.h"
#include "dev/leds.h"

#include <stdio.h>
#include <string.h>

#if PUBNUB_USE_MDNS

#include "ip64.h"
#include "mdns.h"


/* If you need some configuration data, here's the
   place to put it
*/
static uip_ipaddr_t google_ipv4_dns_server = {
    .u8 = {
	/* Google's IPv4 DNS in mapped in an IPv6 address (::ffff:8.8.8.8) */
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0xff, 0xff,
	0x08, 0x08, 0x08, 0x08,
    }
};

#endif



static const char pubkey[] = "demo";
static const char subkey[] = "demo";
static const char channel[] = "hello_world";


PROCESS(pubnub_demo, "PubNub Demo process");
AUTOSTART_PROCESSES(&pubnub_demo, &pubnub_process);

struct pubnub *m_pb;


/** Helper function to translate Pubnub result to a string */
static char const* pubnub_res_2_string(enum pubnub_res e)
{
    switch (e) {
    case PNR_OK: return "OK";
    case PNR_TIMEOUT: return "Timeout";
    case PNR_IO_ERROR: return "I/O (communication) error";
    case PNR_HTTP_ERROR: return "HTTP error received from server";
    case PNR_FORMAT_ERROR: return "Response format error";
    case PNR_CANCELLED: return "Pubnub API transaction cancelled";
    case PNR_STARTED: return "Pubnub API transaction started";
    case PNR_IN_PROGRESS: return "Pubnub API transaction already in progress";
    case PNR_RX_BUFF_NOT_EMPTY: return "Rx buffer not empty";
    case PNR_TX_BUFF_TOO_SMALL:  return "Tx buffer too small for sending/publishing the message";
    default: return "!?!?!";
    }
}


PROCESS_THREAD(pubnub_demo, ev, data)
{
    static struct etimer et;
    PROCESS_BEGIN();

#if PUBNUB_USE_MDNS
    ip64_init();
    mdns_conf(&google_ipv4_dns_server);
#endif

    /* Get a context and initialize it */
    m_pb = pubnub_alloc();
    pubnub_init(m_pb, pubkey, subkey);
    
    etimer_set(&et, 5*CLOCK_SECOND);
    
    while (1) {
	PROCESS_WAIT_EVENT();
	if (ev == PROCESS_EVENT_TIMER) {
	    printf("pubnubDemo: Timer\n");
	    pubnub_publish(m_pb, channel, "\"ConTiki Pubnub voyager\"");
	}
	else if (ev == pubnub_publish_event) {
	    printf("pubnubDemo: Publish event: %s\n", pubnub_res_2_string(pubnub_last_result(m_pb)));
	    pubnub_subscribe(m_pb, channel);
	}
	else if (ev == pubnub_subscribe_event) {
	    printf("pubnubDemo: Subscribe event: %s\n", pubnub_res_2_string(pubnub_last_result(m_pb)));
	    for (;;) {
		char const *msg = pubnub_get(m_pb);
		if (NULL == msg) {
		    break;
		}
		leds_toggle(LEDS_ALL);
		printf("pubnubDemo: Received message: %s\n", msg);
	    }
	    etimer_restart(&et);
	}
    }

    PROCESS_END();
}
