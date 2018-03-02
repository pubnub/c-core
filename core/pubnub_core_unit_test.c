/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "cgreen/cgreen.h"
#include "cgreen/mocks.h"

#include "pubnub_pubsubapi.h"
#include "pubnub_coreapi.h"
#include "pubnub_assert.h"
#include "pubnub_alloc.h"
#include "pubnub_log.h"

#include "pbpal.h"
#include "pubnub_internal.h"

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

/* The Pubnub PAL mocks and stubs */

static void buf_setup(pubnub_t *pb)
{
    pb->ptr = (uint8_t*)pb->core.http_buf;
    pb->left = sizeof pb->core.http_buf;
//
    printf("\n PUBNUB_BUF_MAXLEN= %d\n", PUBNUB_BUF_MAXLEN);  
    printf(" sizeof pb->core.http_buf= %d\n\n", pb->left);
//
}

void pbpal_init(pubnub_t *pb)
{
    buf_setup(pb);
}

enum pbpal_resolv_n_connect_result pbpal_resolv_and_connect(pubnub_t *pb)
{
    return (int)mock(pb);
}

int pbntf_got_socket(pubnub_t *pb, pb_socket_t socket)
{
    return (int)mock(pb, socket);
}

void pbntf_lost_socket(pubnub_t *pb, pb_socket_t socket)
{
    mock(pb, socket);
}

void pbntf_update_socket(pubnub_t *pb, pb_socket_t socket)
{
    mock(pb, socket);
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


enum pbpal_resolv_n_connect_result pbpal_check_resolv_and_connect(pubnub_t *pb)
{
    return (int)mock(pb);
}

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

#if 0
#include <execinfo.h>
static void my_stack_trace(void)
{
    void *trace[16];
    char **messages = NULL;
    int i, trace_size = 0;

    trace_size = backtrace(trace, sizeof trace / sizeof trace[0]);
    messages = backtrace_symbols(trace, trace_size);
    for (i = 1; i < trace_size; ++i) {
        printf("[bt] #%d %s \n", i, messages[i]);
        /* find first occurence of '(' or ' ' in message[i] and assume
         * everything before that is the file name. (Don't go beyond 0 though
         * (string terminator)*/
        size_t p = 0;
        while(messages[i][p] != '(' && messages[i][p] != ' '
              && messages[i][p] != 0) 
            ++p;
        
        char syscom[256];
        snprintf(syscom, sizeof syscom, "addr2line %p -e %.*s", trace[i], p, messages[i]);
        //last parameter is the file name of the symbol
        system(syscom);
    }
}
#endif


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
//
    printf("\n to_read = %d\n", to_read);
//
    return to_read;
}

enum pubnub_res pbpal_line_read_status(pubnub_t *pb)
{
    uint8_t c;

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
        --pb->unreadlen;

        c = *pb->ptr++;
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
    return mock(pb);
}


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


/* The Pubnub NTF mocks and stubs */
void pbntf_trans_outcome(pubnub_t *pb)
{
    pb->state = PBS_IDLE;
    mock(pb);
}



/* Assert "catching" */
static bool m_expect_assert;
static jmp_buf m_assert_exp_jmpbuf;
static char const *m_expect_assert_file;


void assert_handler(char const *s, const char *file, long i)
{
//    mock(s, i);
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


/* The tests themselves */


Ensure(/*pbjson_parse, */get_object_value_valid) {
    char const *json = "{\"service\": \"xxx\", \"error\": true, \"payload\":{\"group\":\"gr\",\"chan\":[1,2,3]}, \"message\":0}";
    struct pbjson_elem elem = { json, json + strlen(json) };
    struct pbjson_elem parsed;
    
    attest(pbjson_get_object_value(&elem, "error", &parsed), equals(jonmpOK));
    attest(pbjson_elem_equals_string(&parsed, "true"), is_true);
    
    attest(pbjson_get_object_value(&elem, "service", &parsed), equals(jonmpOK));
    attest(pbjson_elem_equals_string(&parsed, "\"xxx\""), is_true);
    
    attest(pbjson_get_object_value(&elem, "message", &parsed), equals(jonmpOK));
    attest(pbjson_elem_equals_string(&parsed, "0"), is_true);
    
    attest(pbjson_get_object_value(&elem, "payload", &parsed), equals(jonmpOK));
    attest(pbjson_elem_equals_string(&parsed, "{\"group\":\"gr\",\"chan\":[1,2,3]}"), is_true);
}


Ensure(/*pbjson_parse, */ get_object_value_invalid) {
    char const *json = "{\"service\": \"xxx\", \"error\": true, \"payload\":{\"group\":\"gr\",\"chan\":[1,2,3]}, \"message\":0}";
    struct pbjson_elem elem = { json, json + strlen(json) };
    struct pbjson_elem parsed;

    attest(pbjson_get_object_value(&elem, "", &parsed), equals(jonmpInvalidKeyName));

    elem.end = elem.start;
    attest(pbjson_get_object_value(&elem, "payload", &parsed), equals(jonmpObjectIncomplete));

    elem.end = elem.start+1;
    attest(pbjson_get_object_value(&elem, "payload", &parsed), equals(jonmpKeyMissing));

    elem.end = elem.start+2;
    attest(pbjson_get_object_value(&elem, "payload", &parsed), equals(jonmpStringNotTerminated));

    elem.end = elem.start+10;
    attest(pbjson_get_object_value(&elem, "payload", &parsed), equals(jonmpMissingColon));

    elem.end = elem.start+11;
    attest(pbjson_get_object_value(&elem, "payload", &parsed), equals(jonmpMissingValueSeparator));

    elem.end = elem.start+12;
    attest(pbjson_get_object_value(&elem, "payload", &parsed), equals(jonmpMissingValueSeparator));

    elem.end = elem.start+13;
    attest(pbjson_get_object_value(&elem, "payload", &parsed), equals(jonmpMissingValueSeparator));

    elem.end = elem.start+17;
    attest(pbjson_get_object_value(&elem, "payload", &parsed), equals(jonmpObjectIncomplete));

    elem.end = elem.start+18;
    attest(pbjson_get_object_value(&elem, "payload", &parsed), equals(jonmpKeyMissing));

    elem.end = elem.start+19;
    attest(pbjson_get_object_value(&elem, "payload", &parsed), equals(jonmpKeyMissing));

    elem.end = elem.start+20;
    attest(pbjson_get_object_value(&elem, "payload", &parsed), equals(jonmpStringNotTerminated));

    elem.end = elem.start+26;
    attest(pbjson_get_object_value(&elem, "payload", &parsed), equals(jonmpMissingColon));
           
    elem.end = elem.start+27;
    attest(pbjson_get_object_value(&elem, "payload", &parsed), equals(jonmpMissingValueSeparator));

    elem.start = json+1;
    attest(pbjson_get_object_value(&elem, "payload", &parsed), equals(jonmpNoStartCurly));

    char const* json_2 = "{x:2}";
    elem.start = json_2;
    elem.end = json_2 + strlen(json_2);
    attest(pbjson_get_object_value(&elem, "payload", &parsed), equals(jonmpKeyNotString));

    char const* json_no_colon = "{\"x\" 2}";
    elem.start = json_no_colon;
    elem.end = json_no_colon + strlen(json_no_colon);
    attest(pbjson_get_object_value(&elem, "payload", &parsed), equals(jonmpMissingColon));
}


Ensure(/*pbjson_parse, */ get_object_value_key_doesnt_exist) {
    char const *json = "{\"service\": \"xxx\", \"error\": true, \"payload\":{\"group\":\"gr\",\"chan\":[1,2,3]}, \"message\":0}";
    struct pbjson_elem elem = { json, json + strlen(json) };
    struct pbjson_elem parsed;

    attest(pbjson_get_object_value(&elem, "zec", &parsed), equals(jonmpKeyNotFound));
    attest(pbjson_get_object_value(&elem, "xxx", &parsed), equals(jonmpKeyNotFound));
    attest(pbjson_get_object_value(&elem, "\"service\"", &parsed), equals(jonmpKeyNotFound));
    attest(pbjson_get_object_value(&elem, "servic", &parsed), equals(jonmpKeyNotFound));
    attest(pbjson_get_object_value(&elem, "ervice", &parsed), equals(jonmpKeyNotFound));
    attest(pbjson_get_object_value(&elem, "essage", &parsed), equals(jonmpKeyNotFound));
    attest(pbjson_get_object_value(&elem, "messag", &parsed), equals(jonmpKeyNotFound));
    attest(pbjson_get_object_value(&elem, "messagg", &parsed), equals(jonmpKeyNotFound));
    attest(pbjson_get_object_value(&elem, "mmessag", &parsed), equals(jonmpKeyNotFound));

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
//
        printf("\n free(m_msg_array[%d])\n", i);
//
    }
}

AfterEach(single_context_pubnub) {
    free_m_msgs(m_msg_array);
//    expect(pbpal_free, when(pb, equals(pbp)));
//    attest(pubnub_free(pbp), equals(0));
}


void expect_have_dns_for_pubnub_origin()
{
    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbpal_resolv_and_connect, when(pb, equals(pbp)), returns(pbpal_connect_success));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
}


static inline void expect_outgoing_with_url(char const *url) {
    expect(pbpal_send, when(data, streqs("GET ")), returns(0));
    expect(pbpal_send_status, returns(0));
    expect(pbpal_send_str, when(s, streqs(url)), returns(0));
    expect(pbpal_send_status, returns(0));
    expect(pbpal_send, when(data, streqs(" HTTP/1.1\r\nHost: ")), returns(0));
    expect(pbpal_send_status, returns(0));
    expect(pbpal_send_str, when(s, streqs(PUBNUB_ORIGIN)), returns(0));
    expect(pbpal_send_status, returns(0));
    expect(pbpal_send, when(data, streqs("\r\nUser-Agent: PubNub-C-core/2.2\r\n\r\n")), returns(0));
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
}


static void cancel_and_cleanup(pubnub_t *pbp)
{
    expect(pbntf_requeue_for_processing, when(pb, equals(pbp)), returns(0));
    pubnub_cancel(pbp);

    expect(pbpal_close, when(pb, equals(pbp)), returns(0));
    expect(pbpal_closed, when(pb, equals(pbp)), returns(true));
    expect(pbpal_forget, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pbnc_fsm(pbp), equals(0));
    attest(pbp->core.last_result, equals(PNR_CANCELLED));
} 


/* -- LEAVE operation -- */
Ensure(single_context_pubnub, leave_have_dns) {
    pubnub_init(pbp, "pubkey", "subkey");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/subkey/channel/lamanche/leave?pnsdk=unit-test-0.1");
    incoming("HTTP/1.1 200\r\nContent-Length: 2\r\n\r\n{}");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_leave(pbp, "lamanche", NULL), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_free(pbp), equals(-1));
}

/* This tests the DNS resolution code. Since we know for sure it is
   the same for all Pubnub operations/transactions, we shall test it
   only for "leave".
*/
Ensure(single_context_pubnub, leave_wait_dns) {
    pubnub_init(pbp, "pubkey", "subkey");

    /* DNS resolution not yet available... */
    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(+1));
    expect(pbpal_resolv_and_connect, when(pb, equals(pbp)), returns(pbpal_resolv_sent));
    expect(pbntf_watch_in_events, when(pb, equals(pbp)), returns(0));
    attest(pubnub_leave(pbp, "lamanche", NULL), equals(PNR_STARTED));

    /* ... still not available... */
    expect(pbpal_check_resolv_and_connect, when(pb, equals(pbp)), returns(pbpal_resolv_rcv_wouldblock));
    attest(pbnc_fsm(pbp), equals(0));

    /* ... and here it is: */
    expect(pbntf_watch_out_events, when(pb, equals(pbp)), returns(0));
    expect(pbpal_check_resolv_and_connect, when(pb, equals(pbp)), returns(pbpal_connect_success));
    expect_outgoing_with_url("/v2/presence/sub-key/subkey/channel/lamanche/leave?pnsdk=unit-test-0.1");
    incoming("HTTP/1.1 200\r\nContent-Length: 2\r\n\r\n{}");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pbnc_fsm(pbp), equals(0));

    attest(pbp->core.last_result, equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_free(pbp), equals(-1));
}


