#include "contiki-net.h"
#include "dev/leds.h"


#include "ip64.h"
#include "mdns.h"

//#include "ip64-webserver.h"


#include "pubnub.h"

/*---------------------------------------------------------------------------*/
				
				#include <stdio.h>
				#include <string.h>
				#include "dev/button-sensor.h"

				#define DEBUG_PUBNUB  1
				#if DEBUG_PUBNUB == 1
				#define DBG(...) printf(__VA_ARGS__)
				#else
				#define DBG(...)
				#endif /* DEBUG */


				static uip_ipaddr_t google_ipv4_dns_server = {
					.u8 = {
						/* Google's IPv4 DNS in mapped in an IPv6 address (::ffff:8.8.8.8) */
						0x00, 0x00, 0x00, 0x00,
						0x00, 0x00, 0x00, 0x00,
						0x00, 0x00, 0xff, 0xff,
						0x08, 0x08, 0x08, 0x08,
					}
				};

				static const char pubkey[] = "demo";
				static const char subkey[] = "demo";
				static const char channel[] = "hello_world";


				PROCESS(pubnub_demo, "PubNub Demo process");

				struct pubnub *m_pb;

				PROCESS_THREAD(pubnub_demo, ev, data)
				{
					static struct etimer et;
					PROCESS_BEGIN();
					
					ip64_init();
					
					m_pb = pubnub_get_ctx(0);
					pubnub_init(m_pb, pubkey, subkey);
					
					    mdns_conf(&google_ipv4_dns_server);
					
					while (1) {
						etimer_set(&et, 5*CLOCK_SECOND);

						PROCESS_WAIT_EVENT();
						if (ev == PROCESS_EVENT_TIMER) {
							printf("pubnubDemo: Timer\n");
							pubnub_publish(m_pb, channel, "\"ConTiki Pubnub voyager\"");
						}
						else if (ev == pubnub_publish_event) {
							printf("pubnubDemo: Publish event\n");
							pubnub_subscribe(m_pb, channel);
						}
						else if (ev == pubnub_subscribe_event) {
							printf("pubnubDemo: Subscribe event\n");
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

PROCESS(router_node_process, "Router node");
#if 0
PROCESS(blinker_process, "Blinker process");
#endif

AUTOSTART_PROCESSES(&router_node_process
#if 0
,&blinker_process
#endif
,&pubnub_process
,&pubnub_demo
);
				
/*---------------------------------------------------------------------------*/
#if 0
PROCESS_THREAD(blinker_process, ev, data)
{
  static struct etimer et;
  static uint8_t red, green;
  PROCESS_BEGIN();

  etimer_set(&et, CLOCK_SECOND / 2);
  while(1) {
    PROCESS_WAIT_UNTIL(etimer_expired(&et));
    etimer_reset(&et);
    if(0) {
      leds_on(LEDS_RED);
      red = 1;
    } else {
      red = 0;
    }
  if(!ip64_hostaddr_is_configured()) {
      leds_on(LEDS_GREEN);
      green = 1;
    } else {
      green = 0;
    }
    PROCESS_WAIT_UNTIL(etimer_expired(&et));
    etimer_reset(&et);
    if(red) {
      leds_off(LEDS_RED);
    }
    if(green) {
      leds_off(LEDS_GREEN);
    }
  }
  PROCESS_END();
}
#endif
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(router_node_process, ev, data)
{
  PROCESS_BEGIN();

  /* Set us up as a RPL root node. */
  //simple_rpl_init_dag();

  /* Initialize the IP64 module so we'll start translating packets */
  //ip64_init();

  /* Initialize the IP64 webserver */
  //ip64_webserver_init();

  //NETSTACK_RDC.off(1);
  /* ... and do nothing more. */
  while(1) {
    PROCESS_WAIT_EVENT();
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
