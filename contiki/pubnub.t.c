/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "cgreen/cgreen.h"
#include "cgreen/mocks.h"

#include "pubnub_contiki.h"

#include "contiki-net.h"

#include <stdlib.h>
#include <string.h>
#include <setjmp.h>


/* A less chatty cgreen :) */

#define attest assert_that
#define equals is_equal_to
#define streqs is_equal_to_string
#define differs is_not_equal_to
#define strdifs is_not_equal_to_string
#define ptreqs(val) is_equal_to_contents_of(&(val), sizeof(val))
#define ptrdifs(val) is_not_equal_to_contents_of(&(val), sizeof(val))
#define sets(par, val) will_set_contents_of_parameter(par, &(val), sizeof(val))
#define sets_ex will_set_contents_of_parameter
#define returns will_return


/* Contiki test harness (for our purposes) */

#if PROCESS_CONF_NO_PROCESS_NAMES
#define STUB_PROCESS(name, strname) struct process name = { NULL, NULL }
#else
#define STUB_PROCESS(name, strname) struct process name = { NULL, strname, NULL }
#endif

enum {
    RESOLV_EVENT_FOUND = PROCESS_EVENT_MAX,
    TCPIP_EVENT,
    MAX_STATIC_EVENT_ALLOC
};
process_event_t resolv_event_found = RESOLV_EVENT_FOUND;
process_event_t tcpip_event = TCPIP_EVENT;

/* Global vars from Contiki, don't need to be initialized, but should
   get the right value in tests that require them.
*/
uint8_t uip_flags;

//#define uip_mss()             (uip_conn->mss)

struct process *process_current;

void *uip_appdata;

//#define uip_newdata()   (uip_flags & UIP_NEWDATA)

uint16_t uip_len;

/* Global vars from Contiki that need initialization
 */
static struct uip_conn the_uip_conn;

struct uip_conn *uip_conn = &the_uip_conn;


STUB_PROCESS(resolv_process, "DNS resolver");


void uip_send(const void *data, int len)
{
    mock(data, len);
    /* Assume data is acknowledged right away.
       Not the right thing to do, but, should work for now. */
    uip_flags = UIP_ACKDATA;
}


struct uip_conn *tcp_connect(uip_ipaddr_t *ripaddr, uint16_t port, void *appstate)
{
    return (struct uip_conn*)mock(ripaddr, port, appstate);
}


void resolv_query(char const *name)
{
    mock(name);
}

resolv_status_t resolv_lookup(const char *name, uip_ipaddr_t ** ipaddr)
{
    printf("resolv_lookup(%s)\n", name);
    return (resolv_status_t)mock(name, ipaddr);
}



int process_post(struct process *p, process_event_t ev, void* data)
{
    return (int)mock(p, ev, data);
}


void process_start(struct process *p, const char *arg)
{
    mock(p, arg);
}


void _xassert(const char *file, int lineno)
{
  printf("Assertion failed: file %s, line %d.\n", file, lineno);
}


static bool m_expect_assert;
static jmp_buf m_assert_exp_jmpbuf;
static char const *m_expect_assert_file;


void assert_handler(char const *s, const char *file, long i)
{
//    mock(s, i);
    printf("Pubnub assert failed '%s', file '%s', line %d\n", s, file, i);
    
    attest(m_expect_assert);
    attest(m_expect_assert_file, streqs(file));
    m_expect_assert = false;
    longjmp(m_assert_exp_jmpbuf, 1);
}

#define expect_assert_in(expr, file) {          \
    m_expect_assert = true;                     \
    m_expect_assert_file = file;                \
    int val = setjmp(m_assert_exp_jmpbuf);      \
    if (0 == val) { expr; }                     \
    else { attest(!m_expect_assert); }          \
    }


/* Not mocked, but copied to avoid linking whole modules which would
   require other modules, which would require other modules, which
   would... you get the picture.
*/

uint16_t uip_htons(uint16_t val) { return UIP_HTONS(val); }

uint16_t psock_datalen(struct psock *p) { return p->bufsize - p->buf.left; }

int uiplib_ip4addrconv(const char *addrstr, uip_ip4addr_t *ipaddr)
{
    unsigned char i;
    uint8_t charsread = 0;
    unsigned char tmp = 0;
    
    for (i = 0; i < 4; ++i) {
        char c;
        unsigned char j = 0;
        do {
            c = *addrstr;
            if (++j > 4) {
                return 0;
            }
            if (c == '.' || c == 0 || c == ' ') {
                ipaddr->u8[i] = tmp;
                tmp = 0;
            } else if (c >= '0' && c <= '9') {
                tmp = (tmp * 10) + (c - '0');
            } else {
                return 0;
            }
            ++addrstr;
            ++charsread;
        } while (c != '.' && c != 0 && c != ' ');
    }
    return charsread-1;
}


/* These functions are also not (just) mocked, but not copied
   either. They're implemented differently, as per our needs.
*/

process_event_t process_alloc_event(void)
{
    static process_event_t lastevent = MAX_STATIC_EVENT_ALLOC;
    return lastevent++;
}


void tcp_attach(struct uip_conn *conn, void *appstate)
{
}


/* ---------- TESTS ---------- */


Ensure(can_get_context) {
    size_t i;
    pubnub_t *apb[PUBNUB_CTX_MAX];
    for (i = 0; i < PUBNUB_CTX_MAX; ++i) {
        apb[i] = pubnub_alloc();
        attest(apb[i], differs(NULL));
    }
    attest(pubnub_alloc(), equals(NULL));
    for (i = 0; i < PUBNUB_CTX_MAX; ++i) {
        attest(pubnub_free(apb[i]), equals(0));
    }
}


