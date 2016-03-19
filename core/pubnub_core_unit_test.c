/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "cgreen/cgreen.h"
#include "cgreen/mocks.h"

#include "pubnub_coreapi.h"
#include "pubnub_assert.h"
#include "pubnub_alloc.h"
#include "pubnub_log.h"

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


/* The Pubnub PAL mocks and stubs */

static void buf_setup(pubnub_t *pb)
{
    pb->ptr = (uint8_t*)pb->core.http_buf;
    pb->left = sizeof pb->core.http_buf;
}

void pbpal_init(pubnub_t *pb)
{
}

enum pubnub_res pbpal_resolv_and_connect(pubnub_t *pb)
{
    return (int)mock(pb);
}

int pbntf_got_socket(pubnub_t *pb, pb_socket_t socket)
{
    return (int)mock(pb, socket);
}


void pbntf_update_socket(pubnub_t *pb, pb_socket_t socket)
{
    mock(pb, socket);
}


enum pubnub_res pbpal_check_resolv_and_connect(pubnub_t *pb)
{
    return (int)mock(pb);
}

bool pbpal_connected(pubnub_t *pb)
{
    return (bool)mock(pb);
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


int pbpal_send(pubnub_t *pb, void *data, size_t n)
{
    return (int)mock(pb, data, n);
}

int pbpal_send_str(pubnub_t *pb, char *s)
{
    return (int)mock(pb, s);
}


int pbpal_send_status(pubnub_t *pb)
{
    return (bool)mock(pb);
}


int pbpal_start_read_line(pubnub_t *pb)
{
    if (pb->sock_state != STATE_NONE) {
        PUBNUB_LOG_TRACE("pbpal_start_read_line(): pb->sock_state != STATE_NONE: "); WATCH_ENUM(pb->sock_state);
        return -1;
    }

    if (pb->ptr > (uint8_t*)pb->core.http_buf) {
        unsigned distance = pb->ptr - (uint8_t*)pb->core.http_buf;
        memmove(pb->core.http_buf, pb->ptr, pb->readlen);
        pb->ptr -= distance;
        pb->left += distance;
    }
    else {
        if (pb->left == 0) {
            /* Obviously, our buffer is not big enough, maybe some
               error should be reported */
            buf_setup(pb);
        }
    }

    pb->sock_state = STATE_READ_LINE;
    return +1;
}

static char const *m_read;

int pbpal_read_len(pubnub_t *pb)
{
    return sizeof pb->core.http_buf - pb->left;
}

static int my_recv(void *p, size_t n)
{
    int to_read;

    attest(m_read, differs(NULL));
    if (m_read[0] == '\0') {
        return 0;
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
    uint8_t c;

    if (pb->readlen == 0) {
        int recvres = my_recv(pb->ptr, pb->left);
        if (recvres < 0) {
            return PNR_IO_ERROR;
        }
        pb->sock_state = STATE_READ_LINE;
        pb->readlen = recvres;
    }

    while (pb->left > 0 && pb->readlen > 0) {
        c = *pb->ptr++;

        --pb->readlen;
        --pb->left;
        
        if (c == '\n') {
            pb->sock_state = STATE_NONE;
            return PNR_OK;
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

    return PNR_IN_PROGRESS;
}


int pbpal_start_read(pubnub_t *pb, size_t n)
{
    if (pb->sock_state != STATE_NONE) {
        PUBNUB_LOG_TRACE("pbpal_start_read(): pb->sock_state != STATE_NONE: "); WATCH_ENUM(pb->sock_state);
        return -1;
    }
    if (pb->ptr > (uint8_t*)pb->core.http_buf) {
        unsigned distance = pb->ptr - (uint8_t*)pb->core.http_buf;
        memmove(pb->core.http_buf, pb->ptr, pb->readlen);
        pb->ptr -= distance;
        pb->left += distance;
    }
    else {
        if (pb->left == 0) {
            /* Obviously, our buffer is not big enough, maybe some
               error should be reported */
            buf_setup(pb);
        }
    }
    pb->sock_state = STATE_READ;
    pb->len = n;
    return +1;
}


bool pbpal_read_over(pubnub_t *pb)
{
    unsigned to_read = 0;

    if (pb->readlen == 0) {
        int recvres;
        to_read =  pb->len - pbpal_read_len(pb);
        if (to_read > pb->left) {
            to_read = pb->left;
        }
        recvres = my_recv(pb->ptr, to_read);
        if (recvres <= 0) {
            /* This is error or connection close, which may be handled
               in some way...
             */
            return false;
        }
        pb->sock_state = STATE_READ;
        pb->readlen = recvres;
    } 

    to_read = pb->len;
    if (pb->readlen < to_read) {
        to_read = pb->readlen;
    }
    pb->ptr += to_read;
    pb->readlen -= to_read;
    PUBNUB_ASSERT_OPT(pb->left >= to_read);
    pb->left -= to_read;
    pb->len -= to_read;

    if (pb->len == 0) {
        pb->sock_state = STATE_NONE;
        return true;
    }

    if (pb->left == 0) {
        /* Buffer has been filled, but the requested block has not been
         * read.  We have to "reset" this "mini-fsm", as otherwise we
         * won't read anything any more. This means that we have lost
         * the current contents of the buffer, which is bad. In some
         * general code, that should be reported, as the caller could
         * save the contents of the buffer somewhere else or simply
         * decide to ignore this block (when it does end eventually).
         */
        pb->sock_state = STATE_NONE;
    }
    else {
        pb->sock_state = STATE_NEWDATA_EXHAUSTED;
        return false;
    }

    return true;
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
    if (0 == val) { expr; }                     \
    else { attest(!m_expect_assert); }          \
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
    pubnub_assert_set_handler(assert_handler);
    m_read = NULL;
    pbp = pubnub_alloc();
    attest(pbp, differs(NULL));

}

AfterEach(single_context_pubnub) {
    attest(pubnub_free(pbp), equals(0));
}


void expect_have_dns_for_pubnub_origin()
{
    expect(pbpal_resolv_and_connect, when(pb, equals(pbp)), returns(PNR_OK));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
}


inline void expect_outgoing_with_url(char const *url) {
    expect(pbpal_send, when(data, streqs("GET ")), returns(0));
    expect(pbpal_send_status, returns(0));
    expect(pbpal_send_str, when(s, streqs(url)), returns(0));
    expect(pbpal_send_status, returns(0));
    expect(pbpal_send, when(data, streqs(" HTTP/1.1\r\nHost: ")), returns(0));
    expect(pbpal_send_status, returns(0));
    expect(pbpal_send_str, when(s, streqs(PUBNUB_ORIGIN)), returns(0));
    expect(pbpal_send_status, returns(0));
    expect(pbpal_send, when(data, streqs("\r\nUser-Agent: PubNub-C-core/2.1\r\nConnection: Keep-Alive\r\n\r\n")), returns(0));
    expect(pbpal_send_status, returns(0));
}


inline void incoming(char const *str) {
    m_read = str;
}


inline void incoming_and_close(char const *str) {
    incoming(str);
    expect(pbpal_close, when(pb, equals(pbp)), returns(0));
//    expect(pbpal_closed, when(pb, equals(pbp)), returns(true));
    expect(pbpal_forget, when(pb, equals(pbp)));
}


static void cancel_and_cleanup(pubnub_t *pbp)
{
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
    incoming_and_close("HTTP/1.1 200\r\nContent-Length: 2\r\n\r\n{}");
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_leave(pbp, "lamanche", NULL), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));
}


/* This tests the DNS resolution code. Since we know for sure it is
   the same for all Pubnub operations/transactions, we shall test it
   only for "leave".
*/
Ensure(single_context_pubnub, leave_wait_dns) {
    pubnub_init(pbp, "pubkey", "subkey");

    /* DNS resolution not yet available... */
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(+1));
    expect(pbpal_resolv_and_connect, when(pb, equals(pbp)), returns(PNR_STARTED));
    attest(pubnub_leave(pbp, "lamanche", NULL), equals(PNR_STARTED));

    /* ... still not available... */
    expect(pbpal_check_resolv_and_connect, when(pb, equals(pbp)), returns(PNR_STARTED));
    attest(pbnc_fsm(pbp), equals(0));

    /* ... and here it is: */
    expect(pbpal_check_resolv_and_connect, when(pb, equals(pbp)), returns(PNR_OK));
    expect(pbntf_update_socket, when(pb, equals(pbp)));
    expect_outgoing_with_url("/v2/presence/sub-key/subkey/channel/lamanche/leave?pnsdk=unit-test-0.1");
    incoming_and_close("HTTP/1.1 200\r\nContent-Length: 2\r\n\r\n{}");
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pbnc_fsm(pbp), equals(0));

    attest(pbp->core.last_result, equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));
}


