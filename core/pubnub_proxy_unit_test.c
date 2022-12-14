/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
// TODO: try to merge that file with `pubnub_test_mocks`
#include "cgreen/cgreen.h"
#include "cgreen/mocks.h"

#include "pubnub_internal.h"
#include "pubnub_version_internal.h"
#include "pubnub_test_helper.h"
#include "pubnub_pubsubapi.h"
#include "pubnub_coreapi.h"
#include "pubnub_assert.h"
#include "pubnub_alloc.h"
#include "pubnub_log.h"

#include "pbpal.h"
#include "pubnub_keep_alive.h"
#include "pubnub_proxy.h"
#include "pubnub_json_parse.h"

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

/* Current (simulated) message received */
static char const *m_read;
/* Number of simulated messages received */
static int m_num_msgrcvd;
/* Array of simulated 'receive' messages */
static char *m_msg_array[20] = {NULL,};
/* Index(in the array) of the current message used while receiving*/
static int m_i;

/* Awaits given amount of time in seconds */
static void wait4it(time_t time_in_seconds) {
    time_t time_start = time(NULL);
    do{
    }while((time(NULL) - time_start) < time_in_seconds);
    return;
}



/* The Pubnub NTF mocks and stubs */
void pbntf_trans_outcome(pubnub_t *pb, enum pubnub_state state)
{
    pb->state = state;
    mock(pb);
}

int pbntf_got_socket(pubnub_t *pb)
{
    return (int)mock(pb);
}

void pbntf_lost_socket(pubnub_t *pb)
{
    mock(pb);
}

void pbntf_update_socket(pubnub_t *pb)
{
    mock(pb);
}

void pbntf_start_wait_connect_timer(pubnub_t* pb)
{
    /* This might be mocked at some point */
}

void pbntf_start_transaction_timer(pubnub_t* pb)
{
    /* This might be mocked at some point */
}

int pbntf_requeue_for_processing(pubnub_t *pb)
{
    return (int)mock(pb);
}

int pbntf_enqueue_for_processing(pubnub_t *pb)
{
    return (int)mock(pb);
}

int pbntf_watch_out_events(pubnub_t *pb)
{
    return (int)mock(pb);
}

int pbntf_watch_in_events(pubnub_t *pb)
{
    return (int)mock(pb);
}

/* The Pubnub PAL mocks and stubs */

static void buf_setup(pubnub_t *pb)
{
    pb->ptr = (uint8_t*)pb->core.http_buf;
    pb->left = sizeof pb->core.http_buf / sizeof pb->core.http_buf[0];
}

void pbpal_init(pubnub_t *pb)
{
    buf_setup(pb);
}

enum pbpal_resolv_n_connect_result pbpal_resolv_and_connect(pubnub_t *pb)
{
    return (int)mock(pb);
}

enum pbpal_resolv_n_connect_result pbpal_check_resolv_and_connect(pubnub_t *pb)
{
    return (int)mock(pb);
}

#if defined(PUBNUB_CALLBACK_API)
#if PUBNUB_CHANGE_DNS_SERVERS
int pbpal_dns_rotate_server(pubnub_t *pb)
{
    return (int)mock(pb);
}
#endif /* PUBNUB_CHANGE_DNS_SERVERS */
#endif /* defined(PUBNUB_CALLBACK_API) */

bool pbpal_connected(pubnub_t *pb)
{
    return (bool)mock(pb);
}

void pbpal_free(pubnub_t *pb)
{
    mock(pb);
}

enum pbpal_resolv_n_connect_result pbpal_check_connect(pubnub_t *pb)
{
    return (int)mock(pb);
}

int pbpal_send(pubnub_t *pb, void const* data, size_t n)
{
    return (int)mock(pb, data, n);
}

int pbpal_send_str(pubnub_t *pb, char const* s)
{
    return (int)mock(pb, s);
}


int pbpal_send_status(pubnub_t *pb)
{
    return (bool)mock(pb);
}

void pbpal_report_error_from_environment(pubnub_t* pb, char const* file, int line)
{
    mock(pb);
}

int pbpal_start_read_line(pubnub_t *pb)
{
    unsigned distance;

    PUBNUB_ASSERT_INT_OPT(pb->sock_state, ==, STATE_NONE);

    if (pb->unreadlen > 0) {
        PUBNUB_ASSERT_OPT((char*)(pb->ptr + pb->unreadlen) <= (char*)(pb->core.http_buf + PUBNUB_BUF_MAXLEN));
        memmove(pb->core.http_buf, pb->ptr, pb->unreadlen);
    }
    distance = pb->ptr - (uint8_t*)pb->core.http_buf;
    PUBNUB_ASSERT_UINT((distance + pb->left + pb->unreadlen), ==, (sizeof pb->core.http_buf / sizeof pb->core.http_buf[0]));
    pb->ptr -= distance;
    pb->left += distance;

    pb->sock_state = STATE_READ_LINE;

    return +1;
}

int pbpal_read_len(pubnub_t *pb)
{
    return (char*)pb->ptr - pb->core.http_buf;
}


static int my_recv(void *p, size_t n)
{
    int to_read;

    attest(m_read, differs(NULL));
    attest(m_num_msgrcvd, is_less_than(sizeof m_msg_array + 1));

    if (m_read[0] == '\0') {
        /* In case of receiving from current string
         * look for next(string) in the array of simulated
         * messages. So, effectivly, it's a game(play) with lengths of
         * messages and http_ buffer(within context) available
         */
        if(m_i < m_num_msgrcvd - 1) {
            m_read = m_msg_array[++m_i];
            attest(m_read, differs(NULL));
           
            if(strlen(m_read) == 0) {
                /* an empty string sent(server response simulation),
                 * through function 'incoming', 
                 * 'APIs' should interpret as:
                 * ('recv_from_platform_result' < 0) which gives
                 * the oportunity to simulate and test
                 * 'callback' interface in some degree.
                 * NOTE: first string in the array must not be empty
                 * string unless its ment to be the last
                 */
                return -1;
            }
        } else {
            /* If there is no more 'incoming' msgs and the last
             * caracter available is '\0' virtual platform simulates
             * 'connection closed - server side'(0 bytes received)
             */
            return 0;
        }
    }
    to_read = strlen(m_read);
    if (to_read > n) {
        to_read = n;
    }
    memcpy(p, m_read, to_read);
    m_read += to_read;
    return to_read;
}

enum pubnub_res pbpal_line_read_status(pubnub_t *pb)
{
    PUBNUB_ASSERT_OPT(STATE_READ_LINE == pb->sock_state);

    if (pb->unreadlen == 0) {
        int recvres;
        PUBNUB_ASSERT_OPT((char*)(pb->ptr + pb->left) == (char*)(pb->core.http_buf + PUBNUB_BUF_MAXLEN));
        recvres = my_recv((char*)pb->ptr, pb->left);
        if (recvres < 0) {
            return PNR_IN_PROGRESS;
        }
        else if (0 == recvres) {
            pb->sock_state = STATE_NONE;
            return PNR_TIMEOUT;
        }
        PUBNUB_ASSERT_OPT(recvres <= pb->left);
        PUBNUB_LOG_TRACE("pb=%p have new data of length=%d: %.*s\n", pb, recvres, recvres, pb->ptr);
        pb->unreadlen = recvres;
        pb->left -= recvres;
    }

    while (pb->unreadlen > 0) {
        uint8_t c = *pb->ptr++;
        --pb->unreadlen;

        if (c == '\n') {
            PUBNUB_LOG_TRACE("pb=%p, newline found, line length: %d, ", pb, pbpal_read_len(pb)); WATCH_USHORT(pb->unreadlen);
            pb->sock_state = STATE_NONE;
            return PNR_OK;
        }
    }

    if (pb->left == 0) {
        PUBNUB_LOG_ERROR("pbpal_line_read_status(pb=%p): buffer full but newline not found", pb);
        pb->sock_state = STATE_NONE;
        return PNR_TX_BUFF_TOO_SMALL;
    }

    return PNR_IN_PROGRESS;
}

int pbpal_start_read(pubnub_t *pb, size_t n)
{
    unsigned distance;

    PUBNUB_ASSERT_UINT_OPT(n, >, 0);
    PUBNUB_ASSERT_INT_OPT(pb->sock_state, ==, STATE_NONE);

    WATCH_USHORT(pb->unreadlen);
    WATCH_USHORT(pb->left);
    if (pb->unreadlen > 0) {
        PUBNUB_ASSERT_OPT((char*)(pb->ptr + pb->unreadlen) <= (char*)(pb->core.http_buf + PUBNUB_BUF_MAXLEN));
        memmove(pb->core.http_buf, pb->ptr, pb->unreadlen);
    }
    distance = pb->ptr - (uint8_t*)pb->core.http_buf;
    WATCH_UINT(distance);
    PUBNUB_ASSERT_UINT(distance + pb->unreadlen + pb->left, ==, sizeof pb->core.http_buf / sizeof pb->core.http_buf[0]);
    pb->ptr -= distance;
    pb->left += distance;

    pb->sock_state = STATE_READ;
    pb->len = n;

    return +1;
}

enum pubnub_res pbpal_read_status(pubnub_t *pb)
{
    int have_read;

    PUBNUB_ASSERT_OPT(STATE_READ == pb->sock_state);

    if (0 == pb->unreadlen) {
        unsigned to_recv = pb->len;
        if (to_recv > pb->left) {
            to_recv = pb->left;
        }
        PUBNUB_ASSERT_OPT(to_recv > 0 );
        have_read = my_recv((char*)pb->ptr, to_recv);
        if (have_read < 0) {
            return PNR_IN_PROGRESS;
        } 
        else if (0 == have_read) {
            pb->sock_state = STATE_NONE;
            return PNR_TIMEOUT;
        }
        PUBNUB_ASSERT_OPT(pb->left >= have_read);
        pb->left -= have_read;
    }
    else {
        have_read = (pb->unreadlen >= pb->len) ? pb->len : pb->unreadlen;
        pb->unreadlen -= have_read;
    }