#define HTTP_PORT 80


Describe(single_context_pubnub);

static pubnub_t *pbp;
static uip_ipaddr_t pubnub_ip_addr;
static uip_ipaddr_t* pubnub_ip_addr_ptr = &pubnub_ip_addr;


#define WATCH(x, fmt) printf(#x " = " fmt "\n", x)


BeforeEach(single_context_pubnub) {
    pubnub_assert_set_handler(assert_handler);

    /* Default MSS, a MSS specific test can change it */
    the_uip_conn.mss = 536;

    pbp = pubnub_alloc();
    attest(pbp, differs(NULL));

    expect(process_start, when(p, equals(&resolv_process)));
    attest(pubnub_process.thread(&pubnub_process.pt, PROCESS_EVENT_INIT, NULL), equals(PT_YIELDED));

    process_current = &pubnub_process;
}

AfterEach(single_context_pubnub) {
    attest(pubnub_free(pbp), equals(0));
    attest(pubnub_process.thread(&pubnub_process.pt, PROCESS_EVENT_EXIT, NULL), equals(PT_ENDED));
}


void incoming(char const*str)
{
    printf("incoming(%s)\n", str);
    uip_appdata = (void*)str;
    uip_flags = UIP_NEWDATA;
    uip_len = strlen(str);
    attest(pubnub_process.thread(&pubnub_process.pt, TCPIP_EVENT, pbp), equals(PT_YIELDED));
}

/* For now, it seems it is pretty much impossible to detect if data
   has actually been read by the tested code. But, since tests already
   have this check, we're leaving it to see if we can figure it out.
*/
inline size_t readbuf_left(void) { return 0; }


inline void close_incoming() {
    uip_flags = UIP_CLOSE;
    attest(pubnub_process.thread(&pubnub_process.pt, TCPIP_EVENT, pbp), equals(PT_YIELDED));
}

inline void incoming_and_close(char const *str) {
    incoming(str);
    close_incoming();
}


void expect_cached_dns_for_pubnub_origin()
{
    expect(resolv_lookup, when(name, streqs(PUBNUB_ORIGIN)),
       sets(ipaddr, pubnub_ip_addr_ptr),
       returns(RESOLV_STATUS_CACHED));
    expect(tcp_connect,
       when(ripaddr, equals(pubnub_ip_addr_ptr)),
       when(port, equals(uip_htons(HTTP_PORT))),
       when(appstate, equals(pbp)));
}


#define expect_event(ev_)                       \
    expect(process_post,                        \
           when(p, equals(&pubnub_process)),    \
           when(ev, equals(ev_)),               \
           when(data, equals(pbp))              \
    )


inline void expect_outgoing_with_url(char const *url) {
    expect(uip_send, when(data, streqs("GET ")));
    expect(uip_send, when(data, streqs(url)));
    expect(uip_send, when(data, streqs(" HTTP/1.1\r\nHost: ")));
    expect(uip_send, when(data, streqs(PUBNUB_ORIGIN)));
    expect(uip_send, when(data, streqs("\r\nUser-Agent: PubNub-ConTiki/0.1\r\nConnection: Keep-Alive\r\n\r\n")));
    attest(pubnub_process.thread(&pubnub_process.pt, TCPIP_EVENT, pbp), equals(PT_YIELDED));
}


Ensure(single_context_pubnub, leave_cached_dns) {
    pubnub_init(pbp, "pubkey", "subkey");

    expect_cached_dns_for_pubnub_origin();
    attest(pubnub_leave(pbp, "lamanche"), equals(PNR_STARTED));

    uip_flags = UIP_CONNECTED;
    expect_outgoing_with_url("/v2/presence/sub-key/subkey/channel/lamanche/leave?");
    expect_event(pubnub_leave_event);
    incoming_and_close("HTTP/1.1 200\r\nContent-Length: 2\r\n\r\n[]");

    attest(readbuf_left(), equals(0));
    attest(pubnub_last_result(pbp), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));
}

Ensure(single_context_pubnub, leave_query_dns) {
    pubnub_init(pbp, "pubkey", "subskey");

    expect(resolv_lookup, when(name, streqs(PUBNUB_ORIGIN)),
       returns(RESOLV_STATUS_EXPIRED));
    expect(resolv_query, when(name, streqs(PUBNUB_ORIGIN)));
    attest(pubnub_leave(pbp, "dunav-tisa-dunav"), equals(PNR_STARTED));

    expect_cached_dns_for_pubnub_origin();
    attest(pubnub_process.thread(&pubnub_process.pt, RESOLV_EVENT_FOUND, PUBNUB_ORIGIN), equals(PT_YIELDED));

    uip_flags = UIP_CONNECTED;
    expect_outgoing_with_url("/v2/presence/sub-key/subskey/channel/dunav-tisa-dunav/leave?");
    expect_event(pubnub_leave_event);
    incoming_and_close("HTTP/1.1 200\r\nContent-Length: 2\r\n\r\n[]");

    attest(readbuf_left(), equals(0));
    attest(pubnub_last_result(pbp), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));
}