Ensure(single_context_pubnub, leave_wait_dns_cancel) {
    pubnub_init(pbp, "pubkey", "subkey");

    /* DNS resolution not yet available... */
    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(+1));
    expect(pbpal_resolv_and_connect, when(pb, equals(pbp)), returns(pbpal_resolv_sent));
    expect(pbntf_watch_in_events, when(pb, equals(pbp)), returns(0));
    attest(pubnub_leave(pbp, "lamanche", NULL), equals(PNR_STARTED));

    /* ... user is impatient... */
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    expect(pbntf_requeue_for_processing, when(pb, equals(pbp)), returns(0));
    pubnub_cancel(pbp);
    expect(pbpal_close, when(pb, equals(pbp)), returns(0));
    expect(pbpal_closed, when(pb, equals(pbp)), returns(true));
    expect(pbpal_forget, when(pb, equals(pbp)));
    attest(pbnc_fsm(pbp), equals(0));
    attest(pbp->core.last_result, equals(PNR_CANCELLED));
}

/* This tests the TCP establishment code. Since we know for sure it is
   the same for all Pubnub operations/transactions, we shall test it
   only for "leave".
 */
Ensure(single_context_pubnub, leave_wait_tcp) {
    pubnub_init(pbp, "pubkey", "subkey");

    /* DNS resolved but TCP connection not yet established... */
    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(+1));
    expect(pbpal_resolv_and_connect, when(pb, equals(pbp)), returns(pbpal_connect_wouldblock));
    attest(pubnub_leave(pbp, "lamanche", NULL), equals(PNR_STARTED));

    /* ... and here it is: */
    expect(pbpal_check_connect, when(pb, equals(pbp)), returns(pbpal_connect_success));
    expect_outgoing_with_url("/v2/presence/sub-key/subkey/channel/lamanche/leave?pnsdk=unit-test-0.1");
    incoming("HTTP/1.1 200\r\nContent-Length: 2\r\n\r\n{}");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pbnc_fsm(pbp), equals(0));

    attest(pbp->core.last_result, equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_free(pbp), equals(-1));
}


Ensure(single_context_pubnub, leave_wait_tcp_cancel) {
    pubnub_init(pbp, "pubkey", "subkey");

    /* DNS resolved but TCP connection not yet established... */
    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbpal_resolv_and_connect, when(pb, equals(pbp)), returns(pbpal_connect_wouldblock));
    expect(pbpal_check_connect, when(pb, equals(pbp)), returns(pbpal_connect_wouldblock));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    attest(pubnub_leave(pbp, "lamanche", NULL), equals(PNR_STARTED));

    /* ... user is impatient... */
    cancel_and_cleanup(pbp);
}


Ensure(single_context_pubnub, leave_changroup) {
    pubnub_init(pbp, "kpub", "ssub");

    /* Both channel and channel group set */
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/ssub/channel/k1/leave?pnsdk=unit-test-0.1&channel-group=tnt");
    incoming("HTTP/1.1 200\r\nContent-Length: 2\r\n\r\n{}");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_leave(pbp, "k1", "tnt"), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_free(pbp), equals(-1));

    /* Only channel group set */
    /* We dont have to do DNS resolution again on the same context already in use.
       The connection keeps beeing established by default. */ 
//    expect_have_dns_for_pubnub_origin();
    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/v2/presence/sub-key/ssub/channel/,/leave?pnsdk=unit-test-0.1&channel-group=mala");
    incoming("HTTP/1.1 200\r\nContent-Length: 2\r\n\r\n{}");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_leave(pbp, NULL, "mala"), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_free(pbp), equals(-1));

    /* Neither channel nor channel group set */
    attest(pubnub_leave(pbp, NULL, NULL), equals(PNR_INVALID_CHANNEL));
}


Ensure(single_context_pubnub, leave_uuid_auth) {
    pubnub_init(pbp, "pubX", "Xsub");

    /* Set UUID */
    pubnub_set_uuid(pbp, "DEDA-BABACECA-DECA");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/Xsub/channel/k/leave?pnsdk=unit-test-0.1&uuid=DEDA-BABACECA-DECA");
    incoming("HTTP/1.1 200\r\nContent-Length: 2\r\n\r\n{}");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_leave(pbp, "k", NULL), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));

    /* Set auth, too */
    pubnub_set_auth(pbp, "super-secret-key");
    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/v2/presence/sub-key/Xsub/channel/k2/leave?pnsdk=unit-test-0.1&uuid=DEDA-BABACECA-DECA&auth=super-secret-key");
    incoming("HTTP/1.1 200\r\nContent-Length: 2\r\n\r\n{}");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_leave(pbp, "k2", NULL), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));

    /* Reset UUID */
    pubnub_set_uuid(pbp, NULL);
    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/v2/presence/sub-key/Xsub/channel/k3/leave?pnsdk=unit-test-0.1&auth=super-secret-key");
    incoming("HTTP/1.1 200\r\nContent-Length: 2\r\n\r\n{}");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_leave(pbp, "k3", NULL), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));

    /* Reset auth, too */
    pubnub_set_auth(pbp, NULL);
    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/v2/presence/sub-key/Xsub/channel/k4/leave?pnsdk=unit-test-0.1");
    incoming("HTTP/1.1 200\r\nContent-Length: 2\r\n\r\n{}");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_leave(pbp, "k4", NULL), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_free(pbp), equals(-1));
}


Ensure(single_context_pubnub, leave_bad_response) {
    pubnub_init(pbp, "pubkey", "subkey");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/subkey/channel/lamanche/leave?pnsdk=unit-test-0.1");
    incoming("HTTP/1.1 200\r\nContent-Length: 2\r\n\r\n[]");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_leave(pbp, "lamanche", NULL), equals(PNR_FORMAT_ERROR));
    attest(pubnub_free(pbp), equals(-1));
}


Ensure(single_context_pubnub, leave_in_progress) {
    pubnub_init(pbp, "pubkey", "subkey");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/subkey/channel/lamanche/leave?pnsdk=unit-test-0.1");
    incoming("HTTP/1.1 200\r\n");
    /* simulates 'callback' condition of PNR_IN_PROGRESS.
     * expl: recv_msg would return 0 otherwise as if the connection
     * closes from servers side.
     */
    incoming("");
    attest(pubnub_leave(pbp, "lamanche", NULL), equals(PNR_STARTED));
    attest(pubnub_leave(pbp, "lamanche", NULL), equals(PNR_IN_PROGRESS));

    cancel_and_cleanup(pbp);
}


/* -- TIME operation -- */


Ensure(single_context_pubnub, time) {
    pubnub_init(pbp, "tkey", "subt");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/time/0?pnsdk=unit-test-0.1");
    incoming("HTTP/1.1 200\r\nContent-Length: 9\r\n\r\n[1643092]");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_time(pbp), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_get(pbp), streqs("1643092"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), equals(NULL));
    attest(pubnub_free(pbp), equals(-1));
}


Ensure(single_context_pubnub, time_bad_response) {
    pubnub_init(pbp, "tkey", "subt");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/time/0?pnsdk=unit-test-0.1");
    incoming("HTTP/1.1 200\r\nContent-Length: 9\r\n\r\n{1643092}");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_time(pbp), equals(PNR_FORMAT_ERROR));

    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), equals(NULL));
    attest(pubnub_free(pbp), equals(-1));
}


Ensure(single_context_pubnub, time_in_progress) {
    pubnub_init(pbp, "pubkey", "subkey");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/time/0?pnsdk=unit-test-0.1");
    incoming("HTTP/1.1 200\r\n");
    /* incoming empty string simulates conditions for PNR_IN_PROGRESS */ 
    incoming("");
//    expect(pbpal_close, when(pb, equals(pbp)), returns(0));
    attest(pubnub_time(pbp), equals(PNR_STARTED));
    attest(pubnub_time(pbp), equals(PNR_IN_PROGRESS));

    cancel_and_cleanup(pbp);
}


/* -- PUBLISH operation -- */


Ensure(single_context_pubnub, publish) {
    pubnub_init(pbp, "publkey", "subkey");

    expect_have_dns_for_pubnub_origin();

    expect_outgoing_with_url("/publish/publkey/subkey/0/jarak/0/%22zec%22?pnsdk=unit-test-0.1");
    incoming("HTTP/1.1 200\r\nContent-Length: 30\r\n\r\n[1,\"Sent\",\"14178940800777403\"]");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_publish(pbp, "jarak", "\"zec\""), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_free(pbp), equals(-1));
}


Ensure(single_context_pubnub, publish_http_chunked) {
    pubnub_init(pbp, "publkey", "subkey");

    expect_have_dns_for_pubnub_origin();

    expect_outgoing_with_url("/publish/publkey/subkey/0/jarak/0/%22zec%22?pnsdk=unit-test-0.1");
    incoming("HTTP/1.1 200\r\nTransfer-Encoding: chunked\r\n\r\n12\r\n[1,\"Sent\",\"1417894\r\n0C\r\n0800777403\"]\r\n0\r\n");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_publish(pbp, "jarak", "\"zec\""), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_free(pbp), equals(-1));
}


Ensure(single_context_pubnub, http_headers_no_content_length_or_chunked) {
    pubnub_init(pbp, "publkey", "subkey");

    expect_have_dns_for_pubnub_origin();

    expect_outgoing_with_url("/publish/publkey/subkey/0/,/0/%22zec%22?pnsdk=unit-test-0.1");
    incoming("HTTP/1.1 400\r\n\r\n[0,\"Invalid\",\"14178940800999505\"]");
    expect(pbpal_close, when(pb, equals(pbp)), returns(0));
    expect(pbpal_forget, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_publish(pbp, ",", "\"zec\""), equals(PNR_IO_ERROR));
    expect(pbpal_free, when(pb, equals(pbp)));
    attest(pubnub_free(pbp), equals(0));
}


Ensure(single_context_pubnub, publish_msg_too_long) {
    pubnub_init(pbp, "publkey", "subkey");

    char msg[PUBNUB_BUF_MAXLEN + 1];
    memset(msg, 'A', sizeof msg);
    msg[sizeof msg - 1] = '\0';
    attest(pubnub_publish(pbp, "w", msg), equals(PNR_TX_BUFF_TOO_SMALL));

    /* URL encoded char */
    memset(msg, '"', sizeof msg);
    msg[sizeof msg - 1] = '\0';
    attest(pubnub_publish(pbp, "w", msg), equals(PNR_TX_BUFF_TOO_SMALL));
    expect(pbpal_free, when(pb, equals(pbp)));
    attest(pubnub_free(pbp), equals(0));
}


Ensure(single_context_pubnub, publish_in_progress) {
    pubnub_init(pbp, "pubkey", "subkey");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/publish/pubkey/subkey/0/jarak/0/4443?pnsdk=unit-test-0.1");
    incoming("HTTP/1.1 200\r\n");
    /* simulates recv_msg() < 0 which saves from closed connection */ 
    incoming("");
    attest(pubnub_publish(pbp, "jarak", "4443"), equals(PNR_STARTED));
    attest(pubnub_publish(pbp, "x", "0"), equals(PNR_IN_PROGRESS));

    cancel_and_cleanup(pbp);
}