Ensure(single_context_pubnub, leave_wait_dns_cancel) {
    pubnub_init(pbp, "pubkey", "subkey");

    /* DNS resolution not yet available... */
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(+1));
    expect(pbpal_resolv_and_connect, when(pb, equals(pbp)), returns(PNR_STARTED));
    attest(pubnub_leave(pbp, "lamanche", NULL), equals(PNR_STARTED));

    /* ... user is impatient... */
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
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
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(+1));
    expect(pbpal_resolv_and_connect, when(pb, equals(pbp)), returns(PNR_STARTED));
    attest(pubnub_leave(pbp, "lamanche", NULL), equals(PNR_STARTED));

    /* ... and here it is: */
    expect(pbpal_check_resolv_and_connect, when(pb, equals(pbp)), returns(PNR_OK));
    expect(pbntf_update_socket, when(pb, equals(pbp)));
    expect_outgoing_with_url("/v2/presence/sub-key/subkey/channel/lamanche/leave?pnsdk=unit-test-0.1");
    incoming_and_close("HTTP/1.1 200\r\nContent-Length: 2\r\n\r\n{}");
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pbnc_fsm(pbp), equals(0));

    attest(pbp->core.last_result, equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));
}





Ensure(single_context_pubnub, leave_wait_tcp_cancel) {
    pubnub_init(pbp, "pubkey", "subkey");

    /* DNS resolved but TCP connection not yet established... */
    expect(pbpal_resolv_and_connect, when(pb, equals(pbp)), returns(PNR_IN_PROGRESS));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect(pbpal_connected, when(pb, equals(pbp)), returns(false));
    attest(pubnub_leave(pbp, "lamanche", NULL), equals(PNR_STARTED));

    /* ... user is impatient... */
    cancel_and_cleanup(pbp);
}