    pb->len -= have_read;
    pb->ptr += have_read;

    if ((0 == pb->len) || (0 == pb->left)) {
        pb->sock_state = STATE_NONE;
        return PNR_OK;
    }

    return PNR_IN_PROGRESS;
}


bool pbpal_closed(pubnub_t *pb)
{
    return (bool)mock(pb);
}

void pbpal_forget(pubnub_t *pb)
{
    mock(pb);
}

int pbpal_close(pubnub_t *pb)
{
    pb->sock_state = STATE_NONE;
    return mock(pb);
}

#if PUBNUB_USE_MULTIPLE_ADDRESSES
void pbpal_multiple_addresses_reset_counters(struct pubnub_multi_addresses* spare_addresses)
{
    PUBNUB_UNUSED(spare_addresses);
}
#endif


/* The Pubnub version stubs */

char const *pubnub_sdk_name(void)
{
    return "unit-test";
}

char const *pubnub_version(void)
{
    return "0.1";
}

char const *pubnub_uname(void)
{
    return "unit-test-0.1";
}

char const* pubnub_uagent(void)
{
    return "POSIX-PubNub-C-core/" PUBNUB_SDK_VERSION;
}


/* Assert "catching" */
static bool m_expect_assert;
static jmp_buf m_assert_exp_jmpbuf;
static char const *m_expect_assert_file;


void assert_handler(char const *s, const char *file, long i)
{
    printf("%s:%ld: Pubnub assert failed '%s'\n", file, i, s);
    
    attest(m_expect_assert);
    attest(m_expect_assert_file, streqs(file));
    if (m_expect_assert) {
        m_expect_assert = false;
        longjmp(m_assert_exp_jmpbuf, 1);
    }
}

#define expect_assert_in(expr, file) {          \
    m_expect_assert = true;                     \
    m_expect_assert_file = file;                \
    int val = setjmp(m_assert_exp_jmpbuf);      \
    if (0 == val)  expr;                        \
    attest(!m_expect_assert);                   \
    }

Describe(single_context_pubnub);

static pubnub_t *pbp;


BeforeEach(single_context_pubnub) {
    pubnub_assert_set_handler((pubnub_assert_handler_t)assert_handler);
    m_read = NULL;
    m_num_msgrcvd = 0;
    m_i = 0;
    pbp = pubnub_alloc();
    attest(pbp, differs(NULL));
}

void free_m_msgs(char ** msg_array) {
    int i;

    attest(m_num_msgrcvd, is_less_than(sizeof m_msg_array + 1));

    for(i = 0; i < m_num_msgrcvd; i++) {
        attest(m_msg_array[i], differs(NULL));
        free(m_msg_array[i]);
    }
}

AfterEach(single_context_pubnub) {
    bool state_not_idle = (pbp->state != PBS_IDLE);
    /* pubnub_free() - callback environment behaviour */
    if (state_not_idle) {
        expect(pbntf_requeue_for_processing, when(pb, equals(pbp)));
        expect(pbpal_close, when(pb, equals(pbp)), returns(0));
        expect(pbpal_closed, when(pb, equals(pbp)), returns(true));
        expect(pbpal_forget, when(pb, equals(pbp)));
        expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    }
    expect(pbntf_requeue_for_processing, when(pb, equals(pbp)));
    if (state_not_idle) {
        attest(pubnub_free(pbp), equals(-1));
        attest(pbnc_fsm(pbp), equals(0));
        attest(pubnub_free(pbp), equals(0));
    }
    else {
        attest(pubnub_free(pbp), equals(0));
    }
    attest(pbp->state, equals(PBS_NULL));
    expect(pbpal_free, when(pb, equals(pbp)));
    /* pballoc_free_at_last(pb) is called from socket watcher thread */
    pballoc_free_at_last(pbp);
    free_m_msgs(m_msg_array);
}


void expect_have_dns_for_proxy_server(void)
{
    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbpal_resolv_and_connect, when(pb, equals(pbp)), returns(pbpal_connect_success));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
}

void expect_have_dns_for_proxy_server_without_enqueue_for_processing(void)
{
//    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbpal_resolv_and_connect, when(pb, equals(pbp)), returns(pbpal_connect_success));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
}

static inline void expect_first_outgoing_GET(const char* url)
{
    expect(pbpal_send_str, when(s, streqs("GET ")), returns(0));
    expect(pbpal_send_status, returns(0));
	expect(pbpal_send_str, when(s, streqs("http://")), returns(0));
    expect(pbpal_send_status, returns(0));
    expect(pbpal_send_str, when(s, streqs(PUBNUB_ORIGIN)), returns(0));
	expect(pbpal_send_status, returns(0));
	expect(pbpal_send_str, when(s, streqs(url)), returns(0));
    expect(pbpal_send_status, returns(0));
    expect(pbpal_send, when(data, streqs(" HTTP/1.1\r\nHost: ")), returns(0));
    expect(pbpal_send_status, returns(0));
    expect(pbpal_send_str, when(s, streqs(PUBNUB_ORIGIN)), returns(0));
    expect(pbpal_send_status, returns(0));
    expect(pbpal_send_str, 
           when(s, streqs("\r\nUser-Agent: POSIX-PubNub-C-core/" PUBNUB_SDK_VERSION "\r\n"
                          ACCEPT_ENCODING
                          "\r\n")),
           returns(0));
    expect(pbpal_send_status, returns(0));
    expect(pbntf_watch_in_events, when(pb, equals(pbp)), returns(0));
}

static inline void expect_first_outgoing_CONNECT(void)
{
    expect(pbpal_send, when(data, streqs("CONNECT ")), returns(0));
    expect(pbpal_send_status, returns(0));
    expect(pbpal_send_status, returns(0));
    expect(pbpal_send_str, when(s, streqs(PUBNUB_ORIGIN)), returns(0));
    expect(pbpal_send_status, returns(0));
    expect(pbpal_send_str, when(s, streqs(":80")), returns(0));
    expect(pbpal_send_status, returns(0));
    expect(pbpal_send, when(data, streqs(" HTTP/1.1\r\nHost: ")), returns(0));
    expect(pbpal_send_status, returns(0));
    expect(pbpal_send_str, when(s, streqs(PUBNUB_ORIGIN)), returns(0));
    expect(pbpal_send_status, returns(0));
    expect(pbpal_send_str, 
           when(s, streqs("\r\nUser-Agent: POSIX-PubNub-C-core/" PUBNUB_SDK_VERSION "\r\n"
                          ACCEPT_ENCODING
                          "\r\n")),
           returns(0));
    expect(pbpal_send_status, returns(0));
    expect(pbntf_watch_in_events, when(pb, equals(pbp)), returns(0));
}

static inline void expect_outgoing_with_encoded_credentials_GET(char const *url,
                                                                char const *HTTP_proxy_header)
{
    expect(pbpal_send_str, when(s, streqs("GET ")), returns(0));
    expect(pbpal_send_status, returns(0));
	expect(pbpal_send_str, when(s, streqs("http://")), returns(0));
    expect(pbpal_send_status, returns(0));
	expect(pbpal_send_str, when(s, streqs(PUBNUB_ORIGIN)), returns(0));
    expect(pbpal_send_status, returns(0));
    expect(pbpal_send_str, when(s, streqs(url)), returns(0));
    expect(pbpal_send_status, returns(0));
    expect(pbpal_send, when(data, streqs(" HTTP/1.1\r\nHost: ")), returns(0));
    expect(pbpal_send_status, returns(0));
    expect(pbpal_send_str, when(s, streqs(PUBNUB_ORIGIN)), returns(0));
    expect(pbpal_send_status, returns(0));
    expect(pbpal_send_str, when(s, streqs(HTTP_proxy_header)), returns(0));
    expect(pbpal_send_status, returns(0));
    expect(pbpal_send_str, 
           when(s, streqs("\r\nUser-Agent: POSIX-PubNub-C-core/" PUBNUB_SDK_VERSION "\r\n"
                          ACCEPT_ENCODING
                          "\r\n")),
           returns(0));
    expect(pbpal_send_status, returns(0));
    expect(pbntf_watch_in_events, when(pb, equals(pbp)), returns(0));
}

static inline void expect_outgoing_with_encoded_credentials_CONNECT(char const *url,
                                                                    char const *HTTP_proxy_header)
{
    expect(pbpal_send, when(data, streqs("CONNECT ")), returns(0));
    expect(pbpal_send_status, returns(0));
    expect(pbpal_send_status, returns(0));
	expect(pbpal_send_str, when(s, streqs(PUBNUB_ORIGIN)), returns(0));
    expect(pbpal_send_status, returns(0));
    expect(pbpal_send_str, when(s, streqs(":80")), returns(0));
    expect(pbpal_send_status, returns(0));
    expect(pbpal_send, when(data, streqs(" HTTP/1.1\r\nHost: ")), returns(0));
    expect(pbpal_send_status, returns(0));
    expect(pbpal_send_str, when(s, streqs(PUBNUB_ORIGIN)), returns(0));
    expect(pbpal_send_status, returns(0));
    expect(pbpal_send_str, when(s, streqs(HTTP_proxy_header)), returns(0));
    expect(pbpal_send_status, returns(0));
    expect(pbpal_send_str, 
           when(s, streqs("\r\nUser-Agent: POSIX-PubNub-C-core/" PUBNUB_SDK_VERSION "\r\n"
                          ACCEPT_ENCODING
                          "\r\n")),
           returns(0));
    expect(pbpal_send_status, returns(0));
    expect(pbntf_watch_in_events, when(pb, equals(pbp)), returns(0));
}