Ensure(single_context_pubnub, publish_uuid_auth) {
    pubnub_init(pbp, "pubX", "Xsub");

    /* Set UUID */
    pubnub_set_uuid(pbp, "0ADA-BEDA-0000");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/publish/pubX/Xsub/0/k/0/4443?pnsdk=unit-test-0.1&uuid=0ADA-BEDA-0000");
    incoming("HTTP/1.1 200\r\nContent-Length: 3\r\n\r\n[1]");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_publish(pbp, "k", "4443"), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));

    /* Set auth, too */
    pubnub_set_auth(pbp, "bad-secret-key");
    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/publish/pubX/Xsub/0/k2/0/443?pnsdk=unit-test-0.1&uuid=0ADA-BEDA-0000&auth=bad-secret-key");
    incoming("HTTP/1.1 200\r\nContent-Length: 3\r\n\r\n[1]");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_publish(pbp, "k2", "443"), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));

    /* Reset UUID */
    pubnub_set_uuid(pbp, NULL);
    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/publish/pubX/Xsub/0/k3/0/4443?pnsdk=unit-test-0.1&auth=bad-secret-key");
    incoming("HTTP/1.1 200\r\nContent-Length: 3\r\n\r\n[1]");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_publish(pbp, "k3", "4443"), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));

    /* Reset auth, too */
    pubnub_set_auth(pbp, NULL);
    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/publish/pubX/Xsub/0/k4/0/443?pnsdk=unit-test-0.1");
    incoming("HTTP/1.1 200\r\nContent-Length: 3\r\n\r\n[1]");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_publish(pbp, "k4", "443"), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_free(pbp), equals(-1));
}


Ensure(single_context_pubnub, publish_bad_response) {
    pubnub_init(pbp, "tkey", "subt");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/publish/tkey/subt/0/k6/0/443?pnsdk=unit-test-0.1");
    incoming("HTTP/1.1 200\r\nContent-Length: 9\r\n\r\n<\"1\":\"X\">");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_publish(pbp, "k6", "443"), equals(PNR_FORMAT_ERROR));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), equals(NULL));
    attest(pubnub_free(pbp), equals(-1));
}


Ensure(single_context_pubnub, publish_failed_server_side) {
    pubnub_init(pbp, "tkey", "subt");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/publish/tkey/subt/0/k6/0/443?pnsdk=unit-test-0.1");
    incoming("HTTP/1.1 200\r\nContent-Length: 9\r\n\r\n{\"1\":\"X\"}");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_publish(pbp, "k6", "443"), equals(PNR_PUBLISH_FAILED));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), equals(NULL));
    attest(pubnub_free(pbp), equals(-1));
}


/* -- HISTORY operation -- */


Ensure(single_context_pubnub, history_without_timetoken) {
    pubnub_init(pbp, "publhis", "subhis");

    /* Without time-token */
    expect_have_dns_for_pubnub_origin();

    expect_outgoing_with_url("/v2/history/sub-key/subhis/channel/ch?pnsdk=unit-test-0.1&count=22&include_token=false");
    incoming("HTTP/1.1 200\r\nContent-Length: 45\r\n\r\n[[1,2,3],14370854953886727,14370864554607266]");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_history(pbp, "ch", 22, false), equals(PNR_OK));

    attest(pubnub_get(pbp), streqs("[1,2,3]"));
    attest(pubnub_get(pbp), streqs("14370854953886727"));
    attest(pubnub_get(pbp), streqs("14370864554607266"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_free(pbp), equals(-1));
}

Ensure(single_context_pubnub, history_with_timetoken) {
    pubnub_init(pbp, "publhis", "subhis");

    /* With time-token */
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/history/sub-key/subhis/channel/ch?pnsdk=unit-test-0.1&count=22&include_token=true");
    incoming("HTTP/1.1 200\r\nContent-Length: 171\r\n\r\n[[{\"message\":1,\"timetoken\":14370863460777883},{\"message\":2,\"timetoken\":14370863461279046},{\"message\":3,\"timetoken\":14370863958459501}],14370863460777883,14370863958459501]");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_history(pbp, "ch", 22, true), equals(PNR_OK));

    attest(pubnub_get(pbp), streqs("[{\"message\":1,\"timetoken\":14370863460777883},{\"message\":2,\"timetoken\":14370863461279046},{\"message\":3,\"timetoken\":14370863958459501}]"));
    attest(pubnub_get(pbp), streqs("14370863460777883"));
    attest(pubnub_get(pbp), streqs("14370863958459501"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_free(pbp), equals(-1));
}


Ensure(single_context_pubnub, history_in_progress) {
    pubnub_init(pbp, "publhis", "subhis");

    expect_have_dns_for_pubnub_origin();

    expect_outgoing_with_url("/v2/history/sub-key/subhis/channel/ch?pnsdk=unit-test-0.1&count=22&include_token=false");
    incoming("HTTP/1.1 200\r\n");
    incoming("");
    attest(pubnub_history(pbp, "ch", 22, false), equals(PNR_STARTED));
    attest(pubnub_history(pbp, "x", 55, false), equals(PNR_IN_PROGRESS));

    cancel_and_cleanup(pbp);
}


Ensure(single_context_pubnub, history_auth) {
    pubnub_init(pbp, "pubX", "Xsub");

    pubnub_set_auth(pbp, "go-secret-key");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/history/sub-key/Xsub/channel/hhh?pnsdk=unit-test-0.1&auth=go-secret-key&count=40&include_token=false");
    incoming("HTTP/1.1 200\r\nContent-Length: 2\r\n\r\n[]");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_history(pbp, "hhh", 40, false), equals(PNR_OK));

    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_free(pbp), equals(-1));
}


Ensure(single_context_pubnub, history_bad_response) {
    pubnub_init(pbp, "pubkey", "Xsub");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/history/sub-key/Xsub/channel/ttt?pnsdk=unit-test-0.1&count=10&include_token=false");
    incoming("HTTP/1.1 200\r\nContent-Length: 2\r\n\r\n{}");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_history(pbp, "ttt", 10, false), equals(PNR_FORMAT_ERROR));
    attest(pubnub_free(pbp), equals(-1));
}


/* -- SET_STATE operation -- */


Ensure(single_context_pubnub, set_state) {
    pubnub_init(pbp, "publhis", "subhis");

    /* with uuid from context */
    pubnub_set_uuid(pbp, "universal");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/subhis/channel/ch/uuid/universal/data?pnsdk=unit-test-0.1&state={}");
    incoming("HTTP/1.1 200\r\nContent-Length: 67\r\n\r\n{\"status\": 200,\"message\":\"OK\", \"service\": \"Presence\", \"payload\":{}}");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_set_state(pbp, "ch", NULL, NULL, "{}"), equals(PNR_OK));

    attest(pubnub_get(pbp), streqs("{\"status\": 200,\"message\":\"OK\", \"service\": \"Presence\", \"payload\":{}}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));

    /* with 'auth' set in context and 'uuid' from parameter to the function */
    pubnub_set_auth(pbp, "auth-key");
    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/v2/presence/sub-key/subhis/channel/ch/uuid/shazalakazoo/data?pnsdk=unit-test-0.1&state={}&auth=auth-key");
    incoming("HTTP/1.1 200\r\nContent-Length: 63\r\n\r\n{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\",\"payload\":{}}");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_set_state(pbp, "ch", NULL, "shazalakazoo", "{}"), equals(PNR_OK));

    attest(pubnub_get(pbp), streqs("{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\",\"payload\":{}}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));

    /* set state to 'channel group(s)' with 'auth' set in context and 'uuid' from parameter to the function */
    pubnub_set_auth(pbp, "authentic");
    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/v2/presence/sub-key/subhis/channel/,/uuid/melwokee/data?pnsdk=unit-test-0.1&state={IOW}&channel-group=[gr1,gr2]&auth=authentic");
    incoming("HTTP/1.1 200\r\nContent-Length: 66\r\n\r\n{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\",\"payload\":{IOW}}");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_set_state(pbp, NULL, "[gr1,gr2]", "melwokee", "{IOW}"), equals(PNR_OK));

    attest(pubnub_get(pbp), streqs("{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\",\"payload\":{IOW}}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));

/* set state to 'channel(s)' and 'channel group(s)' with 'auth' set in context and 'uuid' from parameter to the function */
    pubnub_set_auth(pbp, "three");
    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/v2/presence/sub-key/subhis/channel/[ch1,ch2]/uuid/linda-darnell/data?pnsdk=unit-test-0.1&state={I}&channel-group=[gr3,gr4]&auth=three");
    incoming("HTTP/1.1 200\r\nContent-Length: 64\r\n\r\n{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\",\"payload\":{I}}");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_set_state(pbp, "[ch1,ch2]", "[gr3,gr4]", "linda-darnell", "{I}"), equals(PNR_OK));

    attest(pubnub_get(pbp), streqs("{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\",\"payload\":{I}}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));

/* set state for no 'channel' and no 'channel group'. 
 */
    pubnub_set_auth(pbp, NULL);//with or without this line
    attest(pubnub_set_state(pbp, NULL, NULL, "linda-darnell", "{I}"), equals(PNR_INVALID_CHANNEL));
    attest(pubnub_free(pbp), equals(-1));
}

Ensure(single_context_pubnub, set_state_in_progress) {
    pubnub_init(pbp, "publ-one", "sub-one");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/sub-one/channel/ch/uuid/blackbeard/data?pnsdk=unit-test-0.1&state={\"the_pirate\":\"true\"}");
    incoming("HTTP/1.1 200\r\n");
    incoming("");
    attest(pubnub_set_state(pbp, "ch", NULL, "blackbeard", "{\"the_pirate\":\"true\"}"), equals(PNR_STARTED));
    attest(pubnub_set_state(pbp, "ch", NULL, "blackbeard", "{\"the_pirate\":\"arrrrrrr_arrrrr\"}"), equals(PNR_IN_PROGRESS));

    cancel_and_cleanup(pbp);
}

Ensure(single_context_pubnub, set_state_in_progress_interrupted_and_accomplished) {
    pubnub_init(pbp, "publ-one", "sub-one");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/sub-one/channel/ch/uuid/blackbeard/data?pnsdk=unit-test-0.1&state={\"the_pirate\":\"true\"}");
    /* incoming first message */
    incoming("HTTP/1.1 200\r\n");
    incoming("");

    attest(pubnub_set_state(pbp, "ch", NULL, "blackbeard", "{\"the_pirate\":\"true\"}"), equals(PNR_STARTED));
    attest(pubnub_set_state(pbp, "ch", NULL, "blackbeard", "{\"the_pirate\":\"arrrrrrr_arrrrr\"}"), equals(PNR_IN_PROGRESS));

    /* incoming second and last message */
    incoming("Content-Length: 82\r\n\r\n{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\",\"payload\":{\"the_pirate\":\"true\"}}");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp))); 
    attest(pbnc_fsm(pbp), equals(0));
    attest(pbp->core.last_result, equals(PNR_OK));

    attest(pubnub_get(pbp), streqs("{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\",\"payload\":{\"the_pirate\":\"true\"}}"));
    attest(pubnub_get(pbp), equals(NULL));


    attest(pbp->core.last_result, equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_free(pbp), equals(-1));
}


Ensure(single_context_pubnub, set_state_set_auth_and_uuid) {
    pubnub_init(pbp, "pubX", "Xsub");

    pubnub_set_auth(pbp, "portobello");
    pubnub_set_uuid(pbp, "morgan");                             
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/Xsub/channel/ch/uuid/morgan/data?pnsdk=unit-test-0.1&state={\"the_privateer\":\"letter_of_marque\"}&auth=portobello");
    incoming("HTTP/1.1 200\r\nContent-Length: 96\r\n\r\n{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\",\"payload\":{\"the_privateer\":\"letter_of_marque\"}");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_set_state(pbp, "ch", NULL, NULL, "{\"the_privateer\":\"letter_of_marque\"}"), equals(PNR_OK));

    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_free(pbp), equals(-1));
}

Ensure(single_context_pubnub, set_state_bad_response) {
    pubnub_init(pbp, "pubkey", "Xsub");

    pubnub_set_uuid(pbp, "chili_peppers");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/Xsub/channel/ch/uuid/chili_peppers/data?pnsdk=unit-test-0.1&state={\"chili\":\"red\"}");
    incoming("HTTP/1.1 200\r\nContent-Length: 2\r\n\r\n<chiki, chiki bum, chiki bum]");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_set_state(pbp, "ch", NULL, NULL, "{\"chili\":\"red\"}"), equals(PNR_FORMAT_ERROR));
    attest(pubnub_free(pbp), equals(-1));
}


/* STATE_GET operation */