Ensure(single_context_pubnub, leave_changroup) {
    pubnub_init(pbp, "kpub", "ssub");

    /* Both channel and channel group set */
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/ssub/channel/k1/leave?pnsdk=unit-test-0.1&channel-group=tnt");
    incoming_and_close("HTTP/1.1 200\r\nContent-Length: 2\r\n\r\n{}");
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_leave(pbp, "k1", "tnt"), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));

    /* Only channel group set */
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/ssub/channel/,/leave?pnsdk=unit-test-0.1&channel-group=mala");
    incoming_and_close("HTTP/1.1 200\r\nContent-Length: 2\r\n\r\n{}");
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_leave(pbp, NULL, "mala"), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));

    /* Neither channel nor channel group set */
    attest(pubnub_leave(pbp, NULL, NULL), equals(PNR_INVALID_CHANNEL));
}


Ensure(single_context_pubnub, leave_uuid_auth) {
    pubnub_init(pbp, "pubX", "Xsub");

    /* Set UUID */
    pubnub_set_uuid(pbp, "DEDA-BABACECA-DECA");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/Xsub/channel/k/leave?pnsdk=unit-test-0.1&uuid=DEDA-BABACECA-DECA");
    incoming_and_close("HTTP/1.1 200\r\nContent-Length: 2\r\n\r\n{}");
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_leave(pbp, "k", NULL), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));

    /* Set auth, too */
    pubnub_set_auth(pbp, "super-secret-key");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/Xsub/channel/k2/leave?pnsdk=unit-test-0.1&uuid=DEDA-BABACECA-DECA&auth=super-secret-key");
    incoming_and_close("HTTP/1.1 200\r\nContent-Length: 2\r\n\r\n{}");
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_leave(pbp, "k2", NULL), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));

    /* Reset UUID */
    pubnub_set_uuid(pbp, NULL);
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/Xsub/channel/k3/leave?pnsdk=unit-test-0.1&auth=super-secret-key");
    incoming_and_close("HTTP/1.1 200\r\nContent-Length: 2\r\n\r\n{}");
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_leave(pbp, "k3", NULL), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));

    /* Reset auth, too */
    pubnub_set_auth(pbp, NULL);
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/Xsub/channel/k4/leave?pnsdk=unit-test-0.1");
    incoming_and_close("HTTP/1.1 200\r\nContent-Length: 2\r\n\r\n{}");
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_leave(pbp, "k4", NULL), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));
}


Ensure(single_context_pubnub, leave_bad_response) {
    pubnub_init(pbp, "pubkey", "subkey");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/subkey/channel/lamanche/leave?pnsdk=unit-test-0.1");
    incoming_and_close("HTTP/1.1 200\r\nContent-Length: 2\r\n\r\n[]");
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_leave(pbp, "lamanche", NULL), equals(PNR_FORMAT_ERROR));
}