Ensure(single_context_pubnub, leave_cached_dns_uuid_auth) {
    pubnub_init(pbp, "pubkey", "subkey");

    /* Set UID */
    printf("****************** Set UID\n");
    pubnub_set_uuid(pbp, "BABA-DEDA-DECA");

    expect_cached_dns_for_pubnub_origin();
    attest(pubnub_leave(pbp, "lamanche"), equals(PNR_STARTED));

    uip_flags = UIP_CONNECTED;
    expect_outgoing_with_url("/v2/presence/sub-key/subkey/channel/lamanche/leave?uuid=BABA-DEDA-DECA");
    expect_event(pubnub_leave_event);
    incoming_and_close("HTTP/1.1 200\r\nContent-Length: 2\r\n\r\n[]");

    attest(readbuf_left(), equals(0));
    attest(pubnub_last_result(pbp), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));

    /* Set auth, too */
    printf("************ Set auth, too\n");
    pubnub_set_auth(pbp, "super-secret-key");

    expect_cached_dns_for_pubnub_origin();
    attest(pubnub_leave(pbp, "lamanche"), equals(PNR_STARTED));

    uip_flags = UIP_CONNECTED;
    expect_outgoing_with_url("/v2/presence/sub-key/subkey/channel/lamanche/leave?uuid=BABA-DEDA-DECA&auth=super-secret-key");
    expect_event(pubnub_leave_event);
    incoming_and_close("HTTP/1.1 200\r\nContent-Length: 2\r\n\r\n[]");

    attest(readbuf_left(), equals(0));
    attest(pubnub_last_result(pbp), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));

    /* Reset UUID */
    printf("************ Reset UUID\n");
    pubnub_set_uuid(pbp, NULL);

    expect_cached_dns_for_pubnub_origin();
    attest(pubnub_leave(pbp, "lamanche"), equals(PNR_STARTED));

    uip_flags = UIP_CONNECTED;
    expect_outgoing_with_url("/v2/presence/sub-key/subkey/channel/lamanche/leave?auth=super-secret-key");
    expect_event(pubnub_leave_event);
    incoming_and_close("HTTP/1.1 200\r\nContent-Length: 2\r\n\r\n[]");

    attest(readbuf_left(), equals(0));
    attest(pubnub_last_result(pbp), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));

    /* Reset auth, too */
    printf("************ Reset auth, too\n");
    pubnub_set_auth(pbp, NULL);

    expect_cached_dns_for_pubnub_origin();
    attest(pubnub_leave(pbp, "lamanche"), equals(PNR_STARTED));

    uip_flags = UIP_CONNECTED;
    expect_outgoing_with_url("/v2/presence/sub-key/subkey/channel/lamanche/leave?");

    expect_event(pubnub_leave_event);
    incoming_and_close("HTTP/1.1 200\r\nContent-Length: 2\r\n\r\n[]");

    attest(readbuf_left(), equals(0));
    attest(pubnub_last_result(pbp), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));
}


Ensure(single_context_pubnub, leave_cached_dns_cancel) {
    pubnub_init(pbp, "pubkey", "subkey");

    expect_cached_dns_for_pubnub_origin();
    attest(pubnub_leave(pbp, "lamanche"), equals(PNR_STARTED));

    expect_event(pubnub_leave_event);
    pubnub_cancel(pbp);
    attest(pubnub_process.thread(&pubnub_process.pt, TCPIP_EVENT, pbp), equals(PT_YIELDED));
    uip_flags = 0;
    attest(pubnub_process.thread(&pubnub_process.pt, TCPIP_EVENT, pbp), equals(PT_YIELDED));
    close_incoming();
    attest(pubnub_last_result(pbp), equals(PNR_CANCELLED));
}

Ensure(single_context_pubnub, leave_while_busy_fails) {
    pubnub_init(pbp, "pubkey", "subkey");

    /* Leave while leaving */
    expect_cached_dns_for_pubnub_origin();
    attest(pubnub_leave(pbp, "lamanche"), equals(PNR_STARTED));

    attest(pubnub_leave(pbp, "lamanche"), equals(PNR_IN_PROGRESS));

    expect_event(pubnub_leave_event);
    pubnub_cancel(pbp);
    close_incoming();
    close_incoming();
    attest(pubnub_last_result(pbp), equals(PNR_CANCELLED));
}


Ensure(single_context_pubnub, publish_cached_dns) {
    pubnub_init(pbp, "publkey", "subkey");

    expect_cached_dns_for_pubnub_origin();
    attest(pubnub_publish(pbp, "jarak", "\"zec\""), equals(PNR_STARTED));

    uip_flags = UIP_CONNECTED;
    expect_outgoing_with_url("/publish/publkey/subkey/0/jarak/0/%22zec%22");
    expect_event(pubnub_publish_event);
    incoming_and_close("HTTP/1.1 200\r\nContent-Length: 30\r\n\r\n[1,\"Sent\",\"14178940800777403\"]");

    attest(readbuf_left(), equals(0));
    attest(pubnub_last_result(pbp), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));
}


Ensure(single_context_pubnub, publish_while_busy_fails) {
    pubnub_init(pbp, "pubkey", "subkey");

    /* Leave while leaving */
    expect_cached_dns_for_pubnub_origin();
    attest(pubnub_leave(pbp, "lamanche"), equals(PNR_STARTED));

    attest(pubnub_publish(pbp, "lamanche", "123"), equals(PNR_IN_PROGRESS));

    expect_event(pubnub_leave_event);
    pubnub_cancel(pbp);
    close_incoming();
    close_incoming();
    attest(pubnub_last_result(pbp), equals(PNR_CANCELLED));
}


Ensure(single_context_pubnub, publish_cached_dns_too_long_message) {
    pubnub_init(pbp, "publkey", "subkey");

    char msg[PUBNUB_BUF_MAXLEN + 1];
    memset(msg, 'A', sizeof msg);
    msg[sizeof msg - 1] = '\0';
    attest(pubnub_publish(pbp, "w", msg), equals(PNR_TX_BUFF_TOO_SMALL));

    /* URL encoded char */
    memset(msg, '"', sizeof msg);
    msg[sizeof msg - 1] = '\0';
    attest(pubnub_publish(pbp, "w", msg), equals(PNR_TX_BUFF_TOO_SMALL));
}