static inline void expect_outgoing_with_url(char const *url) {
    expect(pbpal_send_str, when(s, streqs("GET ")), returns(0));
    expect(pbpal_send_status, returns(0));
    expect(pbpal_send_status, returns(0));
    expect(pbpal_send_status, returns(0));
    expect(pbpal_send_str, when(s, streqs(url)), returns(0));
    expect(pbpal_send_status, returns(0));
    expect(pbpal_send, when(data, streqs(" HTTP/1.1\r\nHost: ")), returns(0));
    expect(pbpal_send_status, returns(0));
    expect(pbpal_send_str, when(s, streqs(PUBNUB_ORIGIN)), returns(0));
    expect(pbpal_send_status, returns(0));
    expect(pbpal_send_str, 
           when(s, streqs("\r\nUser-Agent: POSIX-PubNub-C-core/" PUBNUB_SDK_VERSION "\r\n"
                          ACCEPT_ENCODING
                          "\r\n")),
           returns(0));
    expect(pbpal_send_status, returns(0));
    expect(pbntf_watch_in_events, when(pb, equals(pbp)), returns(0));
}

static inline void incoming(char const *str) {
    char *pmsg = malloc(sizeof(char)*(strlen(str) + 1));
    
    attest(pmsg, differs(NULL));
    attest(m_num_msgrcvd, is_less_than(sizeof m_msg_array));

    strcpy(pmsg, str);
    m_msg_array[m_num_msgrcvd++] = pmsg;
    if(m_num_msgrcvd == 1) {
        m_read = pmsg;
    }
}


static inline void incoming_and_close(char const *str) {
    incoming(str);
    expect(pbpal_close, when(pb, equals(pbp)), returns(0));
//    expect(pbpal_closed, when(pb, equals(pbp)), returns(true));
    expect(pbpal_forget, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
}
#if 0
static void cancel_and_cleanup(pubnub_t *pbp)
{
    expect(pbpal_close, when(pb, equals(pbp)), returns(0));
    expect(pbpal_closed, when(pb, equals(pbp)), returns(true));
    expect(pbpal_forget, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    pubnub_cancel(pbp);
    attest(pbp->core.last_result, equals(PNR_CANCELLED));
} 
#endif


Ensure(single_context_pubnub, establishes_proxy_connection_GET_Basic)
{
    uint16_t proxy_port = 500;
    char proxy_hostname_to_long[PUBNUB_MAX_PROXY_HOSTNAME_LENGTH + 2];
    memset(proxy_hostname_to_long, 'A', sizeof proxy_hostname_to_long);
    proxy_hostname_to_long[sizeof proxy_hostname_to_long - 1] = '\0';

    pubnub_init(pbp, "publ-key", "sub-key");
    pubnub_set_user_id(pbp, "test_id");
    /* Some proxy_type(enum) not yet supported */
    attest(pubnub_set_proxy_manual(pbp, 5, "localhost", proxy_port), equals(-1));
    /* proxy hostname/url too long */
    attest(pubnub_set_proxy_manual(pbp, pbproxyHTTP_GET, proxy_hostname_to_long, proxy_port), equals(-1));
    /* parameters acceptable */
    attest(pubnub_set_proxy_manual(pbp, pbproxyHTTP_GET, "proxy_server_url", proxy_port), equals(0));
    attest(pubnub_set_proxy_authentication_username_password(pbp, "some_user", "some_password"), equals(0));
    expect_have_dns_for_proxy_server();
    expect_first_outgoing_GET("/subscribe/sub-key/colors/0/0?pnsdk=unit-test-0.1&uuid=test_id");
    incoming("HTTP/1.1 407 ProxyAuthentication Required\r\n"
             "Server: proxy_server.com\r\n"
             "Date: Mon, 5 Mar 2018 23:45:55 GMT\r\n"
             "Proxy-Authenticate: Basic\r\n"
			 "Content-Length: 169\r\n"
			 "\r\n"
			 "<!DOCTYPE html>\r\n"
			 "<html>\r\n"
			 " <head>\r\n"
			 "  <meta charset=\"UTF-8\" />\r\n"
			 "  <title>Error</title>\r\n"
			 " </head>\r\n"
			 " <body>\r\n"
			 "  <h1>407 ProxyAuthentication Required.</h1>\r\n"
			 " </body>\r\n"
			 "</html>\n");
    expect_outgoing_with_encoded_credentials_GET("/subscribe/sub-key/colors/0/0?pnsdk=unit-test-0.1&uuid=test_id",
                                                 "\r\nProxy-Authorization: Basic c29tZV91c2VyOnNvbWVfcGFzc3dvcmQ=");
    incoming("HTTP/1.1 200\r\n"
             "Server: proxy_server.com\r\n"
             "Date: Mon, 5 Mar 2018 23:45:57 GMT\r\n"
             "Content-Length: 26\r\n"
             "\r\n"
             "[[],\"1516014978925123457\"]");
    attest(pubnub_subscribe(pbp, "colors", NULL), equals(PNR_STARTED));

    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    /* Receiving buffer not very big */
    attest(pbnc_fsm(pbp), equals(0));

    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));

    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));

    expect_outgoing_with_encoded_credentials_GET("/subscribe/sub-key/colors/0/1516014978925123457?pnsdk=unit-test-0.1&uuid=test_id",
                                                 "\r\nProxy-Authorization: Basic c29tZV91c2VyOnNvbWVfcGFzc3dvcmQ=");
    incoming("HTTP/1.1 200\r\n"
             "Server: proxy_server.com\r\n"
             "Date: Mon, 5 Mar 2018 23:45:57 GMT\r\n"
             "Content-Length: 40\r\n"
             "\r\n"
             "[[red,green,blue],\"1516714978925123457\"]");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(pbp, "colors", NULL), equals(PNR_OK));

    attest(pubnub_last_http_code(pbp), equals(200));

    attest(pubnub_get(pbp), streqs("red"));
    attest(pubnub_get(pbp), streqs("green"));
    attest(pubnub_get(pbp), streqs("blue"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), streqs(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_proxy_protocol_get(pbp), equals(pbproxyHTTP_GET));
    
    /* Suddenly, we decided not to use proxy any more just for kicks :) */
    pubnub_set_proxy_none(pbp);
    attest(pubnub_proxy_protocol_get(pbp), equals(pbproxyNONE)); 
}


Ensure(single_context_pubnub, proxy_GET_Basic_client_sets_timeout_and_max_operation_count_for_keep_alive)
{
    uint16_t proxy_port = 500;
    time_t await_time;
    pubnub_init(pbp, "publ-key", "sub-key");
    pubnub_set_user_id(pbp, "test_id");

    attest(pubnub_set_proxy_manual(pbp, pbproxyHTTP_GET, "46.235.225.189", proxy_port), equals(0));
    attest(pubnub_set_proxy_authentication_username_password(pbp, "some_user", "some_password"), equals(0));
    /* Transactions' limits: up to whole second, 4 operations until client's connection closure */
    pubnub_set_keep_alive_param(pbp, 0, 4);

    expect_have_dns_for_proxy_server();
    expect_first_outgoing_GET("/subscribe/sub-key/colors/0/0?pnsdk=unit-test-0.1&uuid=test_id");
    incoming("HTTP/1.1 407 ProxyAuthentication Required\r\n"
             "Server: proxy_server.com\r\n"
             "Date: Mon, 5 Mar 2018 23:45:55 GMT\r\n"
             "Proxy-Authenticate: Basic\r\n"
			 "Content-Length: 169\r\n"
			 "\r\n"
			 "<!DOCTYPE html>\r\n"
			 "<html>\r\n"
			 " <head>\r\n"
			 "  <meta charset=\"UTF-8\" />\r\n"
			 "  <title>Error</title>\r\n"
			 " </head>\r\n"
			 " <body>\r\n"
			 "  <h1>407 ProxyAuthentication Required.</h1>\r\n"
			 " </body>\r\n"
			 "</html>\n");
    expect_outgoing_with_encoded_credentials_GET("/subscribe/sub-key/colors/0/0?pnsdk=unit-test-0.1&uuid=test_id",
                                                 "\r\nProxy-Authorization: Basic c29tZV91c2VyOnNvbWVfcGFzc3dvcmQ=");
    incoming("HTTP/1.1 200\r\n"
             "Server: proxy_server.com\r\n"
             "Date: Mon, 5 Mar 2018 23:45:57 GMT\r\n"
             "Content-Length: 26\r\n"
             "\r\n"
             "[[],\"1516014978925123457\"]");
    attest(pubnub_subscribe(pbp, "colors", NULL), equals(PNR_STARTED));

    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    /* Receiving buffer not very big */
    attest(pbnc_fsm(pbp), equals(0));
    /* Wait */
    await_time=0;
    wait4it(await_time);

    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));

    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_encoded_credentials_GET("/subscribe/sub-key/colors/0/1516014978925123457?pnsdk=unit-test-0.1&uuid=test_id",
                                                 "\r\nProxy-Authorization: Basic c29tZV91c2VyOnNvbWVfcGFzc3dvcmQ=");
    incoming("HTTP/1.1 200\r\n"
             "Server: proxy_server.com\r\n"
             "Date: Mon, 5 Mar 2018 23:45:57 GMT\r\n"
             "Content-Length: 40\r\n"
             "\r\n"
             "[[red,green,blue],\"1516714978925123457\"]");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(pbp, "colors", NULL), equals(PNR_OK));

    /* Wait */