Ensure(single_context_pubnub, leave_in_progress) {
    pubnub_init(pbp, "pubkey", "subkey");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/subkey/channel/lamanche/leave?pnsdk=unit-test-0.1");
    incoming("HTTP/1.1 200\r\n");
    attest(pubnub_leave(pbp, "lamanche", NULL), equals(PNR_STARTED));
    attest(pubnub_leave(pbp, "lamanche", NULL), equals(PNR_IN_PROGRESS));

    cancel_and_cleanup(pbp);
}


/* -- TIME operation -- */


Ensure(single_context_pubnub, time) {
    pubnub_init(pbp, "tkey", "subt");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/time/0?pnsdk=unit-test-0.1");
    incoming_and_close("HTTP/1.1 200\r\nContent-Length: 9\r\n\r\n[1643092]");
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_time(pbp), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_get(pbp), streqs("1643092"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), equals(NULL));
}


Ensure(single_context_pubnub, time_bad_response) {
    pubnub_init(pbp, "tkey", "subt");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/time/0?pnsdk=unit-test-0.1");
    incoming_and_close("HTTP/1.1 200\r\nContent-Length: 9\r\n\r\n{1643092}");
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_time(pbp), equals(PNR_FORMAT_ERROR));

    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), equals(NULL));
}


Ensure(single_context_pubnub, time_in_progress) {
    pubnub_init(pbp, "pubkey", "subkey");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/time/0?pnsdk=unit-test-0.1");
    incoming("HTTP/1.1 200\r\n");
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
    incoming_and_close("HTTP/1.1 200\r\nContent-Length: 30\r\n\r\n[1,\"Sent\",\"14178940800777403\"]");
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_publish(pbp, "jarak", "\"zec\""), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));
}


Ensure(single_context_pubnub, publish_http_chunked) {
    pubnub_init(pbp, "publkey", "subkey");

    expect_have_dns_for_pubnub_origin();

    expect_outgoing_with_url("/publish/publkey/subkey/0/jarak/0/%22zec%22?pnsdk=unit-test-0.1");
    incoming_and_close("HTTP/1.1 200\r\nTransfer-Encoding: chunked\r\n\r\n12\r\n[1,\"Sent\",\"1417894\r\n0C\r\n0800777403\"]\r\n0\r\n");
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_publish(pbp, "jarak", "\"zec\""), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));
}


Ensure(single_context_pubnub, http_headers_no_content_length_or_chunked) {
    pubnub_init(pbp, "publkey", "subkey");

    expect_have_dns_for_pubnub_origin();

    expect_outgoing_with_url("/publish/publkey/subkey/0/,/0/%22zec%22?pnsdk=unit-test-0.1");
    incoming_and_close("HTTP/1.1 400\r\n\r\n[0,\"Invalid\",\"14178940800999505\"]");
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_publish(pbp, ",", "\"zec\""), equals(PNR_IO_ERROR));
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
}


Ensure(single_context_pubnub, publish_in_progress) {
    pubnub_init(pbp, "pubkey", "subkey");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/publish/pubkey/subkey/0/jarak/0/4443?pnsdk=unit-test-0.1");
    incoming("HTTP/1.1 200\r\n");
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
    incoming_and_close("HTTP/1.1 200\r\nContent-Length: 3\r\n\r\n[1]");
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_publish(pbp, "k", "4443"), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));

    /* Set auth, too */
    pubnub_set_auth(pbp, "bad-secret-key");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/publish/pubX/Xsub/0/k2/0/443?pnsdk=unit-test-0.1&uuid=0ADA-BEDA-0000&auth=bad-secret-key");
    incoming_and_close("HTTP/1.1 200\r\nContent-Length: 3\r\n\r\n[1]");
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_publish(pbp, "k2", "443"), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));

    /* Reset UUID */
    pubnub_set_uuid(pbp, NULL);
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/publish/pubX/Xsub/0/k3/0/4443?pnsdk=unit-test-0.1&auth=bad-secret-key");
    incoming_and_close("HTTP/1.1 200\r\nContent-Length: 3\r\n\r\n[1]");
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_publish(pbp, "k3", "4443"), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));

    /* Reset auth, too */
    pubnub_set_auth(pbp, NULL);
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/publish/pubX/Xsub/0/k4/0/443?pnsdk=unit-test-0.1");
    incoming_and_close("HTTP/1.1 200\r\nContent-Length: 3\r\n\r\n[1]");
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_publish(pbp, "k4", "443"), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));
}


Ensure(single_context_pubnub, publish_bad_response) {
    pubnub_init(pbp, "tkey", "subt");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/publish/tkey/subt/0/k6/0/443?pnsdk=unit-test-0.1");
    incoming_and_close("HTTP/1.1 200\r\nContent-Length: 9\r\n\r\n{\"1\":\"X\"}");
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_publish(pbp, "k6", "443"), equals(PNR_FORMAT_ERROR));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), equals(NULL));
}