Ensure(single_context_pubnub, subscribe_cached_dns) {
    pubnub_init(pbp, "publkey", "timok");

    expect_cached_dns_for_pubnub_origin();
    attest(pubnub_subscribe(pbp, "morava"), equals(PNR_STARTED));

    uip_flags = UIP_CONNECTED;
    expect_outgoing_with_url("/subscribe/timok/morava/0/0?&pnsdk=PubNub-Contiki-%2F1.1");
    expect_event(pubnub_subscribe_event);
    incoming_and_close("HTTP/1.1 200\r\nContent-Length: 33\r\n\r\n[[\"Hi\",\"Fi\"],\"14179836755957292\"]");

    attest(readbuf_left(), equals(0));
    attest(pubnub_last_result(pbp), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_get(pbp), streqs("\"Hi\""));
    attest(pubnub_get(pbp), streqs("\"Fi\""));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), equals(NULL));

    /* Response with channels */
    expect_cached_dns_for_pubnub_origin();
    attest(pubnub_subscribe(pbp, "morava,lim"), equals(PNR_STARTED));

    uip_flags = UIP_CONNECTED;
    expect_outgoing_with_url("/subscribe/timok/morava,lim/0/14179836755957292?&pnsdk=PubNub-Contiki-%2F1.1");

    expect_event(pubnub_subscribe_event);
    incoming_and_close("HTTP/1.1 200\r\nContent-Length: 63\r\n\r\n[[{\"Wi\"},[\"Xa\"],\"\\\"Qi\\\"\"],\"14179857817724547\",\"lim,morava,lim\"]");

    attest(readbuf_left(), equals(0));
    attest(pubnub_last_result(pbp), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_get(pbp), streqs("{\"Wi\"}"));
    attest(pubnub_get(pbp), streqs("[\"Xa\"]"));
    attest(pubnub_get(pbp), streqs("\"\\\"Qi\\\"\""));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), streqs("lim"));
    attest(pubnub_get_channel(pbp), streqs("morava"));
    attest(pubnub_get_channel(pbp), streqs("lim"));
    attest(pubnub_get_channel(pbp), equals(NULL));
}

Ensure(single_context_pubnub, subscribe_cached_dns_chunked) {
    pubnub_init(pbp, "publkey", "timok");

    expect_cached_dns_for_pubnub_origin();
    attest(pubnub_subscribe(pbp, "morava"), equals(PNR_STARTED));

    uip_flags = UIP_CONNECTED;
    expect_outgoing_with_url("/subscribe/timok/morava/0/0?&pnsdk=PubNub-Contiki-%2F1.1");

    incoming("HTTP/1.1 200\r\nTransfer-Encoding: chunked\r\n\r\n0d\r\n[[1234,\"Da\"],\r\n");
    expect_event(pubnub_subscribe_event);
    incoming_and_close("14\r\n\"14179915548467106\"]\r\n0\r\n");

    attest(readbuf_left(), equals(0));
    attest(pubnub_last_result(pbp), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_get(pbp), streqs("1234"));
    attest(pubnub_get(pbp), streqs("\"Da\""));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), equals(NULL));

    /* Now "unchunked" */
    expect_cached_dns_for_pubnub_origin();
    attest(pubnub_subscribe(pbp, "ravanica"), equals(PNR_STARTED));

    uip_flags = UIP_CONNECTED;
    expect_outgoing_with_url("/subscribe/timok/ravanica/0/14179915548467106?&pnsdk=PubNub-Contiki-%2F1.1");
    expect_event(pubnub_subscribe_event);
    incoming_and_close("HTTP/1.1 200\r\nContent-Length: 33\r\n\r\n[[\"Yo\",1098],\"14179916751973238\"]");

    attest(readbuf_left(), equals(0));
    attest(pubnub_last_result(pbp), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_get(pbp), streqs("\"Yo\""));
    attest(pubnub_get(pbp), streqs("1098"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), equals(NULL));
}