//    await_time=0;
//    wait4it(await_time);

    attest(pubnub_last_http_code(pbp), equals(200));

    attest(pubnub_get(pbp), streqs("red"));
    attest(pubnub_get(pbp), streqs("green"));
    attest(pubnub_get(pbp), streqs("blue"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), streqs(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
    
    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_encoded_credentials_GET("/publish/publ-key/sub-key/0/jarak/0/%22zec%22?pnsdk=unit-test-0.1&uuid=test_id",
                                                 "\r\nProxy-Authorization: Basic c29tZV91c2VyOnNvbWVfcGFzc3dvcmQ=");
    incoming("HTTP/1.1 200\r\nContent-Length: 32\r\n\r\n[1,\"Sent\",\"1516714978925123458\"]");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_last_publish_result(pbp), streqs(""));
    attest(pubnub_publish(pbp, "jarak", "\"zec\""), equals(PNR_OK));
    attest(pubnub_last_publish_result(pbp), streqs("\"Sent\""));
    attest(pubnub_last_http_code(pbp), equals(200));

    expect(pbntf_got_socket, when(pb, equals(pbp)));
    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
//    expect(pbntf_lost_socket, when(pb, equals(pbp)));
//    expect_have_dns_for_proxy_server(); 
    expect_outgoing_with_encoded_credentials_GET("/subscribe/sub-key/jarak/0/1516714978925123457?pnsdk=unit-test-0.1&uuid=test_id",
                                                 "\r\nProxy-Authorization: Basic c29tZV91c2VyOnNvbWVfcGFzc3dvcmQ=");
    incoming_and_close("HTTP/1.1 200\r\n"
             "Server: proxy_server.com\r\n"
             "Date: Mon, 5 Mar 2018 23:45:57 GMT\r\n"
             "Content-Length: 40\r\n"
             "\r\n"
             "[[\"zec\",lav,mish],\"1516714978925123457\"]");
    attest(pubnub_subscribe(pbp, "jarak", NULL), equals(PNR_OK));

    attest(pubnub_get(pbp), streqs("\"zec\""));
    attest(pubnub_get(pbp), streqs("lav"));
    attest(pubnub_get(pbp), streqs("mish"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), streqs(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));

    /* History with time-token */
    expect_have_dns_for_proxy_server();
    expect_outgoing_with_encoded_credentials_GET("/v2/history/sub-key/sub-key/channel/ch?pnsdk=unit-test-0.1&uuid=test_id&count=22&include_token=true",
                                                 "\r\nProxy-Authorization: Basic c29tZV91c2VyOnNvbWVfcGFzc3dvcmQ=");
    incoming("HTTP/1.1 200\r\nContent-Length: 171\r\n\r\n[[{\"message\":1,\"timetoken\":14370863460777883},{\"message\":2,\"timetoken\":14370863461279046},{\"message\":3,\"timetoken\":14370863958459501}],14370863460777883,14370863958459501]");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_history(pbp, "ch", 22, true), equals(PNR_OK));

    attest(pubnub_get(pbp), streqs("[{\"message\":1,\"timetoken\":14370863460777883},{\"message\":2,\"timetoken\":14370863461279046},{\"message\":3,\"timetoken\":14370863958459501}]"));
    attest(pubnub_get(pbp), streqs("14370863460777883"));
    attest(pubnub_get(pbp), streqs("14370863958459501"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));

    attest(pubnub_proxy_protocol_get(pbp), equals(pbproxyHTTP_GET)); 
}


Ensure(single_context_pubnub, GET_Basic_proxy_closes_connection_after_first_407answer_client_keeps_folloving_protocol_until_established)
{
    uint16_t proxy_port = 500;
    pubnub_init(pbp, "publ-key", "sub-key");
    pubnub_set_user_id(pbp, "test_id");

    attest(pubnub_set_proxy_manual(pbp,
                                   pbproxyHTTP_GET,
                                   "2a00:1098::82:1000:3b:1:1",
                                   proxy_port),
           equals(0));
    attest(pubnub_set_proxy_authentication_username_password(pbp, "some_user", "some_password"), equals(0));
    expect_have_dns_for_proxy_server();
    expect_first_outgoing_GET("/subscribe/sub-key/colors/0/0?pnsdk=unit-test-0.1&uuid=test_id");
    incoming("HTTP/1.1 407 ProxyAuthentication Required\r\n"
             "Server: proxy_server.com\r\n"
             "Date: Mon, 5 Mar 2018 23:45:55 GMT\r\n"
             "Proxy-Authenticate: Basic\r\n"
             "Connection: close\r\n"
			 "Content-Length: 169\r\n"
			 "\r\n"
			 "<!DOCTYPE html>\r\n"
			 "<html>\r\n"
			 " <head>\r\n"
			 "  <meta charset=\"UTF-8\" />\r\n"
			 "  <title>Error</title>\r\n"
			 " </head>\r\n"
			 " <body>\r\n"
			 "  <h1>407 ProxyAuthentication Required.</h1>\r\n"
			 " </body>\r\n"
			 "</html>\n");
    expect(pbpal_close, when(pb, equals(pbp)), returns(0));
    expect_have_dns_for_proxy_server_without_enqueue_for_processing();
    expect_outgoing_with_encoded_credentials_GET("/subscribe/sub-key/colors/0/0?pnsdk=unit-test-0.1&uuid=test_id",
                                                 "\r\nProxy-Authorization: Basic c29tZV91c2VyOnNvbWVfcGFzc3dvcmQ=");
    incoming("HTTP/1.1 200\r\n"
             "Server: proxy_server.com\r\n"
             "Date: Mon, 5 Mar 2018 23:45:57 GMT\r\n"
             "Content-Length: 26\r\n"
             "\r\n"
             "[[],\"1516014978925123457\"]");
    attest(pubnub_subscribe(pbp, "colors", NULL), equals(PNR_STARTED));

    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    /* Receiving buffer not very big */
    attest(pbnc_fsm(pbp), equals(0));

    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));

    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));

    expect_outgoing_with_encoded_credentials_GET("/subscribe/sub-key/colors/0/1516014978925123457?pnsdk=unit-test-0.1&uuid=test_id",
                                                 "\r\nProxy-Authorization: Basic c29tZV91c2VyOnNvbWVfcGFzc3dvcmQ=");
    incoming("HTTP/1.1 200\r\n"
             "Server: proxy_server.com\r\n"
             "Date: Mon, 5 Mar 2018 23:45:57 GMT\r\n"
             "Content-Length: 40\r\n"
             "\r\n"
             "[[red,green,blue],\"1516714978925123457\"]");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(pbp, "colors", NULL), equals(PNR_OK));

    attest(pubnub_last_http_code(pbp), equals(200));

    attest(pubnub_get(pbp), streqs("red"));
    attest(pubnub_get(pbp), streqs("green"));
    attest(pubnub_get(pbp), streqs("blue"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), streqs(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
    
    /* Basic sheme - Proxy responds with 407 after credentials being sent.
       It is supposed that authentication is recognized as erroneous
     */
    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_encoded_credentials_GET("/subscribe/sub-key/colors/0/1516714978925123457?pnsdk=unit-test-0.1&uuid=test_id",
                                                 "\r\nProxy-Authorization: Basic c29tZV91c2VyOnNvbWVfcGFzc3dvcmQ=");
    incoming_and_close("HTTP/1.1 407 ProxyAuthentication Required\r\n"
                       "Server: proxy_server.com\r\n"
                       "Date: Mon, 5 Mar 2018 23:45:55 GMT\r\n"
                       "Proxy-Authenticate: Basic\r\n"
                       "Connection: close\r\n"
                       "Content-Length: 169\r\n"
                       "\r\n"
                       "<!DOCTYPE html>\r\n"
                       "<html>\r\n"
                       " <head>\r\n"
                       "  <meta charset=\"UTF-8\" />\r\n"
                       "  <title>Error</title>\r\n"
                       " </head>\r\n"
                       " <body>\r\n"
                       "  <h1>407 ProxyAuthentication Required.</h1>\r\n"
                       " </body>\r\n"
                       "</html>\n");
    attest(pubnub_subscribe(pbp, "colors", NULL), equals(PNR_AUTHENTICATION_FAILED));

    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(407));
}