/* -- HISTORY operation -- */


Ensure(single_context_pubnub, history) {
    pubnub_init(pbp, "publhis", "subhis");

    /* Without time-token */
    expect_have_dns_for_pubnub_origin();

    expect_outgoing_with_url("/v2/history/sub-key/subhis/channel/ch?pnsdk=unit-test-0.1&count=22&include_token=false");
    incoming_and_close("HTTP/1.1 200\r\nContent-Length: 45\r\n\r\n[[1,2,3],14370854953886727,14370864554607266]");
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_history(pbp, "ch", 22, false), equals(PNR_OK));

    attest(pubnub_get(pbp), streqs("[1,2,3]"));
    attest(pubnub_get(pbp), streqs("14370854953886727"));
    attest(pubnub_get(pbp), streqs("14370864554607266"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));

    /* With time-token */
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/history/sub-key/subhis/channel/ch?pnsdk=unit-test-0.1&count=22&include_token=true");
    incoming_and_close("HTTP/1.1 200\r\nContent-Length: 171\r\n\r\n[[{\"message\":1,\"timetoken\":14370863460777883},{\"message\":2,\"timetoken\":14370863461279046},{\"message\":3,\"timetoken\":14370863958459501}],14370863460777883,14370863958459501]");
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_history(pbp, "ch", 22, true), equals(PNR_OK));

    attest(pubnub_get(pbp), streqs("[{\"message\":1,\"timetoken\":14370863460777883},{\"message\":2,\"timetoken\":14370863461279046},{\"message\":3,\"timetoken\":14370863958459501}]"));
    attest(pubnub_get(pbp), streqs("14370863460777883"));
    attest(pubnub_get(pbp), streqs("14370863958459501"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
}


Ensure(single_context_pubnub, history_in_progress) {
    pubnub_init(pbp, "publhis", "subhis");

    expect_have_dns_for_pubnub_origin();

    expect_outgoing_with_url("/v2/history/sub-key/subhis/channel/ch?pnsdk=unit-test-0.1&count=22&include_token=false");
    incoming("HTTP/1.1 200\r\n");
    attest(pubnub_history(pbp, "ch", 22, false), equals(PNR_STARTED));
    attest(pubnub_history(pbp, "x", 55, false), equals(PNR_IN_PROGRESS));

    cancel_and_cleanup(pbp);
}


Ensure(single_context_pubnub, history_auth) {
    pubnub_init(pbp, "pubX", "Xsub");

    pubnub_set_auth(pbp, "go-secret-key");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/history/sub-key/Xsub/channel/hhh?pnsdk=unit-test-0.1&auth=go-secret-key&count=40&include_token=false");
    incoming_and_close("HTTP/1.1 200\r\nContent-Length: 2\r\n\r\n[]");
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_history(pbp, "hhh", 40, false), equals(PNR_OK));

    attest(pubnub_last_http_code(pbp), equals(200));
}


Ensure(single_context_pubnub, history_bad_response) {
    pubnub_init(pbp, "pubkey", "Xsub");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/history/sub-key/Xsub/channel/ttt?pnsdk=unit-test-0.1&count=10&include_token=false");
    incoming_and_close("HTTP/1.1 200\r\nContent-Length: 2\r\n\r\n{}");
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_history(pbp, "ttt", 10, false), equals(PNR_FORMAT_ERROR));
}


/* Verify ASSERT gets fired */

Ensure(single_context_pubnub, illegal_context_fires_assert) {
    expect_assert_in(pubnub_init(NULL, "k", "u"), "pubnub_coreapi.c");
    expect_assert_in(pubnub_publish(NULL, "x", "0"), "pubnub_coreapi.c");
    expect_assert_in(pubnub_subscribe(NULL, "x", NULL), "pubnub_coreapi.c");
    expect_assert_in(pubnub_leave(NULL, "x", NULL), "pubnub_coreapi.c");
    expect_assert_in(pubnub_cancel(NULL), "pubnub_coreapi.c");
    expect_assert_in(pubnub_set_uuid(NULL, ""), "pubnub_coreapi.c");
    expect_assert_in(pubnub_set_auth(NULL, ""), "pubnub_coreapi.c");
    expect_assert_in(pubnub_last_http_code(NULL), "pubnub_coreapi.c");
    expect_assert_in(pubnub_get(NULL), "pubnub_coreapi.c");
    expect_assert_in(pubnub_get_channel(NULL), "pubnub_coreapi.c");

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