Ensure(single_context_pubnub, subscribed_cached_dns_uuid_auth) {
    pubnub_init(pbp, "pubkey", "timok");

    /* Set UID */
    pubnub_set_uuid(pbp, "CECA-CACA-DACA");

    expect_cached_dns_for_pubnub_origin();
    attest(pubnub_subscribe(pbp, "boka"), equals(PNR_STARTED));

    uip_flags = UIP_CONNECTED;
    expect_outgoing_with_url("/subscribe/timok/boka/0/0?uuid=CECA-CACA-DACA&pnsdk=PubNub-Contiki-%2F1.1");
    expect_event(pubnub_subscribe_event);
    incoming_and_close("HTTP/1.1 200\r\nContent-Length: 8\r\n\r\n[[],\"0\"]");
    attest(readbuf_left(), equals(0));
    attest(pubnub_last_result(pbp), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));

    /* Set auth, too */
    pubnub_set_auth(pbp, "public-key");

    expect_cached_dns_for_pubnub_origin();
    attest(pubnub_subscribe(pbp, "kotor"), equals(PNR_STARTED));

    uip_flags = UIP_CONNECTED;
    expect_outgoing_with_url("/subscribe/timok/kotor/0/0?uuid=CECA-CACA-DACA&auth=public-key&pnsdk=PubNub-Contiki-%2F1.1");
    expect_event(pubnub_subscribe_event);
    incoming_and_close("HTTP/1.1 200\r\nContent-Length: 8\r\n\r\n[[],\"0\"]");
    attest(readbuf_left(), equals(0));
    attest(pubnub_last_result(pbp), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));

    /* Reset UUID */
    pubnub_set_uuid(pbp, NULL);

    expect_cached_dns_for_pubnub_origin();
    attest(pubnub_subscribe(pbp, "sava"), equals(PNR_STARTED));

    uip_flags = UIP_CONNECTED;
    expect_outgoing_with_url("/subscribe/timok/sava/0/0?auth=public-key&pnsdk=PubNub-Contiki-%2F1.1");
    expect_event(pubnub_subscribe_event);
    incoming_and_close("HTTP/1.1 200\r\nContent-Length: 8\r\n\r\n[[],\"0\"]");
    attest(readbuf_left(), equals(0));
    attest(pubnub_last_result(pbp), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));

    /* Reset auth, too */
    pubnub_set_auth(pbp, NULL);

    expect_cached_dns_for_pubnub_origin();
    attest(pubnub_subscribe(pbp, "k"), equals(PNR_STARTED));

    uip_flags = UIP_CONNECTED;
    expect_outgoing_with_url("/subscribe/timok/k/0/0?&pnsdk=PubNub-Contiki-%2F1.1");
    expect_event(pubnub_subscribe_event);
    incoming_and_close("HTTP/1.1 200\r\nContent-Length: 8\r\n\r\n[[],\"0\"]");
    attest(readbuf_left(), equals(0));
    attest(pubnub_last_result(pbp), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));
}
//#endif

Ensure(single_context_pubnub, subscribe_while_busy_fails) {
    pubnub_init(pbp, "pubkey", "subkey");

    /* Leave while leaving */
    expect_cached_dns_for_pubnub_origin();
    attest(pubnub_publish(pbp, "nishava", "10987654321"), equals(PNR_STARTED));

    attest(pubnub_subscribe(pbp, "moravica"), equals(PNR_IN_PROGRESS));

    expect_event(pubnub_publish_event);
    pubnub_cancel(pbp);
    close_incoming();
    close_incoming();
    attest(pubnub_last_result(pbp), equals(PNR_CANCELLED));
}


Ensure(single_context_pubnub, subscribe_corner_cases) {
    pubnub_init(pbp, "publkey", "timok");

    /* Smallest regular response */
    expect_cached_dns_for_pubnub_origin();
    attest(pubnub_subscribe(pbp, "morava"), equals(PNR_STARTED));

    uip_flags = UIP_CONNECTED;
    expect_outgoing_with_url("/subscribe/timok/morava/0/0?&pnsdk=PubNub-Contiki-%2F1.1");
    expect_event(pubnub_subscribe_event);
    incoming_and_close("HTTP/1.1 200\r\nContent-Length: 9\r\n\r\n[[0],\"0\"]");

    attest(readbuf_left(), equals(0));
    attest(pubnub_last_result(pbp), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_get(pbp), streqs("0"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), equals(NULL));

    /* Largest regular response */
    expect_cached_dns_for_pubnub_origin();
    attest(pubnub_subscribe(pbp, "morava"), equals(PNR_STARTED));

    uip_flags = UIP_CONNECTED;
    expect_outgoing_with_url("/subscribe/timok/morava/0/0?&pnsdk=PubNub-Contiki-%2F1.1");
    expect_event(pubnub_subscribe_event);

    incoming("HTTP/1.1 200\r\nContent-Length: ");

#define PRELUDE "\r\n\r\n[[\""
#define INTERLUDE "\"],\"0\"]"
#define PACKETS 4
#define MAX_USEFUL (PUBNUB_REPLY_MAXLEN + 6 - sizeof PRELUDE -sizeof INTERLUDE)

    char *s = malloc(MAX_USEFUL + 1);
    attest(s, differs(NULL));
    snprintf(s, MAX_USEFUL, "%d", PUBNUB_REPLY_MAXLEN);
    printf("incoming content length: %d\n", PUBNUB_REPLY_MAXLEN);
    incoming(s);
    incoming(PRELUDE);
    size_t size_so_far = 0;
    int i;
    for (i = 0; i < PACKETS; ++i) {
        memset(s, 'X', MAX_USEFUL / PACKETS);
        s[MAX_USEFUL / PACKETS] = '\0';
        size_so_far += MAX_USEFUL / PACKETS;
        incoming(s);
    }
    if (size_so_far < MAX_USEFUL) {
        memset(s, 'X', MAX_USEFUL - size_so_far);
        s[MAX_USEFUL - size_so_far] = '\0';
        incoming(s);
    }
    free(s);
    incoming_and_close(INTERLUDE);
    
    attest(readbuf_left(), equals(0));
    attest(pubnub_last_result(pbp), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));

    s = malloc(MAX_USEFUL+2 + 1);
    attest(s, differs(NULL));
    s[0] = '"';
    s[MAX_USEFUL+1] = '"';
    s[MAX_USEFUL+2] = '\0';
    memset(s+1, 'X', MAX_USEFUL);
    attest(pubnub_get(pbp), streqs(s));
    free(s);

    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), equals(NULL));

#undef PRELUDE
#undef INTERLUDE
#undef PACKETS
#undef MAX_USEFUL
}