Ensure(single_context_pubnub, establishes_proxy_connection_GET_Digest_and_continues_negotiating_after_stale_nounce)
{
    uint16_t port = 500;
#if __APPLE__
    char const* HTTP_proxy_header1 = "\r\nProxy-Authorization: Digest username=\"average_user\", "
                                     "realm=\"testrealm@host.com\", "
                                     "nonce=\"dcd98b7102dd2f0e8b11d0f600bfb0c093\", "
                                     "uri=\"/subscribe/sub-key/music/0/0?pnsdk=unit-test-0.1&uuid=test_id\", "
                                     "cnonce=\"a7410000f13a461099acb7602a0cb53a\", "
                                     "nc=\"00000001\", "
                                     "qop=\"auth-int\", "
                                     "response=\"b80c06ddeb4015ea4833e0ac86411db2\", "
                                     "opaque=\"5ccc069c403ebaf9f0171e9517f40e41\"";
    char const* HTTP_proxy_header2 = "\r\nProxy-Authorization: Digest username=\"average_user\", "
                                     "realm=\"testrealm@host.com\", "
                                     "nonce=\"dcd98b7102dd2f0e8b11d0f600bfb0c093\", "
                                     "uri=\"/subscribe/sub-key/music/0/1516014978925123457?pnsdk=unit-test-0.1&uuid=test_id\", "
                                     "cnonce=\"a7410000f13a461099acb7602a0cb53a\", "
                                     "nc=\"00000002\", "
                                     "qop=\"auth-int\", "
                                     "response=\"ed45d736ebe38d636a13d88e8d03ea02\", "
                                     "opaque=\"5ccc069c403ebaf9f0171e9517f40e41\"";
    char const* HTTP_proxy_header3 = "\r\nProxy-Authorization: Digest username=\"average_user\", "
                                     "realm=\"testrealm@host.com\", "
                                     "nonce=\"dcd98b7102dd2f0e8b11d0f600bfb0c093\", "
                                     "uri=\"/subscribe/sub-key/music/0/1516014978925123557?pnsdk=unit-test-0.1&uuid=test_id\", "
                                     "cnonce=\"a7410000f13a461099acb7602a0cb53a\", "
                                     "nc=\"00000003\", "
                                     "qop=\"auth-int\", "
                                     "response=\"4b142a4459a24b07f4f25f9f37234677\", "
                                     "opaque=\"5ccc069c403ebaf9f0171e9517f40e41\"";
    char const* HTTP_proxy_header4 = "\r\nProxy-Authorization: Digest username=\"average_user\", "
                                     "realm=\"testrealm@host.com\", "
                                     "nonce=\"dcd98b7102cc2f0e8b11d0f700bfb0c093\", "
                                     "uri=\"/subscribe/sub-key/music/0/1516014978925123557?pnsdk=unit-test-0.1&uuid=test_id\", "
                                     "cnonce=\"82b73144c8da461c988e0506fe09e556\", "
                                     "nc=\"00000001\", "
                                     "qop=\"auth-int\", "
                                     "response=\"899cdf71ec0945f17cc204298564991b\", "
                                     "opaque=\"5ccc069c403ebaf9f0171e9517f40e41\"";
#else
    char const* HTTP_proxy_header1 = "\r\nProxy-Authorization: Digest username=\"average_user\", "
                                     "realm=\"testrealm@host.com\", "
                                     "nonce=\"dcd98b7102dd2f0e8b11d0f600bfb0c093\", "
                                     "uri=\"/subscribe/sub-key/music/0/0?pnsdk=unit-test-0.1&uuid=test_id\", "
                                     "cnonce=\"67458b6bc6234b32a9983c6473483366\", "
                                     "nc=\"00000001\", "
                                     "qop=\"auth-int\", "
                                     "response=\"d2dc552a828b6ef185ea09007dbdb52d\", "
                                     "opaque=\"5ccc069c403ebaf9f0171e9517f40e41\"";
    char const* HTTP_proxy_header2 = "\r\nProxy-Authorization: Digest username=\"average_user\", "
                                     "realm=\"testrealm@host.com\", "
                                     "nonce=\"dcd98b7102dd2f0e8b11d0f600bfb0c093\", "
                                     "uri=\"/subscribe/sub-key/music/0/1516014978925123457?pnsdk=unit-test-0.1&uuid=test_id\", "
                                     "cnonce=\"67458b6bc6234b32a9983c6473483366\", "
                                     "nc=\"00000002\", "
                                     "qop=\"auth-int\", "
                                     "response=\"aae3ef4d1eeca71de4d1cdc5396adaed\", "
                                     "opaque=\"5ccc069c403ebaf9f0171e9517f40e41\"";
    char const* HTTP_proxy_header3 = "\r\nProxy-Authorization: Digest username=\"average_user\", "
                                     "realm=\"testrealm@host.com\", "
                                     "nonce=\"dcd98b7102dd2f0e8b11d0f600bfb0c093\", "
                                     "uri=\"/subscribe/sub-key/music/0/1516014978925123557?pnsdk=unit-test-0.1&uuid=test_id\", "
                                     "cnonce=\"67458b6bc6234b32a9983c6473483366\", "
                                     "nc=\"00000003\", "
                                     "qop=\"auth-int\", "
                                     "response=\"6ba0269934636b5b7bd117c960a540fb\", "
                                     "opaque=\"5ccc069c403ebaf9f0171e9517f40e41\"";
    char const* HTTP_proxy_header4 = "\r\nProxy-Authorization: Digest username=\"average_user\", "
                                     "realm=\"testrealm@host.com\", "
                                     "nonce=\"dcd98b7102cc2f0e8b11d0f700bfb0c093\", "
                                     "uri=\"/subscribe/sub-key/music/0/1516014978925123557?pnsdk=unit-test-0.1&uuid=test_id\", "
                                     "cnonce=\"51dcb074ff5c49198a94e82aec585562\", "
                                     "nc=\"00000001\", "
                                     "qop=\"auth-int\", "
                                     "response=\"7ccdc3723507e52e4bc0d6d52e975a9e\", "
                                     "opaque=\"5ccc069c403ebaf9f0171e9517f40e41\"";
#endif
    pubnub_init(pbp, "publ-key", "sub-key");
    pubnub_set_user_id(pbp, "test_id");

    attest(pubnub_set_proxy_manual(pbp, pbproxyHTTP_GET, "proxy.mythic-beasts.com", port), equals(0));
    attest(pubnub_set_proxy_authentication_username_password(pbp, "average_user", "password"), equals(0));
    expect_have_dns_for_proxy_server();
    expect_first_outgoing_GET("/subscribe/sub-key/music/0/0?pnsdk=unit-test-0.1&uuid=test_id");
    incoming("HTTP/1.0 407 ProxyAuthentication Required\r\n"
             "Server: proxy_server.com\r\n"
             "Date: Tue, 12 Mar 2018 19:02:30 GMT\r\n"
             "Proxy-Authenticate: Digest realm=\"testrealm@host.com\", "
                                        "qop=\"auth,auth-int\", "
                                        "nonce=\"dcd98b7102dd2f0e8b11d0f600bfb0c093\", "
                                        "opaque=\"5ccc069c403ebaf9f0171e9517f40e41\"\r\n"
             "Content-Type: text/html\r\n"
             "Content-Length: 153\r\n"
             "\r\n"
             "<!DOCTYPE html>\r\n"
             "<html>\r\n"
             " <head>\r\n"
             "  <meta charset=\"UTF-8\" />\r\n"
             "  <title>Error</title>\r\n"
             " </head>\r\n"
             " <body>\r\n"
             "  <h1>401 Unauthorized.</h1>\r\n"
             " </body>\r\n"
             "</html>\n");
    expect_outgoing_with_encoded_credentials_GET("/subscribe/sub-key/music/0/0?pnsdk=unit-test-0.1&uuid=test_id",
                                                 HTTP_proxy_header1);
    incoming("HTTP/1.1 200\r\n"
             "Server: proxy_server.com\r\n"
             "Date: Tue, 12 Mar 2018 19:02:32 GMT\r\n"
             "Content-Length: 26\r\n"
             "\r\n"
             "[[],\"1516014978925123457\"]");
    attest(pubnub_subscribe(pbp, "music", NULL), equals(PNR_STARTED));

    /* Pushing receiving messages until finished */
    attest(pbnc_fsm(pbp), equals(0));

    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pbnc_fsm(pbp), equals(0));

    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));

    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_encoded_credentials_GET(
        "/subscribe/sub-key/music/0/1516014978925123457?pnsdk=unit-test-0.1&uuid=test_id",
        HTTP_proxy_header2);
    incoming("HTTP/1.1 200\r\n"
             "Server: proxy_server.com\r\n"
             "Date: Tue, 12 Mar 2018 19:32:32 GMT\r\n"
             "Content-Length: 42\r\n"
             "\r\n"
             "[[pop,rock,classic],\"1516014978925123557\"]");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(pbp, "music", NULL), equals(PNR_OK));

    attest(pubnub_get(pbp), streqs("pop"));
    attest(pubnub_get(pbp), streqs("rock"));
    attest(pubnub_get(pbp), streqs("classic"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), streqs(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));

    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_encoded_credentials_GET(
        "/subscribe/sub-key/music/0/1516014978925123557?pnsdk=unit-test-0.1&uuid=test_id",
        HTTP_proxy_header3);
    incoming("HTTP/1.0 407 ProxyAuthentication Required\r\n"
             "Server: proxy_server.com\r\n"
             "Date: Tue, 12 Mar 2018 19:02:30 GMT\r\n"
             "Proxy-Authenticate: Digest realm=\"testrealm@host.com\", "
                                        "qop=\"auth,auth-int\", "
                                        "stale=TRUE, "
                                        "nonce=\"dcd98b7102cc2f0e8b11d0f700bfb0c093\", "
                                        "opaque=\"5ccc069c403ebaf9f0171e9517f40e41\"\r\n"
             "Content-Type: text/html\r\n"
             "Content-Length: 153\r\n"
             "\r\n"
             "<!DOCTYPE html>\r\n"
             "<html>\r\n"
             " <head>\r\n"
             "  <meta charset=\"UTF-8\" />\r\n"
             "  <title>Error</title>\r\n"
             " </head>\r\n"
             " <body>\r\n"
             "  <h1>401 Unauthorized.</h1>\r\n"
             " </body>\r\n"
             "</html>\n");
    expect_outgoing_with_encoded_credentials_GET(
        "/subscribe/sub-key/music/0/1516014978925123557?pnsdk=unit-test-0.1&uuid=test_id",
        HTTP_proxy_header4);
    incoming("HTTP/1.1 200\r\n"
             "Server: proxy_server.com\r\n"
             "Date: Tue, 12 Mar 2018 19:32:32 GMT\r\n"
             "Content-Length: 35\r\n"
             "\r\n"
             "[[folk,jazz],\"1516014978925123577\"]");
    attest(pubnub_subscribe(pbp, "music", NULL), equals(PNR_STARTED));
    /* Pushing receiving messages until finished */
    attest(pbnc_fsm(pbp), equals(0));

    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pbnc_fsm(pbp), equals(0));

    attest(pubnub_get(pbp), streqs("folk"));
    attest(pubnub_get(pbp), streqs("jazz"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), streqs(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_proxy_protocol_get(pbp), equals(pbproxyHTTP_GET)); 
}