Ensure(single_context_pubnub, state_get_1channel) {
    pubnub_init(pbp, "key", "subY");

    /* with uuid from context */
    pubnub_set_uuid(pbp, "speedy");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/subY/channel/ch/uuid/speedy?pnsdk=unit-test-0.1");
    incoming("HTTP/1.1 200\r\nContent-Length: 76\r\n\r\n{\"status\": 200,\"message\":\"OK\", \"service\": \"Presence\", \"payload\":{\"running\"}}");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_state_get(pbp, "ch", NULL, NULL), equals(PNR_OK));

    attest(pubnub_get(pbp), streqs("{\"status\": 200,\"message\":\"OK\", \"service\": \"Presence\", \"payload\":{\"running\"}}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));

    /* with 'auth' set in context and 'uuid' from parameter to the function */
    pubnub_set_auth(pbp, "auth-key");
    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/v2/presence/sub-key/subY/channel/ch/uuid/brza_fotografija?pnsdk=unit-test-0.1&auth=auth-key");
    incoming("HTTP/1.1 200\r\nContent-Length: 72\r\n\r\n{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\",\"payload\":{key:value}}");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_state_get(pbp, "ch", NULL, "brza_fotografija"), equals(PNR_OK));

    attest(pubnub_get(pbp), streqs("{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\",\"payload\":{key:value}}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_free(pbp), equals(-1));
}

Ensure(single_context_pubnub, state_get_channelgroup) {
    pubnub_init(pbp, "key", "subY");

    /* state_get to 'channel group(s)' with 'auth' set in context and 'uuid' from parameter to the function */
    pubnub_set_auth(pbp, "mouth");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/subY/channel/,/uuid/fireworks?pnsdk=unit-test-0.1&channel-group=[gr1,gr2]&auth=mouth");
    incoming("HTTP/1.1 200\r\nContent-Length: 141\r\n\r\n{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\",\"payload\":{\"channels\":{\"alfa\":{key:value},\"bravo\":{},\"mike\":{},\"charlie\":{},\"foxtrot\":{}}}}");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_state_get(pbp, NULL, "[gr1,gr2]", "fireworks"), equals(PNR_OK));

    attest(pubnub_get(pbp), streqs("{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\",\"payload\":{\"channels\":{\"alfa\":{key:value},\"bravo\":{},\"mike\":{},\"charlie\":{},\"foxtrot\":{}}}}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));

/* set state to 'channel(s)' and 'channel group(s)' with 'auth' set in context and 'uuid' from parameter to the function */
    pubnub_set_auth(pbp, "cat");
    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/v2/presence/sub-key/subY/channel/[ch1,ch2]/uuid/leslie-mann?pnsdk=unit-test-0.1&channel-group=[gr3,gr4]&auth=cat");
    incoming("HTTP/1.1 200\r\nContent-Length: 153\r\n\r\n{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\",\"payload\":{\"channels\":{\"ch1\":{jason_state},\"ch2\":{},\"this_one\":{},\"that_one\":{},\"and_another_one\":{}}}}");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_state_get(pbp, "[ch1,ch2]", "[gr3,gr4]", "leslie-mann"), equals(PNR_OK));

    attest(pubnub_get(pbp), streqs("{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\",\"payload\":{\"channels\":{\"ch1\":{jason_state},\"ch2\":{},\"this_one\":{},\"that_one\":{},\"and_another_one\":{}}}}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));

/* state_get for no 'channel' and no 'channel group'. 
 */
    pubnub_set_auth(pbp, NULL);//with or without this line
    attest(pubnub_state_get(pbp, NULL, NULL, "leslie"), equals(PNR_INVALID_CHANNEL));
    attest(pubnub_free(pbp), equals(-1));
}

Ensure(single_context_pubnub, state_get_in_progress_interrupted_and_accomplished) {
    pubnub_init(pbp, "publ-key", "sub-key");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/sub-key/channel/ch/uuid/dragon?pnsdk=unit-test-0.1");
    /* incoming first message */
    incoming("HTTP/1.1 200\r\n");
    /* incoming empty string simulates conditions for PNR_IN_PROGRESS */ 
    incoming("");
    attest(pubnub_state_get(pbp, "ch", NULL, "dragon"), equals(PNR_STARTED));
    attest(pubnub_state_get(pbp, "night_channel", NULL, "knight"), equals(PNR_IN_PROGRESS));
    
    /* incoming next message */
    incoming("Content-Length: 82\r\n\r\n{");
    /* incoming the last message */
    incoming("\"status\":200,\"message\":\"OK\",\"service\":\"Presence\",\"payload\":{free_state_of_jones}}");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    
    /* rolling rock */
    attest(pbnc_fsm(pbp), equals(0));
    attest(pbnc_fsm(pbp), equals(0));
    attest(pbp->core.last_result, equals(PNR_OK));

    attest(pubnub_get(pbp), streqs("{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\",\"payload\":{free_state_of_jones}}"));
    attest(pubnub_get(pbp), equals(NULL));


    attest(pbp->core.last_result, equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_free(pbp), equals(-1));
}

Ensure(single_context_pubnub, state_get_bad_response) {
    pubnub_init(pbp, "publkey", "Xsub");

    pubnub_set_uuid(pbp, "annoying");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/Xsub/channel/ch/uuid/annoying?pnsdk=unit-test-0.1");
    incoming("HTTP/1.1 200\r\nContent-Length: 18\r\n\r\n[incorrect answer]");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_state_get(pbp, "ch", NULL, NULL), equals(PNR_FORMAT_ERROR));
    attest(pubnub_free(pbp), equals(-1));
}


/* HERE_NOW operation */

Ensure(single_context_pubnub, here_now_channel) {
    pubnub_init(pbp, "publZ", "subZ");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/subZ/channel/shade?pnsdk=unit-test-0.1");
    incoming("HTTP/1.1 200\r\nContent-Length: 98\r\n\r\n{\"status\": 200,\"message\":\"OK\", \"service\": \"Presence\", \"uuids\":[jack,johnnie,chivas],\"occupancy\":3}");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_here_now(pbp, "shade", NULL), equals(PNR_OK));

    attest(pubnub_get(pbp), streqs("{\"status\": 200,\"message\":\"OK\", \"service\": \"Presence\", \"uuids\":[jack,johnnie,chivas],\"occupancy\":3}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_free(pbp), equals(-1));
}

Ensure(single_context_pubnub, here_now_channel_with_auth) {
    pubnub_init(pbp, "publZ", "subZ");

    pubnub_set_auth(pbp, "auth-key");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/subZ/channel/channel?pnsdk=unit-test-0.1&auth=auth-key");
    incoming("HTTP/1.1 200\r\nContent-Length: 102\r\n\r\n{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\",\"uuids\":[daniel's,walker,regal,beam],\"occupancy\":4}}");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_here_now(pbp, "channel", NULL), equals(PNR_OK));

    attest(pubnub_get(pbp), streqs("{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\",\"uuids\":[daniel's,walker,regal,beam],\"occupancy\":4}}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_free(pbp), equals(-1));
}

Ensure(single_context_pubnub, here_now_channelgroups) {
    pubnub_init(pbp, "publZ", "subZ");

    /* set_auth */
    pubnub_set_auth(pbp, "mouse");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/subZ/channel/,?pnsdk=unit-test-0.1&channel-group=[gr2,gr1]&auth=mouse");
    incoming("HTTP/1.1 200\r\nContent-Length: 233\r\n\r\n{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\",\"payload\":{channels:{\"ch1\":{\"uuids\":[uuid1,uuid2],\"occupancy\":2},\"ch2\":{\"uuids\":[uuid3],\"occupancy\":1},\"ch3\":{etc.},\"ch4\":{},\"ch5\":{}},\"total_channels\":5,\"total_occupancy\":something}}");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_here_now(pbp, NULL, "[gr2,gr1]"), equals(PNR_STARTED));
    attest(pbnc_fsm(pbp), equals(0));
    attest(pbp->core.last_result, equals(PNR_OK));

    attest(pubnub_get(pbp), streqs("{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\",\"payload\":{channels:{\"ch1\":{\"uuids\":[uuid1,uuid2],\"occupancy\":2},\"ch2\":{\"uuids\":[uuid3],\"occupancy\":1},\"ch3\":{etc.},\"ch4\":{},\"ch5\":{}},\"total_channels\":5,\"total_occupancy\":something}}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_free(pbp), equals(-1));
}


Ensure(single_context_pubnub, here_now_channel_and_channelgroups) {
    pubnub_init(pbp, "publZ", "subZ");

    /* here_now on 'channel(s)' and 'channel group(s)' with 'auth' and 'uuid' */
    pubnub_set_auth(pbp, "globe");
    pubnub_set_uuid(pbp, "12345");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/subZ/channel/[ch1,ch2]?pnsdk=unit-test-0.1&channel-group=[gr3,gr4]&auth=globe&uuid=12345");
    incoming("HTTP/1.1 200\r\nContent-Length: 290\r\n\r\n{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\",\"payload\":{channels:{\"ch1\":{\"uuids\":[uuid1,uuid2,uuid3],\"occupancy\":3},\"ch2\":{\"uuids\":[uuid3,uuid4],\"occupancy\":2},\"ch3\":{\"uuids\":[uuid7],\"occupancy\":1},\"ch4\":{\"uuids\":[],\"occupancy\":0},\"ch5\":{etc.}},\"total_channels\":5,\"total_occupancy\":8}}");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_here_now(pbp, "[ch1,ch2]", "[gr3,gr4]"), equals(PNR_STARTED));
    attest(pbnc_fsm(pbp), equals(0));
    attest(pbp->core.last_result, equals(PNR_OK));

    attest(pubnub_get(pbp), streqs("{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\",\"payload\":{channels:{\"ch1\":{\"uuids\":[uuid1,uuid2,uuid3],\"occupancy\":3},\"ch2\":{\"uuids\":[uuid3,uuid4],\"occupancy\":2},\"ch3\":{\"uuids\":[uuid7],\"occupancy\":1},\"ch4\":{\"uuids\":[],\"occupancy\":0},\"ch5\":{etc.}},\"total_channels\":5,\"total_occupancy\":8}}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_free(pbp), equals(-1));
}

/*
Ensure(single_context_pubnub, here_now_no_channel_and_no_channelgroups) {
    pubnub_init(pbp, "publZ", "subZ");
    attest(pubnub_here_now(pbp, NULL, NULL), equals(PNR_INVALID_CHANNEL));
}
*/

/* processing of chunked response is common for all operations */
Ensure(single_context_pubnub, here_now_channel_and_channelgroups_chunked) {
    pubnub_init(pbp, "publZ", "subZ");

    /* here_now on 'channel(s)' and 'channel group(s)' with 'auth' and 'uuid' */
    pubnub_set_auth(pbp, "globe");
    pubnub_set_uuid(pbp, "12345");
    expect_have_dns_for_pubnub_origin();

    /* Don't forget that chunk lengths should be in hexadecimal representation */
    expect_outgoing_with_url("/v2/presence/sub-key/subZ/channel/[ch1,ch2]?pnsdk=unit-test-0.1&channel-group=[gr3,gr4]&auth=globe&uuid=12345");
    incoming("HTTP/1.1 200\r\nTransfer-Encoding: chunked\r\n\r\n 118\r\n{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\",\"payload\":{channels:{\"ch1\":{\"uuids\":[uuid1,uuid2,uuid3],\"occupancy\":3},\"ch2\":{\"uuids\":[uuid3,uuid4],\"occupancy\":2},\"ch3\":{\"uuids\":[uuid7],\"occupancy\":1},\"ch4\":{\"uuids\":[],\"occupancy\":0},\"ch5\":{etc.}},\"total_channels\":5,\"total_occu\r\na\r\npancy\":8}}\r\n0\r\n");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_here_now(pbp, "[ch1,ch2]", "[gr3,gr4]"), equals(PNR_STARTED));
    attest(pbnc_fsm(pbp), equals(0));
    attest(pbp->core.last_result, equals(PNR_OK));

    attest(pubnub_get(pbp), streqs("{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\",\"payload\":{channels:{\"ch1\":{\"uuids\":[uuid1,uuid2,uuid3],\"occupancy\":3},\"ch2\":{\"uuids\":[uuid3,uuid4],\"occupancy\":2},\"ch3\":{\"uuids\":[uuid7],\"occupancy\":1},\"ch4\":{\"uuids\":[],\"occupancy\":0},\"ch5\":{etc.}},\"total_channels\":5,\"total_occupancy\":8}}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_free(pbp), equals(-1));
}


Ensure(single_context_pubnub, here_now_in_progress_interrupted_and_accomplished) {
    pubnub_init(pbp, "publ-one", "sub-one");

    /* here_now on 'channel(s)' and 'channel group(s)' with 'auth' and 'uuid' */
    pubnub_set_auth(pbp, "lion");
    pubnub_set_uuid(pbp, "cub");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/sub-one/channel/[ch5,ch7]?pnsdk=unit-test-0.1&channel-group=[gr1,gr2]&auth=lion&uuid=cub");
    incoming("HTTP/1.1 200\r\nTransfer-Encoding: chunked\r\n\r\n122\r\n{\"status\":200,\"mes");

    attest(pubnub_here_now(pbp, "[ch5,ch7]", "[gr1,gr2]"), equals(PNR_STARTED));
    attest(pubnub_here_now(pbp, "ch", NULL), equals(PNR_IN_PROGRESS));

    /* finish chunked */
    incoming("sage\":\"OK\",\"service\":\"Presence\",\"payload\":{channels:{\"ch1\":{\"uuids\":[uuid1,uuid2,uuid3],\"occupancy\":3},\"ch2\":{\"uuids\":[uuid3,uuid4],\"occupancy\":2},\"ch3\":{\"uuids\":[uuid7],\"occupancy\":1},\"ch4\":{\"uuids\":[],\"occupancy\":0},\"ch5\":{etc.}},\"total_channels\":5,\"total_occupancy\":8}}\r\n0\r\n");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pbnc_fsm(pbp), equals(0));
    attest(pbp->core.last_result, equals(PNR_OK));

    attest(pubnub_get(pbp), streqs("{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\",\"payload\":{channels:{\"ch1\":{\"uuids\":[uuid1,uuid2,uuid3],\"occupancy\":3},\"ch2\":{\"uuids\":[uuid3,uuid4],\"occupancy\":2},\"ch3\":{\"uuids\":[uuid7],\"occupancy\":1},\"ch4\":{\"uuids\":[],\"occupancy\":0},\"ch5\":{etc.}},\"total_channels\":5,\"total_occupancy\":8}}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_free(pbp), equals(-1));
}

/* GLOBAL_HERE_NOW operation */

Ensure(single_context_pubnub, global_here_now) {
    pubnub_init(pbp, "publ-white", "sub-white");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/sub-white?pnsdk=unit-test-0.1");
    incoming("HTTP/1.1 200\r\nContent-Length: 334\r\n\r\n{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\",\"payload\":{channels:{\"ch1\":{\"uuids\":[uuid1,uuid2,uuid3],\"occupancy\":3},\"ch2\":{\"uuids\":[uuid3,uuid4],\"occupancy\":2},\"ch3\":{\"uuids\":[uuid7],\"occupancy\":1},\"ch4\":{\"uuids\":[],\"occupancy\":0},\"ch5\":{\"uuids\":[prle, tihi, mrki, paja], \"occupancy\":4}},\"total_channels\":5,\"total_occupancy\":12}}");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_global_here_now(pbp), equals(PNR_STARTED));
    attest(pubnub_global_here_now(pbp), equals(PNR_IN_PROGRESS));

    attest(pbnc_fsm(pbp), equals(0));
    attest(pbp->core.last_result, equals(PNR_OK));

    attest(pubnub_get(pbp), streqs("{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\",\"payload\":{channels:{\"ch1\":{\"uuids\":[uuid1,uuid2,uuid3],\"occupancy\":3},\"ch2\":{\"uuids\":[uuid3,uuid4],\"occupancy\":2},\"ch3\":{\"uuids\":[uuid7],\"occupancy\":1},\"ch4\":{\"uuids\":[],\"occupancy\":0},\"ch5\":{\"uuids\":[prle, tihi, mrki, paja], \"occupancy\":4}},\"total_channels\":5,\"total_occupancy\":12}}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_free(pbp), equals(-1));
}

Ensure(single_context_pubnub, global_here_now_chunked) {
    pubnub_init(pbp, "publ-beo", "sub-beo");
    
    /* With uuid & auth */
    pubnub_set_auth(pbp, "beograd");
    pubnub_set_uuid(pbp, "pobednik");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/sub-beo?pnsdk=unit-test-0.1&auth=beograd&uuid=pobednik");
    /* Chunk lengths are in hexadecimal representation */
    incoming("HTTP/1.1 200\r\nTransfer-Encoding: chunked\r\n\r\n1\r\n{\r\n12c\r\n\"status\":200,\"message\":\"OK\",\"service\":\"Presence\",\"payload\":{channels:{\"ch1\":{\"uuids\":[uuid1,uuid2,uuid3],\"occupancy\":3},\"ch2\":{\"uuids\":[uuid3,uuid4],\"occupancy\":2},\"ch3\":{\"uuids\":[uuid7],\"occupancy\":1},\"ch4\":{\"uuids\":[],\"occupancy\":0},\"ch5\":{\"uuids\":[prle, tihi, mrki, paja], \"occupancy\":4}},\"total_c\r\n21\r\nhannels\":5,\"total_occupancy\":12}}\r\n0\r\n");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_global_here_now(pbp), equals(PNR_STARTED));

    attest(pbnc_fsm(pbp), equals(0));
    attest(pbp->core.last_result, equals(PNR_OK));

    attest(pubnub_get(pbp), streqs("{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\",\"payload\":{channels:{\"ch1\":{\"uuids\":[uuid1,uuid2,uuid3],\"occupancy\":3},\"ch2\":{\"uuids\":[uuid3,uuid4],\"occupancy\":2},\"ch3\":{\"uuids\":[uuid7],\"occupancy\":1},\"ch4\":{\"uuids\":[],\"occupancy\":0},\"ch5\":{\"uuids\":[prle, tihi, mrki, paja], \"occupancy\":4}},\"total_channels\":5,\"total_occupancy\":12}}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_free(pbp), equals(-1));
}

Ensure(single_context_pubnub, global_here_now_in_progress_interrupted_and_acomplished) {
    pubnub_init(pbp, "publ-my", "sub-my");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/sub-my?pnsdk=unit-test-0.1");
    incoming("HTTP/1.1 200\r\nContent-Length: 334\r\n\r\n{\"status\":200,\"mess");
    /* Keeps fsm in progress */
    incoming("");
    attest(pubnub_global_here_now(pbp), equals(PNR_STARTED));
    attest(pubnub_global_here_now(pbp), equals(PNR_IN_PROGRESS));

    incoming("age\":\"OK\",\"service\":\"Presence\",\"payload\":{channels:{\"ch1\":{\"uuids\":[uuid1,uuid2,uuid3],\"occupancy\":3},\"ch2\":{\"uuids\":[uuid3,uuid4],\"occupancy\":2},\"ch3\":{\"uuids\":[uuid7],\"occupancy\":1},\"ch4\":{\"uuids\":[],\"occupancy\":0},\"ch5\":{\"uuids\":[prle, tihi, mrki, paja], \"occupancy\":4}},\"total_channels\":5,\"total_occupancy\":12}}");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    
    /* roll the rock */
    attest(pbnc_fsm(pbp), equals(0));
    attest(pbnc_fsm(pbp), equals(0));
    attest(pbp->core.last_result, equals(PNR_OK));

    attest(pubnub_get(pbp), streqs("{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\",\"payload\":{channels:{\"ch1\":{\"uuids\":[uuid1,uuid2,uuid3],\"occupancy\":3},\"ch2\":{\"uuids\":[uuid3,uuid4],\"occupancy\":2},\"ch3\":{\"uuids\":[uuid7],\"occupancy\":1},\"ch4\":{\"uuids\":[],\"occupancy\":0},\"ch5\":{\"uuids\":[prle, tihi, mrki, paja], \"occupancy\":4}},\"total_channels\":5,\"total_occupancy\":12}}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_free(pbp), equals(-1));
}

/* WHERE_NOW operation */

Ensure(single_context_pubnub, where_now) {
    pubnub_init(pbp, "publ-where", "sub-where");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/sub-where/uuid/shane(1953)?pnsdk=unit-test-0.1");
    incoming("HTTP/1.1 200\r\nContent-Length: 89\r\n\r\n{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\",\"Payload\":{\"channels\":[tcm,retro,mgm]}}");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_where_now(pbp, "shane(1953)"), equals(PNR_OK));

    attest(pubnub_get(pbp), streqs("{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\",\"Payload\":{\"channels\":[tcm,retro,mgm]}}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_free(pbp), equals(-1));
}

Ensure(single_context_pubnub, where_now_set_uuid) {
    pubnub_init(pbp, "publ-her", "sub-her");

    pubnub_set_uuid(pbp, "bird");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/sub-her/uuid/bird?pnsdk=unit-test-0.1");
    incoming("HTTP/1.1 200\r\nContent-Length: 100\r\n\r\n{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\",\"Payload\":{\"channels\":[discovery,nat_geo,nature]}}");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    /* Recognizes the uuid set from the context */
    attest(pubnub_where_now(pbp, NULL), equals(PNR_OK));

    attest(pubnub_get(pbp), streqs("{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\",\"Payload\":{\"channels\":[discovery,nat_geo,nature]}}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_free(pbp), equals(-1));
}

Ensure(single_context_pubnub, where_now_set_auth) {
    pubnub_init(pbp, "publ-sea", "sub-sea");

    pubnub_set_uuid(pbp, "fish");
    pubnub_set_auth(pbp, "big");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/sub-sea/uuid/whale?pnsdk=unit-test-0.1&auth=big");
    incoming("HTTP/1.1 200\r\nContent-Length: 107\r\n\r\n{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\",\"Payload\":{\"channels\":[first,second,third,fourth,fifth]}}");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    /* Chooses the uuid from the call not the one set in context */
    attest(pubnub_where_now(pbp, "whale"), equals(PNR_OK));

    attest(pubnub_get(pbp), streqs("{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\",\"Payload\":{\"channels\":[first,second,third,fourth,fifth]}}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_free(pbp), equals(-1));
}

Ensure(single_context_pubnub, where_now_in_progress_interrupted_and_accomplished) {
    pubnub_init(pbp, "publ-good", "sub-good");

    pubnub_set_uuid(pbp, "man_with_no_name");
    pubnub_set_auth(pbp, "west");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/sub-good/uuid/bad?pnsdk=unit-test-0.1&auth=west");
    incoming("HTTP/1.1 200\r\nContent-Length: 140\r\n\r\n{\"status\":200,\"message\":\"OK\",\"ser");
    /* server won't close connection after sending this much */
    incoming("");
    attest(pubnub_where_now(pbp, "bad"), equals(PNR_STARTED));
    attest(pubnub_where_now(pbp, "ugly"), equals(PNR_IN_PROGRESS));
    
    /* client reciving rest of the message */
    incoming("vice\":\"Presence\",\"Payload\":{\"channels\":[western,here,there,everywhere,fifth_channel,small_town,boot_hill]}}");
 
    /* Push until finished */ 
    attest(pbnc_fsm(pbp), equals(0));
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pbnc_fsm(pbp), equals(0));
    attest(pbp->core.last_result, equals(PNR_OK));
 
    attest(pubnub_get(pbp), streqs("{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\",\"Payload\":{\"channels\":[western,here,there,everywhere,fifth_channel,small_town,boot_hill]}}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_free(pbp), equals(-1));
}


/* HEARTBEAT operation */

Ensure(single_context_pubnub, heartbeat_channel) {
    pubnub_init(pbp, "publ-beat", "sub-beat");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/sub-beat/channel/panama/heartbeat?pnsdk=unit-test-0.1");
    incoming("HTTP/1.1 200\r\nContent-Length: 50\r\n\r\n{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\"}");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_heartbeat(pbp, "panama", NULL), equals(PNR_OK));

    attest(pubnub_get(pbp), streqs("{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\"}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_free(pbp), equals(-1));
}

Ensure(single_context_pubnub, heartbeat_channelgroups) {
    pubnub_init(pbp, "publ-beat", "sub-beat");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/sub-beat/channel/,/heartbeat?pnsdk=unit-test-0.1&channel-group=[deep,shallow]");
    incoming("HTTP/1.1 200\r\nContent-Length: 50\r\n\r\n{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\"}");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_heartbeat(pbp, NULL, "[deep,shallow]"), equals(PNR_OK));

    attest(pubnub_get(pbp), streqs("{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\"}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_free(pbp), equals(-1));
}

Ensure(single_context_pubnub, heartbeat_channel_and_channelgroups) {
    pubnub_init(pbp, "publ-ocean", "sub-ocean");
    
    pubnub_set_auth(pbp, "sailing");
    pubnub_set_uuid(pbp, "capetan");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/sub-ocean/channel/young_and_salty/heartbeat?pnsdk=unit-test-0.1&channel-group=[deep,shallow]&auth=sailing&uuid=capetan");
    incoming("HTTP/1.1 200\r\nContent-Length: 50\r\n\r\n{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\"}");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_heartbeat(pbp, "young_and_salty", "[deep,shallow]"), equals(PNR_OK));

    attest(pubnub_get(pbp), streqs("{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\"}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_free(pbp), equals(-1));
}


Ensure(single_context_pubnub, heartbeat_no_channel_and_no_channelgroups) {
    pubnub_init(pbp, "publ-", "sub-");
    attest(pubnub_heartbeat(pbp, NULL, NULL), equals(PNR_INVALID_CHANNEL));
    expect(pbpal_free, when(pb, equals(pbp)));
    attest(pubnub_free(pbp), equals(0));
}

Ensure(single_context_pubnub, heartbeat_channel_and_channelgroups_interrupted_and_accomplished) {
    pubnub_init(pbp, "publ-game", "sub-game");
    
    pubnub_set_auth(pbp, "white");
    pubnub_set_uuid(pbp, "player");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/sub-game/channel/moves/heartbeat?pnsdk=unit-test-0.1&channel-group=[fast,slow]&auth=white&uuid=player");
    incoming("HTTP/1.1 200\r\nContent-Length:");
    incoming("");
    attest(pubnub_heartbeat(pbp, "moves", "[fast,slow]"), equals(PNR_STARTED));
    attest(pubnub_heartbeat(pbp, "punches", "[fast,slow]"), equals(PNR_IN_PROGRESS));

    incoming(" 50\r\n\r\n{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\"}");
    /* Push until finished */ 
    attest(pbnc_fsm(pbp), equals(0));
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pbnc_fsm(pbp), equals(0));
    attest(pbp->core.last_result, equals(PNR_OK));

    attest(pubnub_get(pbp), streqs("{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\"}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_free(pbp), equals(-1));
}


/* ADD_CHANNEL_TO_GROUP operation */

Ensure(single_context_pubnub, add_channel_to_group) {
    pubnub_init(pbp, "publ-kc", "sub-kc");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v1/channel-registration/sub-key/sub-kc/channel-group/ch_group?pnsdk=unit-test-0.1&add=ch_one");
    incoming("HTTP/1.1 200\r\nContent-Length: 72\r\n\r\n{\"service\":\"channel-registry\",\"status\":200,\"error\":false,\"message\":\"OK\"}");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_add_channel_to_group(pbp, "ch_one", "ch_group"), equals(PNR_OK));

    attest(pubnub_get_channel(pbp), streqs("{\"service\":\"channel-registry\",\"status\":200,\"error\":false,\"message\":\"OK\"}"));
    attest(pubnub_get_channel(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_free(pbp), equals(-1));
}

Ensure(single_context_pubnub, add_channel_to_group_interrupted_and_accomplished) {
    pubnub_init(pbp, "publ-kc", "sub-kc");
    /* With auth */
    pubnub_set_auth(pbp, "rice_chocolate");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v1/channel-registration/sub-key/sub-kc/channel-group/ch_group?pnsdk=unit-test-0.1&add=ch_one&auth=rice_chocolate");
    incoming("HTTP/1.1 200\r\nContent-Length: 72\r\n\r\n{\"service\"");
    /* won't close connection */
    incoming("");

    attest(pubnub_add_channel_to_group(pbp, "ch_one", "ch_group"), equals(PNR_STARTED));
    attest(pubnub_add_channel_to_group(pbp, "ch", "ch_group"), equals(PNR_IN_PROGRESS));

    incoming(":\"channel-registry\",\"status\":200,\"error\":false,\"message\":\"OK\"}");
    /* keep 'turning' it until finished */
    attest(pbnc_fsm(pbp), equals(0));
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pbnc_fsm(pbp), equals(0));
    attest(pbp->core.last_result, equals(PNR_OK));

    attest(pubnub_get_channel(pbp), streqs("{\"service\":\"channel-registry\",\"status\":200,\"error\":false,\"message\":\"OK\"}"));
    attest(pubnub_get_channel(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_free(pbp), equals(-1));
}

/* REMOVE_CHANNEL_FROM_GROUP operation */

Ensure(single_context_pubnub, remove_channel_from_group) {
    pubnub_init(pbp, "publ-kum_Ruzvelt", "sub-kum_Ruzvelt");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v1/channel-registration/sub-key/sub-kum_Ruzvelt/channel-group/ch_group?pnsdk=unit-test-0.1&remove=ch_one");
    incoming("HTTP/1.1 200\r\nContent-Length: 72\r\n\r\n{\"service\":\"channel-registry\",\"status\":200,\"error\":false,\"message\":\"OK\"}");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_remove_channel_from_group(pbp, "ch_one", "ch_group"), equals(PNR_OK));

    attest(pubnub_get_channel(pbp), streqs("{\"service\":\"channel-registry\",\"status\":200,\"error\":false,\"message\":\"OK\"}"));
    attest(pubnub_get_channel(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_free(pbp), equals(-1));
}

Ensure(single_context_pubnub, remove_channel_from_group_interrupted_and_accomplished) {
    pubnub_init(pbp, "publ-Teheran", "sub-Teheran");
    /* With auth */
    pubnub_set_auth(pbp, "dates");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v1/channel-registration/sub-key/sub-Teheran/channel-group/ch_group?pnsdk=unit-test-0.1&remove=ch_one&auth=dates");
    incoming("HTTP/1.1 200\r\nContent-");
    /* won't close connection */
    incoming("");

    attest(pubnub_remove_channel_from_group(pbp, "ch_one", "ch_group"), equals(PNR_STARTED));
    attest(pubnub_remove_channel_from_group(pbp, "ch", "ch_group"), equals(PNR_IN_PROGRESS));

    incoming("Length: 72\r\n\r\n{\"service\":\"channel-registry\",\"status\":200,\"error\":false,\"message\":\"OK\"}");
    /* keep 'turning' it until finished */
    attest(pbnc_fsm(pbp), equals(0));
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pbnc_fsm(pbp), equals(0));
    attest(pbp->core.last_result, equals(PNR_OK));

    attest(pubnub_get_channel(pbp), streqs("{\"service\":\"channel-registry\",\"status\":200,\"error\":false,\"message\":\"OK\"}"));
    attest(pubnub_get_channel(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_free(pbp), equals(-1));
}

/* REMOVE_CHANNEL_GROUP operation */

Ensure(single_context_pubnub, remove_channel_group) {
    pubnub_init(pbp, "publ-bell", "sub-bell");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v1/channel-registration/sub-key/sub-bell/channel-group/group_name/remove?pnsdk=unit-test-0.1");
    incoming("HTTP/1.1 200\r\nContent-Length: 82\r\n\r\n{\"service\": \"channel-registry\" , \"status\"  : 200 ,\"error\" :false,\"message\":  \"OK\"}");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_remove_channel_group(pbp, "group_name"), equals(PNR_OK));

    attest(pubnub_get_channel(pbp), streqs("{\"service\": \"channel-registry\" , \"status\"  : 200 ,\"error\" :false,\"message\":  \"OK\"}"));
    attest(pubnub_get_channel(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_free(pbp), equals(-1));
}

Ensure(single_context_pubnub, remove_channel_group_interrupted_and_accomplished) {
    pubnub_init(pbp, "publ-blue", "sub-blue");
    /* With auth */
    pubnub_set_auth(pbp, "sky");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v1/channel-registration/sub-key/sub-blue/channel-group/group/remove?pnsdk=unit-test-0.1&auth=sky");
    incoming("HTTP/1.1 200\r\nContent-Len");
    /* won't close connection */
    incoming("");

    attest(pubnub_remove_channel_group(pbp, "group"), equals(PNR_STARTED));
    attest(pubnub_remove_channel_group(pbp, "another_group"), equals(PNR_IN_PROGRESS));

    incoming("gth: 79\r\n\r\n{ \"service\":\"channel-registry\" ,\"status\" : 200,\"error\" : false ,\"message\":\"OK\"}");
    /* keep 'turning' it until finished */
    attest(pbnc_fsm(pbp), equals(0));
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pbnc_fsm(pbp), equals(0));
    attest(pbp->core.last_result, equals(PNR_OK));

    attest(pubnub_get_channel(pbp), streqs("{ \"service\":\"channel-registry\" ,\"status\" : 200,\"error\" : false ,\"message\":\"OK\"}"));
    attest(pubnub_get_channel(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_free(pbp), equals(-1));
}

/* LIST_CHANNEL_GROUP operation */

Ensure(single_context_pubnub, list_channel_group) {
    pubnub_init(pbp, "publ-science", "sub-science");
    /* With auth */
    pubnub_set_auth(pbp, "research");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v1/channel-registration/sub-key/sub-science/channel-group/info?pnsdk=unit-test-0.1&auth=research");
    incoming("HTTP/1.1 200\r\nContent-Length: 154\r\n\r\n{\"service\": \"channel-registry\" , \"status\": 200 ,\"error\" :false,\"payload\":{\"group\":\"info\",\"channels\":{\"weather\", \"polution\",\"resources\",\"consumtion\",...}}}");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_list_channel_group(pbp, "info"), equals(PNR_OK));

    attest(pubnub_get_channel(pbp), streqs("{\"service\": \"channel-registry\" , \"status\": 200 ,\"error\" :false,\"payload\":{\"group\":\"info\",\"channels\":{\"weather\", \"polution\",\"resources\",\"consumtion\",...}}}"));
    attest(pubnub_get_channel(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_free(pbp), equals(-1));
}

Ensure(single_context_pubnub, list_channel_group_interrupted_and_accomplished) {
    pubnub_init(pbp, "publ-air-traffic", "sub-air-traffic");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v1/channel-registration/sub-key/sub-air-traffic/channel-group/airborne?pnsdk=unit-test-0.1");
    incoming("HTTP/1.1 200\r\nContent-Len");
    /* won't close connection */
    incoming("");

    attest(pubnub_list_channel_group(pbp, "airborne"), equals(PNR_STARTED));
    attest(pubnub_list_channel_group(pbp, "landed"), equals(PNR_IN_PROGRESS));

    incoming("gth: 128\r\n\r\n{ \"service\":\"channel-registry\" ,\"status\" : 200,\"error\" : false , \"payload\":{\"group\":\"airborne\",\"channels\":{\"pw45\", \"xg37\",...}}}");
    /* keep 'turning' it until finished */
    attest(pbnc_fsm(pbp), equals(0));
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pbnc_fsm(pbp), equals(0));
    attest(pbp->core.last_result, equals(PNR_OK));

    attest(pubnub_get_channel(pbp), streqs("{ \"service\":\"channel-registry\" ,\"status\" : 200,\"error\" : false , \"payload\":{\"group\":\"airborne\",\"channels\":{\"pw45\", \"xg37\",...}}}"));
    attest(pubnub_get_channel(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_free(pbp), equals(-1));
}


/* SUBSCRIBE operation */

Ensure(single_context_pubnub, subscribe) {
    pubnub_init(pbp, "publ-magazin", "sub-magazin");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/subscribe/sub-magazin/health/0/0?pnsdk=unit-test-0.1");
    incoming("HTTP/1.1 200\r\nContent-Length: 26\r\n\r\n[[],\"1516014978925123457\"]");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(pbp, "health", NULL), equals(PNR_OK));

    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));

    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/subscribe/sub-magazin/health/0/1516014978925123457?pnsdk=unit-test-0.1");
    incoming("HTTP/1.1 200\r\nContent-Length: 79\r\n\r\n[[pomegranate_juice,papaya,mango],\"1516714978925123457\",\"health,health,health\"]");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(pbp, "health", NULL), equals(PNR_OK));

    attest(pubnub_get(pbp), streqs("pomegranate_juice"));
    attest(pubnub_get(pbp), streqs("papaya"));
    attest(pubnub_get(pbp), streqs("mango"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), streqs("health"));
    attest(pubnub_get_channel(pbp), streqs("health"));
    attest(pubnub_get_channel(pbp), streqs("health"));
    attest(pubnub_get_channel(pbp), streqs(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_free(pbp), equals(-1));
}

Ensure(single_context_pubnub, subscribe_channel_groups) {
    pubnub_init(pbp, "publ-bulletin", "sub-bulletin");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/subscribe/sub-bulletin/,/0/0?pnsdk=unit-test-0.1&channel-group=updates");
    incoming("HTTP/1.1 200\r\nContent-Length: 25\r\n\r\n[[],\"251614978925123457\"]");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(pbp, NULL, "updates"), equals(PNR_OK));

    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), streqs(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));

    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/subscribe/sub-bulletin/,/0/251614978925123457?pnsdk=unit-test-0.1&channel-group=updates");
    incoming("HTTP/1.1 200\r\nContent-Length: 110\r\n\r\n[[skype,web_brouser,text_editor],\"251624978925123457\",\"updates,updates,updates\",\"messengers,brousers,editors\"]");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(pbp, NULL, "updates"), equals(PNR_OK));

    attest(pubnub_get(pbp), streqs("skype"));
    attest(pubnub_get_channel(pbp), streqs("messengers"));
    attest(pubnub_get(pbp), streqs("web_brouser"));
    attest(pubnub_get_channel(pbp), streqs("brousers"));
    attest(pubnub_get(pbp), streqs("text_editor"));
    attest(pubnub_get_channel(pbp), streqs("editors"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), streqs(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));

    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/subscribe/sub-bulletin/,/0/251624978925123457?pnsdk=unit-test-0.1&channel-group=updates");
    incoming("HTTP/1.1 200\r\nContent-Length: 51\r\n\r\n[[virtualbox],\"251624978925123457\",\"updates\",\"VMs\"]");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(pbp, NULL, "updates"), equals(PNR_OK));

    attest(pubnub_get(pbp), streqs("virtualbox"));
    attest(pubnub_get_channel(pbp), streqs("VMs"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), streqs(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_free(pbp), equals(-1));
}

Ensure(single_context_pubnub, subscribe_channels_and_channel_groups) {
    pubnub_init(pbp, "publ-key", "sub-Key");

    pubnub_set_uuid(pbp, "admin");
    pubnub_set_auth(pbp, "msgs");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/subscribe/sub-Key/[ch1,ch2]/0/0?pnsdk=unit-test-0.1&channel-group=[chgr2,chgr3,chgr4]&uuid=admin&auth=msgs");
    incoming("HTTP/1.1 200\r\nContent-Length: 26\r\n\r\n[[],\"3516149789251234578\"]");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(pbp, "[ch1,ch2]", "[chgr2,chgr3,chgr4]"), equals(PNR_OK));

    attest(pubnub_get_channel(pbp), streqs(NULL));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));

    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/subscribe/sub-Key/[ch1,ch2]/0/3516149789251234578?pnsdk=unit-test-0.1&channel-group=[chgr2,chgr3,chgr4]&uuid=admin&auth=msgs");
    incoming("HTTP/1.1 200\r\nContent-Length: 150\r\n\r\n[[msg1,msg2,{\"text\":\"Hello World!\"},msg4,msg5,{\"key\":\"val\\ue\"}],\"352624978925123458\",\"chgr4,chgr2,chgr3,chgr4,chgr7,chgr4\",\"ch5,ch8,ch6,ch17,ch1,ch2\"]");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(pbp, "[ch1,ch2]", "[chgr2,chgr3,chgr4]"), equals(PNR_OK));

    attest(pubnub_get_channel(pbp), streqs("ch5"));
    attest(pubnub_get_channel(pbp), streqs("ch8"));
    attest(pubnub_get_channel(pbp), streqs("ch6"));
    attest(pubnub_get_channel(pbp), streqs("ch17"));
    attest(pubnub_get_channel(pbp), streqs("ch1"));
    attest(pubnub_get_channel(pbp), streqs("ch2"));
    attest(pubnub_get_channel(pbp), streqs(NULL));
    attest(pubnub_get(pbp), streqs("msg1"));
    attest(pubnub_get(pbp), streqs("msg2"));
    attest(pubnub_get(pbp), streqs("{\"text\":\"Hello World!\"}"));
    attest(pubnub_get(pbp), streqs("msg4"));
    attest(pubnub_get(pbp), streqs("msg5"));
    attest(pubnub_get(pbp), streqs("{\"key\":\"val\\ue\"}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));

    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/subscribe/sub-Key/ch17/0/352624978925123458?pnsdk=unit-test-0.1&uuid=admin&auth=msgs");
    incoming("HTTP/1.1 200\r\nContent-Length: 47\r\n\r\n[[message],\"352624979925123457\",\"chgr4\",\"ch17\"]");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(pbp, "ch17", NULL), equals(PNR_OK));
    attest(pubnub_subscribe(pbp, NULL, "chgr2"), equals(PNR_RX_BUFF_NOT_EMPTY));

    attest(pubnub_get(pbp), streqs("message"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), streqs("ch17"));
    attest(pubnub_get_channel(pbp), streqs(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_free(pbp), equals(-1));
}

Ensure(single_context_pubnub, subscribe_channel_groups_interrupted_and_accomplished_chunked) {
    pubnub_init(pbp, "publ-measurements", "sub-measurements");

    pubnub_set_uuid(pbp, "technician");
    pubnub_set_auth(pbp, "weather-conditions");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/subscribe/sub-measurements/,/0/0?pnsdk=unit-test-0.1&channel-group=[air-temperature,humidity,wind-speed-and-direction,pressure]&uuid=technician&auth=weather-conditions");
    incoming("HTTP/1.1 200\r\nContent-Length: 26\r\n\r\n[[],\"1516149789251234578\"]");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(pbp, NULL, "[air-temperature,humidity,wind-speed-and-direction,pressure]"), equals(PNR_OK));

    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), streqs(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));

    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/subscribe/sub-measurements/,/0/1516149789251234578?pnsdk=unit-test-0.1&channel-group=[air-temperature,humidity,wind-speed-and-direction,pressure]&uuid=technician&auth=weather-conditions");
    incoming("HTTP/1.1 200\r\nTransfer-Encoding: chunked\r\n\r\n9d\r\n[[{\"uuid1\":\"-2\"},{\"uuid2\":\"-5\"},{\"Kingston\":\"+22\"},{\"Manila\":\"+38\"},{\"bouy76\":\"+5\"},{\"Pr. Astrid Coast\":\"Unable_to_fetch_temperature\"}],\"1516149789251234583\"\r\n97\r\n,\"air-temperature,air-temperature,air-temperature,air-temperature,air-temperature,air-temperature\",\"ch-atmp,ch-atmp,ch-atmp,ch-atmp,ch-atmp,ch-atmp\"]\r\n0\r\n");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(pbp, NULL, "[air-temperature,humidity,wind-speed-and-direction,pressure]"), equals(PNR_STARTED));
    attest(pubnub_subscribe(pbp, "some_channels", "some_channel_goups"), equals(PNR_IN_PROGRESS));

    /* 'push' until finished */
    attest(pbnc_fsm(pbp), equals(0));
    attest(pbp->core.last_result, equals(PNR_OK));
    attest(pubnub_subscribe(pbp, NULL, "some_channel_goups"), equals(PNR_RX_BUFF_NOT_EMPTY));

    attest(pubnub_get(pbp), streqs("{\"uuid1\":\"-2\"}"));
    attest(pubnub_get_channel(pbp), streqs("ch-atmp"));
    attest(pubnub_get(pbp), streqs("{\"uuid2\":\"-5\"}"));
    attest(pubnub_get(pbp), streqs("{\"Kingston\":\"+22\"}"));
    attest(pubnub_get(pbp), streqs("{\"Manila\":\"+38\"}"));
    attest(pubnub_get(pbp), streqs("{\"bouy76\":\"+5\"}"));
    attest(pubnub_subscribe(pbp, NULL, "wind-speed-and-direction"), equals(PNR_RX_BUFF_NOT_EMPTY));
    attest(pubnub_get(pbp), streqs("{\"Pr. Astrid Coast\":\"Unable_to_fetch_temperature\"}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));

    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/subscribe/sub-measurements/,/0/1516149789251234583?pnsdk=unit-test-0.1&channel-group=[air-temperature,humidity,wind-speed-and-direction,pressure]&uuid=technician&auth=weather-conditions");
    incoming("HTTP/1.1 200\r\nTransfer-Encoding: chunked\r\n\r\n101\r\n[[{\"uuid1\":{\"dir\":w,\"speed\":\"2mph\",\"blows\":\"3mph\"}},{\"uuid2\":{\"dir\":nw,\"speed\":\"5mph\",\"blows\":\"7mph\"}},{\"Sebu, Philippines\":{\"dir\":nne,\"speed\":\"4mph\",\"blows\":\"4mph\"}},{\"Sri Jayawardenepura Kotte, Sri Lanka\":{\"dir\":ne,\"speed\":\"7mph\",\"blows\":\"7mph\"}}],\"151614\r\n93\r\n9789251234597\",\"wind-speed-and-direction,wind-speed-and-direction,wind-speed-and-direction,wind-speed-and-direction\",\"ch-ws1,ch-ws2,ch-ws3,ch-ws1\"]\r\n0\r\n");
    attest(pubnub_subscribe(pbp, NULL, "[air-temperature,humidity,wind-speed-and-direction,pressure]"), equals(PNR_STARTED));
    attest(pubnub_subscribe(pbp, NULL, "humidity"), equals(PNR_IN_PROGRESS));
    
    /* 'push' until finished */
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pbnc_fsm(pbp), equals(0));
    attest(pbp->core.last_result, equals(PNR_OK));

    attest(pubnub_get_channel(pbp), streqs("ch-ws1"));
    attest(pubnub_get(pbp), streqs("{\"uuid1\":{\"dir\":w,\"speed\":\"2mph\",\"blows\":\"3mph\"}}"));
    attest(pubnub_get(pbp), streqs("{\"uuid2\":{\"dir\":nw,\"speed\":\"5mph\",\"blows\":\"7mph\"}}"));
    attest(pubnub_get(pbp), streqs("{\"Sebu, Philippines\":{\"dir\":nne,\"speed\":\"4mph\",\"blows\":\"4mph\"}}"));
    attest(pubnub_get(pbp), streqs("{\"Sri Jayawardenepura Kotte, Sri Lanka\":{\"dir\":ne,\"speed\":\"7mph\",\"blows\":\"7mph\"}}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_free(pbp), equals(-1));
}

Ensure(single_context_pubnub, subscribe_no_channel_and_no_channelgroups) {
    pubnub_init(pbp, "publ-something", "sub-something");
    attest(pubnub_subscribe(pbp, NULL, NULL), equals(PNR_INVALID_CHANNEL));
    expect(pbpal_free, when(pb, equals(pbp)));
    attest(pubnub_free(pbp), equals(0));
}

Ensure(single_context_pubnub, subscribe_parse_response_format_error) {
    pubnub_init(pbp, "publ-fe", "sub-fe");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/subscribe/sub-fe/[ch2]/0/0?pnsdk=unit-test-0.1&channel-group=[chgr2,chgr4]");
    incoming("HTTP/1.1 200\r\nContent-Length: 26\r\n\r\n{[],\"3516149789251234578\"}");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(pbp, "[ch2]", "[chgr2,chgr4]"), equals(PNR_FORMAT_ERROR));
    attest(pubnub_free(pbp), equals(-1));
}

Ensure(single_context_pubnub, subscribe_reestablishing_broken_keep_alive_conection) {
    pubnub_init(pbp, "publ-key", "sub-Key");

    pubnub_set_uuid(pbp, "admin");
    pubnub_set_auth(pbp, "msgs");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/subscribe/sub-Key/[ch1,ch2]/0/0?pnsdk=unit-test-0.1&channel-group=[chgr2,chgr3,chgr4]&uuid=admin&auth=msgs");
    incoming("HTTP/1.1 200\r\nContent-Length: 26\r\n\r\n[[],\"3516149789251234578\"]");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(pbp, "[ch1,ch2]", "[chgr2,chgr3,chgr4]"), equals(PNR_OK));

    attest(pubnub_get_channel(pbp), streqs(NULL));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
    
    /* Sending GET request returns failure which means broken connection */
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect(pbpal_send, when(data, streqs("GET ")), returns(-1));
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    
    /* Renewing DNS resolution and reestablishing connection */
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/subscribe/sub-Key/[ch1,ch2]/0/3516149789251234578?pnsdk=unit-test-0.1&channel-group=[chgr2,chgr3,chgr4]&uuid=admin&auth=msgs");
    incoming("HTTP/1.1 200\r\nContent-Length: 150\r\n\r\n[[msg1,msg2,{\"text\":\"Hello World!\"},msg4,msg5,{\"key\":\"val\\ue\"}],\"352624978925123458\",\"chgr4,chgr2,chgr3,chgr4,chgr7,chgr4\",\"ch5,ch8,ch6,ch17,ch1,ch2\"]");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(pbp, "[ch1,ch2]", "[chgr2,chgr3,chgr4]"), equals(PNR_OK));

    attest(pubnub_get_channel(pbp), streqs("ch5"));
    attest(pubnub_get_channel(pbp), streqs("ch8"));
    attest(pubnub_get_channel(pbp), streqs("ch6"));
    attest(pubnub_get_channel(pbp), streqs("ch17"));
    attest(pubnub_get_channel(pbp), streqs("ch1"));
    attest(pubnub_get_channel(pbp), streqs("ch2"));
    attest(pubnub_get_channel(pbp), streqs(NULL));
    attest(pubnub_get(pbp), streqs("msg1"));
    attest(pubnub_get(pbp), streqs("msg2"));
    attest(pubnub_get(pbp), streqs("{\"text\":\"Hello World!\"}"));
    attest(pubnub_get(pbp), streqs("msg4"));
    attest(pubnub_get(pbp), streqs("msg5"));
    attest(pubnub_get(pbp), streqs("{\"key\":\"val\\ue\"}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
    
    /* connection keeps alive this time */
    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/subscribe/sub-Key/ch17/0/352624978925123458?pnsdk=unit-test-0.1&uuid=admin&auth=msgs");
    incoming("HTTP/1.1 200\r\nContent-Length: 47\r\n\r\n[[message],\"352624979925123457\",\"chgr4\",\"ch17\"]");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(pbp, "ch17", NULL), equals(PNR_OK));
    attest(pubnub_subscribe(pbp, NULL, "chgr2"), equals(PNR_RX_BUFF_NOT_EMPTY));

    attest(pubnub_get(pbp), streqs("message"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), streqs("ch17"));
    attest(pubnub_get_channel(pbp), streqs(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_free(pbp), equals(-1));
}

Ensure(single_context_pubnub, subscribe_not_using_keep_alive_connection) {
    pubnub_init(pbp, "publ-key", "sub-Key");

    /* Won't be using default 'keep-alive' connection */
    pubnub_dont_use_http_keep_alive(pbp);

    pubnub_set_uuid(pbp, "admin");
    pubnub_set_auth(pbp, "msgs");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/subscribe/sub-Key/[ch1,ch2]/0/0?pnsdk=unit-test-0.1&channel-group=[chgr2,chgr3,chgr4]&uuid=admin&auth=msgs");
    incoming_and_close("HTTP/1.1 200\r\nContent-Length: 26\r\n\r\n[[],\"3516149789251234578\"]");
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(pbp, "[ch1,ch2]", "[chgr2,chgr3,chgr4]"), equals(PNR_OK));

    attest(pubnub_get_channel(pbp), streqs(NULL));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
    expect(pbpal_free, when(pb, equals(pbp)));
    attest(pubnub_free(pbp), equals(0));
}

Ensure(single_context_pubnub, subscribe_not_using_and_than_using_keep_alive_connection) {
    pubnub_init(pbp, "publ-key", "sub-Key");

    /* Won't be using default 'keep-alive' connection */
    pubnub_dont_use_http_keep_alive(pbp);

    pubnub_set_uuid(pbp, "admin");
    pubnub_set_auth(pbp, "msgs");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/subscribe/sub-Key/[ch1,ch2]/0/0?pnsdk=unit-test-0.1&channel-group=[chgr2,chgr3,chgr4]&uuid=admin&auth=msgs");
    incoming_and_close("HTTP/1.1 200\r\nContent-Length: 26\r\n\r\n[[],\"3516149789251234578\"]");
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(pbp, "[ch1,ch2]", "[chgr2,chgr3,chgr4]"), equals(PNR_OK));

    attest(pubnub_get_channel(pbp), streqs(NULL));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
    
    /* After finishing following transaction switches connection to 'keep_alive' */
    pubnub_use_http_keep_alive(pbp);
    /* Renewing DNS resolution with new request */
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/subscribe/sub-Key/[ch1,ch2]/0/3516149789251234578?pnsdk=unit-test-0.1&channel-group=[chgr2,chgr3,chgr4]&uuid=admin&auth=msgs");
    incoming("HTTP/1.1 200\r\nContent-Length: 150\r\n\r\n[[msg1,msg2,{\"text\":\"Hello World!\"},msg4,msg5,{\"key\":\"val\\ue\"}],\"352624978925123458\",\"chgr4,chgr2,chgr3,chgr4,chgr7,chgr4\",\"ch5,ch8,ch6,ch17,ch1,ch2\"]");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(pbp, "[ch1,ch2]", "[chgr2,chgr3,chgr4]"), equals(PNR_OK));

    attest(pubnub_get_channel(pbp), streqs("ch5"));
    attest(pubnub_get_channel(pbp), streqs("ch8"));
    attest(pubnub_get_channel(pbp), streqs("ch6"));
    attest(pubnub_get_channel(pbp), streqs("ch17"));
    attest(pubnub_get_channel(pbp), streqs("ch1"));
    attest(pubnub_get_channel(pbp), streqs("ch2"));
    attest(pubnub_get_channel(pbp), streqs(NULL));
    attest(pubnub_get(pbp), streqs("msg1"));
    attest(pubnub_get(pbp), streqs("msg2"));
    attest(pubnub_get(pbp), streqs("{\"text\":\"Hello World!\"}"));
    attest(pubnub_get(pbp), streqs("msg4"));
    attest(pubnub_get(pbp), streqs("msg5"));
    attest(pubnub_get(pbp), streqs("{\"key\":\"val\\ue\"}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));

    /* connection keeps alive */
    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/subscribe/sub-Key/ch17/0/352624978925123458?pnsdk=unit-test-0.1&uuid=admin&auth=msgs");
    incoming("HTTP/1.1 200\r\nContent-Length: 47\r\n\r\n[[message],\"352624979925123457\",\"chgr4\",\"ch17\"]");
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(pbp, "ch17", NULL), equals(PNR_OK));
    attest(pubnub_subscribe(pbp, NULL, "chgr2"), equals(PNR_RX_BUFF_NOT_EMPTY));

    attest(pubnub_get(pbp), streqs("message"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), streqs("ch17"));
    attest(pubnub_get_channel(pbp), streqs(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_free(pbp), equals(-1));
    
    /* Expires connection timer */
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    pbnc_stop(pbp, PNR_TIMEOUT);
}

/* Verify ASSERT gets fired */

Ensure(single_context_pubnub, illegal_context_fires_assert) {
    expect_assert_in(pubnub_init(NULL, "k", "u"), "pubnub_pubsubapi.c");
    expect_assert_in(pubnub_publish(NULL, "x", "0"), "pubnub_pubsubapi.c");
    expect_assert_in(pubnub_subscribe(NULL, "x", NULL), "pubnub_pubsubapi.c");
    expect_assert_in(pubnub_leave(NULL, "x", NULL), "pubnub_coreapi.c");
    expect_assert_in(pubnub_cancel(NULL), "pubnub_pubsubapi.c");
    expect_assert_in(pubnub_set_uuid(NULL, ""), "pubnub_pubsubapi.c");
    expect_assert_in(pubnub_set_auth(NULL, ""), "pubnub_pubsubapi.c");
    expect_assert_in(pubnub_last_http_code(NULL), "pubnub_pubsubapi.c");
    expect_assert_in(pubnub_get(NULL), "pubnub_pubsubapi.c");
    expect_assert_in(pubnub_get_channel(NULL), "pubnub_pubsubapi.c");
    expect_assert_in(pubnub_set_state(NULL, "y", "group", NULL, "{}"), "pubnub_coreapi.c");
    expect_assert_in(pubnub_state_get(NULL, "y", "group", NULL), "pubnub_coreapi.c");
    expect_assert_in(pubnub_here_now(NULL, "ch", "ch_group"), "pubnub_coreapi.c");
    expect_assert_in(pubnub_global_here_now(NULL), "pubnub_coreapi.c");
    expect_assert_in(pubnub_heartbeat(NULL, "25", "[37,0Rh-]"), "pubnub_coreapi.c");
    expect_assert_in(pubnub_where_now(NULL, "uuid"), "pubnub_coreapi.c");
    expect_assert_in(pubnub_where_now(pbp, NULL), "pubnub_ccore.c");
    expect_assert_in(pubnub_add_channel_to_group(NULL, "ch", "group"), "pubnub_coreapi.c");
    expect_assert_in(pubnub_add_channel_to_group(pbp, NULL, "group"), "pubnub_ccore.c");
    expect_assert_in(pubnub_add_channel_to_group(pbp, "ch", NULL), "pubnub_ccore.c");
    expect_assert_in(pubnub_remove_channel_from_group(NULL, "ch", "group"), "pubnub_coreapi.c");
    expect_assert_in(pubnub_remove_channel_from_group(pbp, NULL, "group"), "pubnub_ccore.c");
    expect_assert_in(pubnub_remove_channel_from_group(pbp, "ch", NULL), "pubnub_ccore.c");
    expect_assert_in(pubnub_remove_channel_group(NULL, "group"), "pubnub_coreapi.c");
    expect_assert_in(pubnub_remove_channel_group(pbp, NULL), "pubnub_ccore.c");
    expect_assert_in(pubnub_list_channel_group(NULL, "group"), "pubnub_coreapi.c");
    expect_assert_in(pubnub_list_channel_group(pbp, NULL), "pubnub_ccore.c");
    expect_assert_in(pubnub_subscribe(NULL, "%22cheesy%22", "[milk, meat]"), "pubnub_pubsubapi.c");
    expect_assert_in(pubnub_free((pubnub_t*)((char*)pbp + 10000)), "pubnub_alloc_static.c");
}


#if 0
int main(int argc, char *argv[])
{
    TestSuite *suite = create_test_suite();
    add_test_with_context(suite, single_context_pubnub, time_in_progress);    
    run_test_suite(suite, create_text_reporter());
}
#endif