Ensure(single_context_pubnub, subscribe_bad_responses) {
    pubnub_init(pbp, "publkey", "timok");

    /* Bad HTTP version */
    expect_cached_dns_for_pubnub_origin();
    attest(pubnub_subscribe(pbp, "morava"), equals(PNR_STARTED));

    uip_flags = UIP_CONNECTED;
    expect_outgoing_with_url("/subscribe/timok/morava/0/0?&pnsdk=PubNub-Contiki-%2F1.1");
    expect_event(pubnub_subscribe_event);
    incoming_and_close("HTTP/0.9 200\r\nContent-Length: 33\r\n\r\n[[\"Hi\",\"Fi\"],\"14179836755957292\"]");

    attest(pubnub_last_result(pbp), equals(PNR_IO_ERROR));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), equals(NULL));

    /* Response body too long */
    expect_cached_dns_for_pubnub_origin();
    attest(pubnub_subscribe(pbp, "morava"), equals(PNR_STARTED));

    uip_flags = UIP_CONNECTED;
    expect_outgoing_with_url("/subscribe/timok/morava/0/0?&pnsdk=PubNub-Contiki-%2F1.1");
    expect_event(pubnub_subscribe_event);
    incoming_and_close("HTTP/1.1 200\r\nContent-Length: 3333\r\n\r\n");

    attest(pubnub_last_result(pbp), equals(PNR_IO_ERROR));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), equals(NULL));

    /* Response chunk too long */
    expect_cached_dns_for_pubnub_origin();
    attest(pubnub_subscribe(pbp, "morava"), equals(PNR_STARTED));

    uip_flags = UIP_CONNECTED;
    expect_outgoing_with_url("/subscribe/timok/morava/0/0?&pnsdk=PubNub-Contiki-%2F1.1");
    expect_event(pubnub_subscribe_event);
    incoming_and_close("HTTP/1.1 200\r\nTransfer-Encoding: chunked\r\n\r\nFFFF\r\n");

    attest(readbuf_left(), equals(0));
    attest(pubnub_last_result(pbp), equals(PNR_IO_ERROR));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), equals(NULL));

    /* Response chunk that goes "over the edge" */
    expect_cached_dns_for_pubnub_origin();
    attest(pubnub_subscribe(pbp, "morava"), equals(PNR_STARTED));

    uip_flags = UIP_CONNECTED;
    expect_outgoing_with_url("/subscribe/timok/morava/0/0?&pnsdk=PubNub-Contiki-%2F1.1");
    expect_event(pubnub_subscribe_event);

    incoming_and_close("HTTP/1.1 200\r\nTransfer-Encoding: chunked\r\n\r\nF0\r\n0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\r\nF0\r\n0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdef\r\nF0\r\n");

    attest(pubnub_last_result(pbp), equals(PNR_IO_ERROR));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), equals(NULL));
}


Ensure(single_context_pubnub, subscribe_bad_response_content) {
    pubnub_init(pbp, "publkey", "timok");

#define test_(incoming_) do { \
    expect_cached_dns_for_pubnub_origin(); \
    attest(pubnub_subscribe(pbp, "morava"), equals(PNR_STARTED)); \
    uip_flags = UIP_CONNECTED; \
    expect_outgoing_with_url("/subscribe/timok/morava/0/0?&pnsdk=PubNub-Contiki-%2F1.1"); \
    expect_event(pubnub_subscribe_event); \
    incoming_and_close("HTTP/1.0 200\r\nContent-Length: " incoming_); \
    attest(pubnub_last_result(pbp), equals(PNR_FORMAT_ERROR)); \
    attest(pubnub_get(pbp), equals(NULL)); \
    attest(pubnub_get_channel(pbp), equals(NULL)); \
    } while(0)

    /* Does not begin with [ */
    test_("33\r\n\r\nx[\"Hi\",\"Fi\"],\"14179836755957292\"]");

    /* Does not end with ] */
    test_("33\r\n\r\n[[\"Hi\",\"Fi\"],\"14179836755957292\"-");

    /* Last message array element does not end with " */
    test_("33\r\n\r\n[[\"Hi\",\"Fi\"],\"14179836755957292.]");

    /* No string beggining in the message (but has ending just before "]") */
    test_("33\r\n\r\n[[SHix,AFif],114179836755957292\"]");

    /* Too big time-token */
    test_("81\r\n\r\n[[1098,7654],\"0123456789abcdef0123456789abcdef0123456789abcdef0123456789abcdefX\"]");

    /* No string beggining for the "second from the right" in the message */
    test_("36\r\n\r\n[[123,456],114179836755957292\",\"ch\"]");

    /* Message empty  */
    test_("0\r\n\r\n");

    /* Too short message (just one character) */
    test_("1\r\n\r\nB");
    test_("1\r\n\r\n[");
    test_("1\r\n\r\n]");

    /* Too short message (just two characters) */
    test_("2\r\n\r\nBe");
    test_("2\r\n\r\n[,");
    test_("2\r\n\r\n,]");
    test_("2\r\n\r\n,\"");
    test_("2\r\n\r\n\"]");

    /* Too short message (just three characters) */
    test_("3\r\n\r\nBee");
    test_("3\r\n\r\n[,]");
    test_("3\r\n\r\n[,\"");
    test_("3\r\n\r\n,\"]");
    test_("3\r\n\r\n[\"]");

    /* Too short message (just four characters) */
    test_("4\r\n\r\nBeee");
    test_("4\r\n\r\n[,\"]");
    test_("4\r\n\r\n[,\"\"");
    test_("4\r\n\r\n[\",]");
    test_("4\r\n\r\n\",\"]");

    /* Too short message (just five characters) */
    test_("5\r\n\r\nBeeBe");
    test_("5\r\n\r\n[,\"\"]");
    test_("5\r\n\r\n[\",\"]");
    test_("5\r\n\r\n[\"\",]");
    test_("5\r\n\r\n[,,\"]");
    test_("5\r\n\r\n[,\",]");
    test_("5\r\n\r\n[\",,]");

    /* Bad JSON message: No ending brace */
    test_("33\r\n\r\nx[{\"Hi\",\"F\"],\"14179836755957292\"]");

    /* Bad JSON message: No ending bracket */
    test_("33\r\n\r\nx[{\"Hi\",\"F\"],\"14179836755957292\"]");

    /* Bad JSON message: No ending quote */
    test_("33\r\n\r\nx[[\"Hi\",\"F\"],\"14179836755957292\"]");