Ensure(single_context_pubnub, GET_Digest_proxy_closes_connection_after407_and_stale_nounce_but_keeps_negotiating)
{
    uint16_t port = 500;
#if __APPLE__
    char const* HTTP_proxy_header1 = "\r\nProxy-Authorization: Digest username=\"average_user\", "
                                     "realm=\"testrealm@host.com\", "
                                     "nonce=\"dcd98b7102dd2f0e8b11d0f600bfb0c093\", "
                                     "uri=\"/subscribe/sub-key/music/0/0?pnsdk=unit-test-0.1&uuid=test_id\", "
                                     "cnonce=\"a7410000f13a461099acb7602a0cb53a\", "
                                     "nc=\"00000001\", "
                                     "qop=\"auth-int\", "
                                     "response=\"b80c06ddeb4015ea4833e0ac86411db2\", "
                                     "opaque=\"5ccc069c403ebaf9f0171e9517f40e41\"";
    char const* HTTP_proxy_header2 = "\r\nProxy-Authorization: Digest username=\"average_user\", "
                                     "realm=\"testrealm@host.com\", "
                                     "nonce=\"dcd98b7102dd2f0e8b11d0f600bfb0c093\", "
                                     "uri=\"/subscribe/sub-key/music/0/1516014978925123457?pnsdk=unit-test-0.1&uuid=test_id\", "
                                     "cnonce=\"a7410000f13a461099acb7602a0cb53a\", "
                                     "nc=\"00000002\", "
                                     "qop=\"auth-int\", "
                                     "response=\"ed45d736ebe38d636a13d88e8d03ea02\", "
                                     "opaque=\"5ccc069c403ebaf9f0171e9517f40e41\"";
    char const* HTTP_proxy_header3 = "\r\nProxy-Authorization: Digest username=\"average_user\", "
                                     "realm=\"testrealm@host.com\", "
                                     "nonce=\"dcd98b7102dd2f0e8b11d0f600bfb0c093\", "
                                     "uri=\"/subscribe/sub-key/music/0/1516014978925123557?pnsdk=unit-test-0.1&uuid=test_id\", "
                                     "cnonce=\"a7410000f13a461099acb7602a0cb53a\", "
                                     "nc=\"00000003\", "
                                     "qop=\"auth-int\", "
                                     "response=\"4b142a4459a24b07f4f25f9f37234677\", "
                                     "opaque=\"5ccc069c403ebaf9f0171e9517f40e41\"";
    char const* HTTP_proxy_header4 = "\r\nProxy-Authorization: Digest username=\"average_user\", "
                                     "realm=\"testrealm@host.com\", "
                                     "nonce=\"dcd98b7102cc2f0e8b11d0f700bfb0c093\", "
                                     "uri=\"/subscribe/sub-key/music/0/1516014978925123557?pnsdk=unit-test-0.1&uuid=test_id\", "
                                     "cnonce=\"82b73144c8da461c988e0506fe09e556\", "
                                     "nc=\"00000001\", "
                                     "qop=\"auth-int\", "
                                     "response=\"899cdf71ec0945f17cc204298564991b\", "
                                     "opaque=\"5ccc069c403ebaf9f0171e9517f40e41\"";
    char const* HTTP_proxy_header5 = "\r\nProxy-Authorization: Digest username=\"average_user\", "
                                     "realm=\"testrealm@host.com\", "
                                     "nonce=\"dcd98b7102cc2f0e8b11d0f700bfb0c093\", "
                                     "uri=\"/subscribe/sub-key/music/0/1516014978925123577?pnsdk=unit-test-0.1&uuid=test_id\", "
                                     "cnonce=\"82b73144c8da461c988e0506fe09e556\", "
                                     "nc=\"00000002\", "
                                     "qop=\"auth-int\", "
                                     "response=\"e468c901a3471d033a1ae508d5342b9e\", "
                                     "opaque=\"5ccc069c403ebaf9f0171e9517f40e41\"";
    char const* HTTP_proxy_header6 = "\r\nProxy-Authorization: Digest username=\"average_user\", "
                                     "realm=\"testrealm@host.com\", "
                                     "nonce=\"dcd98b7102cc2f0e8b11d0f700bfb0c093\", "
                                     "uri=\"/subscribe/sub-key/music/0/1516014978925123577?pnsdk=unit-test-0.1&uuid=test_id\", "
                                     "cnonce=\"432ff3564d04447798981631553c7c42\", "
                                     "nc=\"00000001\", "
                                     "qop=\"auth-int\", "
                                     "response=\"a9c6e45e15cec493be34f1a8326b5eb7\", "
                                     "opaque=\"5ccc069c403ebaf9f0171e9517f40e41\"";
#else
    char const* HTTP_proxy_header1 = "\r\nProxy-Authorization: Digest username=\"average_user\", "
                                     "realm=\"testrealm@host.com\", "
                                     "nonce=\"dcd98b7102dd2f0e8b11d0f600bfb0c093\", "
                                     "uri=\"/subscribe/sub-key/music/0/0?pnsdk=unit-test-0.1&uuid=test_id\", "
                                     "cnonce=\"67458b6bc6234b32a9983c6473483366\", "
                                     "nc=\"00000001\", "
                                     "qop=\"auth-int\", "
                                     "response=\"d2dc552a828b6ef185ea09007dbdb52d\", "
                                     "opaque=\"5ccc069c403ebaf9f0171e9517f40e41\"";
    char const* HTTP_proxy_header2 = "\r\nProxy-Authorization: Digest username=\"average_user\", "
                                     "realm=\"testrealm@host.com\", "
                                     "nonce=\"dcd98b7102dd2f0e8b11d0f600bfb0c093\", "
                                     "uri=\"/subscribe/sub-key/music/0/1516014978925123457?pnsdk=unit-test-0.1&uuid=test_id\", "
                                     "cnonce=\"67458b6bc6234b32a9983c6473483366\", "
                                     "nc=\"00000002\", "
                                     "qop=\"auth-int\", "
                                     "response=\"aae3ef4d1eeca71de4d1cdc5396adaed\", "
                                     "opaque=\"5ccc069c403ebaf9f0171e9517f40e41\"";
    char const* HTTP_proxy_header3 = "\r\nProxy-Authorization: Digest username=\"average_user\", "
                                     "realm=\"testrealm@host.com\", "
                                     "nonce=\"dcd98b7102dd2f0e8b11d0f600bfb0c093\", "
                                     "uri=\"/subscribe/sub-key/music/0/1516014978925123557?pnsdk=unit-test-0.1&uuid=test_id\", "
                                     "cnonce=\"67458b6bc6234b32a9983c6473483366\", "
                                     "nc=\"00000003\", "
                                     "qop=\"auth-int\", "
                                     "response=\"6ba0269934636b5b7bd117c960a540fb\", "
                                     "opaque=\"5ccc069c403ebaf9f0171e9517f40e41\"";
    char const* HTTP_proxy_header4 = "\r\nProxy-Authorization: Digest username=\"average_user\", "
                                     "realm=\"testrealm@host.com\", "
                                     "nonce=\"dcd98b7102cc2f0e8b11d0f700bfb0c093\", "
                                     "uri=\"/subscribe/sub-key/music/0/1516014978925123557?pnsdk=unit-test-0.1&uuid=test_id\", "
                                     "cnonce=\"51dcb074ff5c49198a94e82aec585562\", "
                                     "nc=\"00000001\", "
                                     "qop=\"auth-int\", "
                                     "response=\"7ccdc3723507e52e4bc0d6d52e975a9e\", "
                                     "opaque=\"5ccc069c403ebaf9f0171e9517f40e41\"";
    char const* HTTP_proxy_header5 = "\r\nProxy-Authorization: Digest username=\"average_user\", "
                                     "realm=\"testrealm@host.com\", "
                                     "nonce=\"dcd98b7102cc2f0e8b11d0f700bfb0c093\", "
                                     "uri=\"/subscribe/sub-key/music/0/1516014978925123577?pnsdk=unit-test-0.1&uuid=test_id\", "
                                     "cnonce=\"51dcb074ff5c49198a94e82aec585562\", "
                                     "nc=\"00000002\", "
                                     "qop=\"auth-int\", "
                                     "response=\"8cf6aece1771bc180aa2019233618fc0\", "
                                     "opaque=\"5ccc069c403ebaf9f0171e9517f40e41\"";
    char const* HTTP_proxy_header6 = "\r\nProxy-Authorization: Digest username=\"average_user\", "
                                     "realm=\"testrealm@host.com\", "
                                     "nonce=\"dcd98b7102cc2f0e8b11d0f700bfb0c093\", "
                                     "uri=\"/subscribe/sub-key/music/0/1516014978925123577?pnsdk=unit-test-0.1&uuid=test_id\", "
                                     "cnonce=\"291f8e23cd7c4846ba581b3dabd77e50\", "
                                     "nc=\"00000001\", "
                                     "qop=\"auth-int\", "
                                     "response=\"2364446a37c71152b6b0af414e70868e\", "
                                     "opaque=\"5ccc069c403ebaf9f0171e9517f40e41\"";
#endif
    pubnub_init(pbp, "publ-key", "sub-key");
    pubnub_set_user_id(pbp, "test_id");
    
    attest(pubnub_set_proxy_manual(pbp, pbproxyHTTP_GET, "proxy_server_url", port), equals(0));
    attest(pubnub_set_proxy_authentication_username_password(pbp, "average_user", "password"), equals(0));
    expect_have_dns_for_proxy_server();
    expect_first_outgoing_GET("/subscribe/sub-key/music/0/0?pnsdk=unit-test-0.1&uuid=test_id");
    incoming("HTTP/1.0 407 ProxyAuthentication Required\r\n"
             "Server: proxy_server.com\r\n"
             "Date: Tue, 12 Mar 2018 19:02:30 GMT\r\n"
             "Proxy-Authenticate: Digest realm=\"testrealm@host.com\", "
                                        "qop=\"auth,auth-int\", "
                                        "nonce=\"dcd98b7102dd2f0e8b11d0f600bfb0c093\", "
                                        "opaque=\"5ccc069c403ebaf9f0171e9517f40e41\"\r\n"
             "Connection: close\r\n"
             "Content-Type: text/html\r\n"
             "Content-Length: 153\r\n"
             "\r\n"
             "<!DOCTYPE html>\r\n"
             "<html>\r\n"
             " <head>\r\n"
             "  <meta charset=\"UTF-8\" />\r\n"
             "  <title>Error</title>\r\n"
             " </head>\r\n"
             " <body>\r\n"
             "  <h1>401 Unauthorized.</h1>\r\n"
             " </body>\r\n"
             "</html>\n");
    expect(pbpal_close, when(pb, equals(pbp)), returns(0));
    expect_have_dns_for_proxy_server_without_enqueue_for_processing();
    expect_outgoing_with_encoded_credentials_GET(
        "/subscribe/sub-key/music/0/0?pnsdk=unit-test-0.1&uuid=test_id",
        HTTP_proxy_header1);
    incoming("HTTP/1.1 200\r\n"
             "Server: proxy_server.com\r\n"
             "Date: Tue, 12 Mar 2018 19:02:32 GMT\r\n"
             "Content-Length: 26\r\n"
             "\r\n"
             "[[],\"1516014978925123457\"]");
    attest(pubnub_subscribe(pbp, "music", NULL), equals(PNR_STARTED));

    /* Pushing receiving messages until finished */
    attest(pbnc_fsm(pbp), equals(0));

    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pbnc_fsm(pbp), equals(0));

    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));

    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_encoded_credentials_GET(
        "/subscribe/sub-key/music/0/1516014978925123457?pnsdk=unit-test-0.1&uuid=test_id",
        HTTP_proxy_header2);
    incoming("HTTP/1.1 200\r\n"
             "Server: proxy_server.com\r\n"
             "Date: Tue, 12 Mar 2018 19:32:32 GMT\r\n"
             "Content-Length: 42\r\n"
             "\r\n"
             "[[pop,rock,classic],\"1516014978925123557\"]");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(pbp, "music", NULL), equals(PNR_OK));

    attest(pubnub_get(pbp), streqs("pop"));
    attest(pubnub_get(pbp), streqs("rock"));
    attest(pubnub_get(pbp), streqs("classic"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), streqs(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));

    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_encoded_credentials_GET(
        "/subscribe/sub-key/music/0/1516014978925123557?pnsdk=unit-test-0.1&uuid=test_id",
        HTTP_proxy_header3);
    incoming("HTTP/1.0 407 ProxyAuthentication Required\r\n"
             "Server: proxy_server.com\r\n"
             "Date: Tue, 12 Mar 2018 19:02:30 GMT\r\n"
             "Proxy-Authenticate: Digest realm=\"testrealm@host.com\", "
                                        "qop=\"auth,auth-int\", "
                                        "stale=TRUE, "
                                        "nonce=\"dcd98b7102cc2f0e8b11d0f700bfb0c093\", "
                                        "opaque=\"5ccc069c403ebaf9f0171e9517f40e41\"\r\n"
             "Connection: close\r\n"
             "Content-Type: text/html\r\n"
             "Content-Length: 153\r\n"
             "\r\n"
             "<!DOCTYPE html>\r\n"
             "<html>\r\n"
             " <head>\r\n"
             "  <meta charset=\"UTF-8\" />\r\n"
             "  <title>Error</title>\r\n"
             " </head>\r\n"
             " <body>\r\n"
             "  <h1>401 Unauthorized.</h1>\r\n"
             " </body>\r\n"
             "</html>\n");
    expect(pbpal_close, when(pb, equals(pbp)), returns(0));
    expect_have_dns_for_proxy_server_without_enqueue_for_processing();
    expect_outgoing_with_encoded_credentials_GET(
        "/subscribe/sub-key/music/0/1516014978925123557?pnsdk=unit-test-0.1&uuid=test_id",
        HTTP_proxy_header4);
    incoming("HTTP/1.1 200\r\n"
             "Server: proxy_server.com\r\n"
             "Date: Tue, 12 Mar 2018 19:32:32 GMT\r\n"
             "Content-Length: 35\r\n"
             "\r\n"
             "[[folk,jazz],\"1516014978925123577\"]");
    attest(pubnub_subscribe(pbp, "music", NULL), equals(PNR_STARTED));
    /* Pushing receiving messages until finished */
    attest(pbnc_fsm(pbp), equals(0));

    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pbnc_fsm(pbp), equals(0));

    attest(pubnub_get(pbp), streqs("folk"));
    attest(pubnub_get(pbp), streqs("jazz"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), streqs(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_proxy_protocol_get(pbp), equals(pbproxyHTTP_GET)); 

    /* Digest sheme - Sending repeated 'authentication required' messages on same realm
       within the same transaction should be considered eroneous
     */
    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_encoded_credentials_GET(
        "/subscribe/sub-key/music/0/1516014978925123577?pnsdk=unit-test-0.1&uuid=test_id",
        HTTP_proxy_header5);
    incoming("HTTP/1.0 407 ProxyAuthentication Required\r\n"
             "Server: proxy_server.com\r\n"
             "Date: Tue, 12 Mar 2018 19:42:31 GMT\r\n"
             "Proxy-Authenticate: Digest realm=\"testrealm@host.com\", "
                                        "qop=\"auth,auth-int\", "
                                        "nonce=\"dcd98b7102cc2f0e8b11d0f700bfb0c093\", "
                                        "opaque=\"5ccc069c403ebaf9f0171e9517f40e41\"\r\n"
             "Connection: keep-alive\r\n"
             "Content-Type: text/html\r\n"
             "Content-Length: 153\r\n"
             "\r\n"
             "<!DOCTYPE html>\r\n"
             "<html>\r\n"
             " <head>\r\n"
             "  <meta charset=\"UTF-8\" />\r\n"
             "  <title>Error</title>\r\n"
             " </head>\r\n"
             " <body>\r\n"
             "  <h1>401 Unauthorized.</h1>\r\n"
             " </body>\r\n"
             "</html>\n");
    expect_outgoing_with_encoded_credentials_GET(
        "/subscribe/sub-key/music/0/1516014978925123577?pnsdk=unit-test-0.1&uuid=test_id",
        HTTP_proxy_header6);
    incoming_and_close("HTTP/1.0 407 ProxyAuthentication Required\r\n"
                       "Server: proxy_server.com\r\n"
                       "Date: Tue, 12 Mar 2018 19:42:31 GMT\r\n"
                       "Proxy-Authenticate: Digest realm=\"testrealm@host.com\", "
                                                  "qop=\"auth,auth-int\", "
                                                  "nonce=\"dcd98b7102cc2f0e8b11d0f700bfb0c093\", "
                                                  "opaque=\"5ccc069c403ebaf9f0171e9517f40e41\"\r\n"
                       "Connection: keep-alive\r\n"
                       "Content-Type: text/html\r\n"
                       "Content-Length: 153\r\n"
                       "\r\n"
                       "<!DOCTYPE html>\r\n"
                       "<html>\r\n"
                       " <head>\r\n"
                       "  <meta charset=\"UTF-8\" />\r\n"
                       "  <title>Error</title>\r\n"
                       " </head>\r\n"
                       " <body>\r\n"
                       "  <h1>401 Unauthorized.</h1>\r\n"
                       " </body>\r\n"
                       "</html>\n");
    attest(pubnub_subscribe(pbp, "music", NULL), equals(PNR_STARTED));
    attest(pbnc_fsm(pbp), equals(0));
    attest(pbnc_fsm(pbp), equals(0));
    attest(pbnc_fsm(pbp), equals(0));
    attest(pbp->core.last_result, equals(PNR_AUTHENTICATION_FAILED));
}