#undef test_
}


Ensure(single_context_pubnub, tcp_timeout) {
    pubnub_init(pbp, "publkey", "drina");

    /* Connection timeout */
    expect_cached_dns_for_pubnub_origin();
    attest(pubnub_subscribe(pbp, "morava"), equals(PNR_STARTED));

    uip_flags = UIP_TIMEDOUT;
    expect_event(pubnub_subscribe_event);
    attest(pubnub_process.thread(&pubnub_process.pt, TCPIP_EVENT, pbp), equals(PT_YIELDED));

    attest(pubnub_last_result(pbp), equals(PNR_TIMEOUT));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), equals(NULL));

    /* Timeout while connected */
    expect_cached_dns_for_pubnub_origin();
    attest(pubnub_subscribe(pbp, "morava"), equals(PNR_STARTED));

    uip_flags = UIP_CONNECTED;
    expect_outgoing_with_url("/subscribe/drina/morava/0/0?&pnsdk=PubNub-Contiki-%2F1.1");
    incoming("");

    
    uip_flags = UIP_TIMEDOUT;

    expect_event(pubnub_subscribe_event);
    attest(pubnub_process.thread(&pubnub_process.pt, TCPIP_EVENT, pbp), equals(PT_YIELDED));

    attest(pubnub_last_result(pbp), equals(PNR_TIMEOUT));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), equals(NULL));
}


Ensure(single_context_pubnub, tcp_abort) {
    pubnub_init(pbp, "publkey", "tisa");

    /* Abort while connecting */
    expect_cached_dns_for_pubnub_origin();
    attest(pubnub_subscribe(pbp, "morava"), equals(PNR_STARTED));

    uip_abort();

    expect_event(pubnub_subscribe_event);
    attest(pubnub_process.thread(&pubnub_process.pt, TCPIP_EVENT, pbp), equals(PT_YIELDED));

    attest(pubnub_last_result(pbp), equals(PNR_ABORTED));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), equals(NULL));

    /* Abort while connected */
    expect_cached_dns_for_pubnub_origin();
    attest(pubnub_subscribe(pbp, "tamish"), equals(PNR_STARTED));

    uip_flags = UIP_CONNECTED;
    expect_outgoing_with_url("/subscribe/tisa/tamish/0/0?&pnsdk=PubNub-Contiki-%2F1.1");
    incoming("");

    uip_abort();

    expect_event(pubnub_subscribe_event);
    attest(pubnub_process.thread(&pubnub_process.pt, TCPIP_EVENT, pbp), equals(PT_YIELDED));

    attest(pubnub_last_result(pbp), equals(PNR_ABORTED));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), equals(NULL));
}


Ensure(single_context_pubnub, tcp_closed) {
    pubnub_init(pbp, "kalauz", "drava");

    /* Connecting fails with close */
    expect_cached_dns_for_pubnub_origin();
    attest(pubnub_subscribe(pbp, "morava"), equals(PNR_STARTED));

    uip_close();

    expect_event(pubnub_subscribe_event);
    attest(pubnub_process.thread(&pubnub_process.pt, TCPIP_EVENT, pbp), equals(PT_YIELDED));

    attest(pubnub_last_result(pbp), equals(PNR_IO_ERROR));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), equals(NULL));

    /* Closed while connected */
    expect_cached_dns_for_pubnub_origin();
    attest(pubnub_subscribe(pbp, "tamish"), equals(PNR_STARTED));

    uip_flags = UIP_CONNECTED;
    expect_outgoing_with_url("/subscribe/drava/tamish/0/0?&pnsdk=PubNub-Contiki-%2F1.1");
    incoming("");

    uip_close();

    expect_event(pubnub_subscribe_event);
    attest(pubnub_process.thread(&pubnub_process.pt, TCPIP_EVENT, pbp), equals(PT_YIELDED));

    attest(pubnub_last_result(pbp), equals(PNR_IO_ERROR));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), equals(NULL));
}

Ensure(single_context_pubnub, tcp_data_ignored_while_connect) {
    pubnub_init(pbp, "kalauz", "drava");

    expect_cached_dns_for_pubnub_origin();
    attest(pubnub_subscribe(pbp, "morava"), equals(PNR_STARTED));

    incoming("");

    attest(pubnub_last_result(pbp), equals(PNR_STARTED));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), equals(NULL));

    /* We have to put the Netcore FSM to IDLE, so that we can free
       the context.
    */
    pubnub_cancel(pbp);
    uip_close();
    attest(pubnub_process.thread(&pubnub_process.pt, TCPIP_EVENT, pbp), equals(PT_YIELDED));
    expect_event(pubnub_subscribe_event);
    attest(pubnub_process.thread(&pubnub_process.pt, TCPIP_EVENT, pbp), equals(PT_YIELDED));
}