Ensure(single_context_pubnub, try_to_establish_proxy_connection_GET_No_response)
{
    uint16_t port = 500;
    pubnub_init(pbp, "publ-key", "sub-key");
    pubnub_set_user_id(pbp, "test_id");

    attest(pubnub_set_proxy_manual(pbp, pbproxyHTTP_GET, "proxy_server_url", port), equals(0));
    attest(pubnub_set_proxy_authentication_username_password(pbp, "average_user", "password"), equals(0));
    expect_have_dns_for_proxy_server();
    expect_first_outgoing_GET("/subscribe/sub-key/music/0/0?pnsdk=unit-test-0.1&uuid=test_id");
    /* First incoming string is empty - 'resv' returns 0 - server closed connection from some reason.
       (Assumed connection timeout) */ 
    incoming_and_close("");
    attest(pubnub_subscribe(pbp, "music", NULL), equals(PNR_TIMEOUT));
}


Ensure(single_context_pubnub, try_to_establish_proxy_connection_GET_unsupported_proxy_authentication)
{
    uint16_t port = 500;
    pubnub_init(pbp, "publ-key", "sub-key");
    pubnub_set_user_id(pbp, "test_id");

    attest(pubnub_set_proxy_manual(pbp, pbproxyHTTP_GET, "proxy_server_url", port), equals(0));
    attest(pubnub_set_proxy_authentication_username_password(pbp, "average_user", "password"), equals(0));
    expect_have_dns_for_proxy_server();
    expect_first_outgoing_GET("/subscribe/sub-key/music/0/0?pnsdk=unit-test-0.1&uuid=test_id");
 	incoming_and_close("HTTP/1.1 407 ProxyAuthentication Required ( The ISA Server requires authorization to fulfill the request. Access to the Web proxy service is denied. )\r\n" 
			 "Via: 1.1 SPIRIT1B\r\n"
			 "Proxy-Authenticate: Negotiate\r\n"
		     "Proxy-Authenticate: Kerberos\r\n"
		     "Proxy-Connection: keep-alive\r\n"
		     "Pragma: no-cache\r\n"
		     "Cache-Control: no-cache\r\n"
		     "Content-Type: text/html\r\n"
			 "Content-Length: 169\r\n"
			 "\r\n"
			 "<!DOCTYPE html>\r\n"
			 "<html>\r\n"
			 " <head>\r\n"
			 "  <meta charset=\"UTF-8\" />\r\n"
			 "  <title>Error</title>\r\n"
			 " </head>\r\n"
			 " <body>\r\n"
			 "  <h1>407 ProxyAuthentication Required.</h1>\r\n"
			 " </body>\r\n"
			 "</html>\n");
    attest(pubnub_subscribe(pbp, "music", NULL), equals(PNR_STARTED));

    attest(pbnc_fsm(pbp), equals(0));
    attest(pubnub_proxy_protocol_get(pbp), equals(pbproxyHTTP_GET)); 
    attest(pbnc_fsm(pbp), equals(0));
    attest(pbp->core.last_result, equals(PNR_HTTP_ERROR));
}


Ensure(single_context_pubnub, establishes_proxy_connection_CONNECT_Basic)
{
    uint16_t proxy_port = 500;
    pubnub_init(pbp, "publ-key", "sub-key");
    pubnub_set_user_id(pbp, "test_id");

    attest(pubnub_set_proxy_manual(pbp, pbproxyHTTP_CONNECT, "proxy_server_url", proxy_port), equals(0));
    attest(pubnub_set_proxy_authentication_username_password(pbp, "serious", "password"), equals(0));

    expect_have_dns_for_proxy_server();
    expect_first_outgoing_CONNECT();
    incoming("HTTP/1.1 407 ProxyAuthentication Required\r\n"
             "Server: proxy_server.com\r\n"
             "Date: Mon, 5 Mar 2018 23:45:55 GMT\r\n"
             "Proxy-Authenticate: Basic\r\n"
		     "Content-Type: text/html\r\n"
			 "Content-Length: 169\r\n"
			 "\r\n"
			 "<!DOCTYPE html>\r\n"
			 "<html>\r\n"
			 " <head>\r\n"
			 "  <meta charset=\"UTF-8\" />\r\n"
			 "  <title>Error</title>\r\n"
			 " </head>\r\n"
			 " <body>\r\n"
			 "  <h1>407 ProxyAuthentication Required.</h1>\r\n"
			 " </body>\r\n"
			 "</html>\n");
    expect_outgoing_with_encoded_credentials_CONNECT("/subscribe/sub-key/health/0/0?pnsdk=unit-test-0.1&uuid=test_id",
                                             "\r\nProxy-Authorization: Basic c2VyaW91czpwYXNzd29yZA==");
    incoming("HTTP/1.1 200\r\n"
             "Server: proxy_server.com\r\n"
             "Date: Mon, 5 Mar 2018 23:45:59 GMT\r\n"
             "\r\n");
    expect_outgoing_with_url("/subscribe/sub-key/health/0/0?pnsdk=unit-test-0.1&uuid=test_id");
    incoming("HTTP/1.1 200\r\n"
             "Server: pubsub.pubnub.com\r\n"
             "Date: Mon, 5 Mar 2018 23:46:03 GMT\r\n"
             "Content-Length: 26\r\n"
             "\r\n"
             "[[],\"1516014978925123457\"]");
    attest(pubnub_subscribe(pbp, "health", NULL), equals(PNR_STARTED));

    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pbnc_fsm(pbp), equals(0));

    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));

    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/subscribe/sub-key/health/0/1516014978925123457?pnsdk=unit-test-0.1&uuid=test_id");
    incoming("HTTP/1.1 200\r\nContent-Length: 56\r\n\r\n[[pomegranate_juice,papaya,mango],\"1516714978925123457\"]");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(pbp, "health", NULL), equals(PNR_OK));

    attest(pubnub_get(pbp), streqs("pomegranate_juice"));
    attest(pubnub_get(pbp), streqs("papaya"));
    attest(pubnub_get(pbp), streqs("mango"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), streqs(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_proxy_protocol_get(pbp), equals(pbproxyHTTP_CONNECT)); 
}


Ensure(single_context_pubnub, establishes_proxy_connection_CONNECT_Digest)
{
    uint16_t port = 500;
#if __APPLE__
    char const* HTTP_proxy_header = "\r\nProxy-Authorization: Digest username=\"serious\", "
                                    "realm=\"testrealm@host.com\", "
                                    "nonce=\"dcd98b7102dd2f0e8b11d0f600bfb0c093\", "
                                    "uri=\"/subscribe/sub-key/health/0/0?pnsdk=unit-test-0.1&uuid=test_id\", "
                                    "cnonce=\"a7410000f13a461099acb7602a0cb53a\", "
                                    "nc=\"00000001\", "
                                    "qop=\"auth-int\", "
                                    "response=\"d3fad8976262b133cd4d01be00b7299f\", "
                                    "opaque=\"5ccc069c403ebaf9f0171e9517f40e41\"";
#else
    char const* HTTP_proxy_header = "\r\nProxy-Authorization: Digest username=\"serious\", "
                                    "realm=\"testrealm@host.com\", "
                                    "nonce=\"dcd98b7102dd2f0e8b11d0f600bfb0c093\", "
                                    "uri=\"/subscribe/sub-key/health/0/0?pnsdk=unit-test-0.1&uuid=test_id\", "
                                    "cnonce=\"67458b6bc6234b32a9983c6473483366\", "
                                    "nc=\"00000001\", "
                                    "qop=\"auth-int\", "
                                    "response=\"702347a42e8cf3663cbf8542a071e12d\", "
                                    "opaque=\"5ccc069c403ebaf9f0171e9517f40e41\"";
#endif
    pubnub_init(pbp, "publ-key", "sub-key");
    pubnub_set_user_id(pbp, "test_id");

    attest(pubnub_set_proxy_manual(pbp, pbproxyHTTP_CONNECT, "HTTPd/0.9", port), equals(0));
    attest(pubnub_set_proxy_authentication_username_password(pbp, "serious", "password"), equals(0));
    expect_have_dns_for_proxy_server();
    expect_first_outgoing_CONNECT();
    incoming("HTTP/1.0 407 ProxyAuthentication Required\r\n"
             "Server: HTTPd/0.9\r\n"
             "Date: Tue, 6 Mar 2018 01:26:47 GMT\r\n"
             "Proxy-Authenticate: Digest realm=\"testrealm@host.com\", "
                                        "qop=\"auth,auth-int\", "
                                        "nonce=\"dcd98b7102dd2f0e8b11d0f600bfb0c093\", "
                                        "opaque=\"5ccc069c403ebaf9f0171e9517f40e41\"\r\n"
             "Connection: close\r\n"
             "Content-Type: text/html\r\n"
             "Content-Length: 153\r\n"
             "\r\n"
             "<!DOCTYPE html>\r\n"
             "<html>\r\n"
             " <head>\r\n"
             "  <meta charset=\"UTF-8\" />\r\n"
             "  <title>Error</title>\r\n"
             " </head>\r\n"
             " <body>\r\n"
             "  <h1>401 Unauthorized.</h1>\r\n"
             " </body>\r\n"
             "</html>\n");
    expect(pbpal_close, when(pb, equals(pbp)), returns(0));
    expect_have_dns_for_proxy_server_without_enqueue_for_processing();
    expect_outgoing_with_encoded_credentials_CONNECT(
        "/subscribe/sub-key/health/0/0?pnsdk=unit-test-0.1&uuid=test_id",
        HTTP_proxy_header);
    incoming("HTTP/1.1 200\r\n"
             "Server: HTTPd/0.9\r\n"
             "Date: Tue, 6 Mar 2018 01:26:50 GMT\r\n"
             "\r\n");
    expect_outgoing_with_url("/subscribe/sub-key/health/0/0?pnsdk=unit-test-0.1&uuid=test_id");
    incoming("HTTP/1.1 200\r\n"
             "Server: pubsub.pubnub.com\r\n"
             "Date: Tue, 6 Mar 2018 01:26:52 GMT\r\n"
             "Content-Length: 26\r\n"
             "\r\n"
             "[[],\"1516014978925123457\"]");
    attest(pubnub_subscribe(pbp, "health", NULL), equals(PNR_STARTED));

    /* Pushing receiving messages until finished(small test buffer) */
    attest(pbnc_fsm(pbp), equals(0));
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));

    attest(pbnc_fsm(pbp), equals(0));

    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));

    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/subscribe/sub-key/health/0/1516014978925123457?pnsdk=unit-test-0.1&uuid=test_id");
    incoming("HTTP/1.1 200\r\nContent-Length: 56\r\n\r\n[[pomegranate_juice,papaya,mango],\"1516714978925123457\"]");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(pbp, "health", NULL), equals(PNR_OK));

    attest(pubnub_get(pbp), streqs("pomegranate_juice"));
    attest(pubnub_get(pbp), streqs("papaya"));
    attest(pubnub_get(pbp), streqs("mango"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), streqs(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_proxy_protocol_get(pbp), equals(pbproxyHTTP_CONNECT)); 
}

/* Verify ASSERT gets fired */

Ensure(single_context_pubnub, illegal_parameter_fires_assert) {
    expect_assert_in(pubnub_set_proxy_manual(NULL, pbproxyHTTP_GET, "proxy_server_url", 500), "pubnub_proxy.c");
    expect_assert_in(pubnub_set_proxy_manual(pbp, pbproxyHTTP_GET, NULL, 500), "pubnub_proxy.c");
    expect_assert_in(pubnub_proxy_protocol_get(NULL), "pubnub_proxy.c");
    expect_assert_in(pubnub_set_proxy_authentication_username_password(NULL, "username", "password"), "pubnub_proxy.c");
}