Ensure(single_context_pubnub, tcp_events_ignored_while_disconnect) {
    pubnub_init(pbp, "kalauz", "drava");

    expect_cached_dns_for_pubnub_origin();
    attest(pubnub_subscribe(pbp, "morava"), equals(PNR_STARTED));

    uip_flags = UIP_CONNECTED;
    expect_outgoing_with_url("/subscribe/drava/morava/0/0?&pnsdk=PubNub-Contiki-%2F1.1");

    incoming("HTTP/1.1 200\r\nContent-Length: 25\r\n\r\n[[0]");
    uip_flags = 0;
    attest(pubnub_process.thread(&pubnub_process.pt, TCPIP_EVENT, pbp), equals(PT_YIELDED));
    attest(pubnub_process.thread(&pubnub_process.pt, TCPIP_EVENT, pbp), equals(PT_YIELDED));

    attest(pubnub_last_result(pbp), equals(PNR_STARTED));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), equals(NULL));

    /* We have to put the Netcore FSM to IDLE, so that we can free
       the context.
    */
    pubnub_cancel(pbp);
    uip_close();
    attest(pubnub_process.thread(&pubnub_process.pt, TCPIP_EVENT, pbp), equals(PT_YIELDED));
    expect_event(pubnub_subscribe_event);
    attest(pubnub_process.thread(&pubnub_process.pt, TCPIP_EVENT, pbp), equals(PT_YIELDED));
}

Ensure(single_context_pubnub, cancel_before_connect) {
    pubnub_init(pbp, "sitnica", "tura");

    expect(resolv_lookup, when(name, streqs(PUBNUB_ORIGIN)),
       returns(RESOLV_STATUS_EXPIRED));
    expect(resolv_query, when(name, streqs(PUBNUB_ORIGIN)));
    attest(pubnub_subscribe(pbp, "morava"), equals(PNR_STARTED));

    expect_event(pubnub_subscribe_event);
    pubnub_cancel(pbp);
    attest(pubnub_last_result(pbp), equals(PNR_CANCELLED));
}


Ensure(single_context_pubnub, cancel_on_idle) {
    pubnub_init(pbp, "sitnica", "tura");

    pubnub_cancel(pbp);
}


Ensure(single_context_pubnub, cant_subscribe_until_messages_read) {
    pubnub_init(pbp, "sitnica", "tura");

    expect_cached_dns_for_pubnub_origin();
    attest(pubnub_subscribe(pbp, "dvapodva"), equals(PNR_STARTED));

    uip_flags = UIP_CONNECTED;
    expect_outgoing_with_url("/subscribe/tura/dvapodva/0/0?&pnsdk=PubNub-Contiki-%2F1.1");
    expect_event(pubnub_subscribe_event);
    incoming_and_close("HTTP/1.1 200\r\nContent-Length: 33\r\n\r\n[[\"Hi\",\"Fi\"],\"14179836755957292\"]");

    attest(readbuf_left(), equals(0));
    attest(pubnub_last_result(pbp), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_get_channel(pbp), equals(NULL));

    attest(pubnub_subscribe(pbp, "x"), equals(PNR_RX_BUFF_NOT_EMPTY));
}


Ensure(single_context_pubnub, illegal_context_fires_assert) {
    expect_assert_in(pubnub_init(NULL, "k", "u"), "pubnub_coreapi.c");
    expect_assert_in(pubnub_publish(NULL, "x", "0"), "pubnub_coreapi.c");
    expect_assert_in(pubnub_subscribe(NULL, "x"), "pubnub_coreapi.c");
    expect_assert_in(pubnub_leave(NULL, "x"), "pubnub_coreapi.c");
    expect_assert_in(pubnub_cancel(NULL), "pubnub_coreapi.c");
    expect_assert_in(pubnub_set_uuid(NULL, ""), "pubnub_coreapi.c");
    expect_assert_in(pubnub_set_auth(NULL, ""), "pubnub_coreapi.c");
    expect_assert_in(pubnub_last_result(NULL), "pubnub_ntf_contiki.c");
    expect_assert_in(pubnub_last_http_code(NULL), "pubnub_coreapi.c");
    expect_assert_in(pubnub_get(NULL), "pubnub_coreapi.c");
    expect_assert_in(pubnub_get_channel(NULL), "pubnub_coreapi.c");

    expect_assert_in(pubnub_free((pubnub_t*)((char*)pbp + 10000)), "pubnub_alloc_static.c");
}


Ensure(single_context_pubnub, null_events_ignored) {
    attest(pubnub_process.thread(&pubnub_process.pt, TCPIP_EVENT, NULL), equals(PT_YIELDED));
    attest(pubnub_process.thread(&pubnub_process.pt, resolv_event_found, NULL), equals(PT_YIELDED));
}


Ensure(single_context_pubnub, events_ignored_on_idle) {
    pubnub_init(pbp, "sitnica", "tura");

    attest(pubnub_process.thread(&pubnub_process.pt, TCPIP_EVENT, pbp), equals(PT_YIELDED));
    attest(pubnub_process.thread(&pubnub_process.pt, resolv_event_found, ""), equals(PT_YIELDED));

    expect(resolv_lookup, when(name, streqs(PUBNUB_ORIGIN)),
       returns(RESOLV_STATUS_EXPIRED));
    expect(resolv_query, when(name, streqs(PUBNUB_ORIGIN)));
    attest(pubnub_leave(pbp, "x"), equals(PNR_STARTED));
    attest(pubnub_process.thread(&pubnub_process.pt, TCPIP_EVENT, pbp), equals(PT_YIELDED));

    expect_event(pubnub_leave_event);
    pubnub_cancel(pbp);
    attest(pubnub_last_result(pbp), equals(PNR_CANCELLED));
}
