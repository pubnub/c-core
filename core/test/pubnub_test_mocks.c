/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "cgreen/cgreen.h"
#include "cgreen/constraint_syntax_helpers.h"
#include "cgreen/mocks.h"
#include "pubnub_test_mocks.h"

#include "pubnub_alloc.h"
#include "pubnub_grant_token_api.h"
#include "pubnub_pubsubapi.h"
#include "pubnub_internal.h"
#include "pubnub_internal_common.h"
#include "pubnub_assert.h"
#include "pubnub_log.h"

#include "pbpal.h"
#include "pubnub_version_internal.h"
#include "pubnub_keep_alive.h"
#include "test/pubnub_test_helper.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <assert.h>

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

/* Current (simulated) string message received */
static const char* m_read;
/* Number of simulated string messages received */
static int m_num_string_msgs_rcvd;
/* Number of 'uint8_t' data blocks received*/
static int m_num_uint8_data_blocks;
/* Array of simulated 'receive' messages */
static char* m_string_msg_array[20];
/* Array of simulated 'receive' messages */
static struct uint8_block* m_uint8_data_block_array[20];
/* Index(in the array) of the current string message used while receiving*/
static int m_i;
/* Index(in the array) of the current 'uint8_t' data block used while
 * receiving*/
static int m_j;
/* bit masks array for advancing through string or uint8blocks arrays while
 * receiving */
uint8_t string_or_uint8block_mask[10];

/* Awaits given amount of time in seconds */
void wait_time_in_seconds(time_t time_in_seconds)
{
    time_t time_start = time(NULL);
    do {
    } while ((time(NULL) - time_start) < time_in_seconds);
    return;
}


/* The Pubnub NTF mocks and stubs */
void pbntf_trans_outcome(pubnub_t* pb, enum pubnub_state state)
{
    pb->state = state;
    mock(pb);
}

int pbntf_got_socket(pubnub_t* pb)
{
    return (int)mock(pb);
}

void pbntf_lost_socket(pubnub_t* pb)
{
    mock(pb);
}

void pbntf_update_socket(pubnub_t* pb)
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

int pbntf_requeue_for_processing(pubnub_t* pb)
{
    return (int)mock(pb);
}

int pbntf_enqueue_for_processing(pubnub_t* pb)
{
    return (int)mock(pb);
}

int pbntf_watch_out_events(pubnub_t* pb)
{
    return (int)mock(pb);
}

int pbntf_watch_in_events(pubnub_t* pb)
{
    return (int)mock(pb);
}

/* The Pubnub PAL mocks and stubs */
static void buf_setup(pubnub_t* pb)
{
    pb->ptr  = (uint8_t*)pb->core.http_buf;
    pb->left = sizeof pb->core.http_buf / sizeof pb->core.http_buf[0];
}

void pbpal_init(pubnub_t* pb)
{
    buf_setup(pb);
}

enum pbpal_resolv_n_connect_result pbpal_resolv_and_connect(pubnub_t* pb)
{
    return (int)mock(pb);
}
enum pbpal_resolv_n_connect_result pbpal_check_resolv_and_connect(pubnub_t* pb)
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

bool pbpal_connected(pubnub_t* pb)
{
    return (bool)mock(pb);
}

void pbpal_free(pubnub_t* pb)
{
    mock(pb);
}

enum pbpal_resolv_n_connect_result pbpal_check_connect(pubnub_t* pb)
{
    return (int)mock(pb);
}

void pbpal_report_error_from_environment(pubnub_t* pb, char const* file, int line)
{
    mock(pb);
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


int pbpal_send(pubnub_t* pb, void const* data, size_t n)
{
    return (int)mock(pb, data, n);
}

int pbpal_send_str(pubnub_t* pb, char const* s)
{
    return (int)mock(pb, s);
}

int pbpal_send_status(pubnub_t* pb)
{
    return (bool)mock(pb);
}

int pbpal_start_read_line(pubnub_t* pb)
{
    unsigned distance;

    PUBNUB_ASSERT_INT_OPT(pb->sock_state, ==, STATE_NONE);

    if (pb->unreadlen > 0) {
        PUBNUB_ASSERT_OPT((char*)(pb->ptr + pb->unreadlen)
                          <= (char*)(pb->core.http_buf + PUBNUB_BUF_MAXLEN));
        memmove(pb->core.http_buf, pb->ptr, pb->unreadlen);
    }
    distance = pb->ptr - (uint8_t*)pb->core.http_buf;
    PUBNUB_ASSERT_UINT((distance + pb->left + pb->unreadlen),
                       ==,
                       (sizeof pb->core.http_buf / sizeof pb->core.http_buf[0]));
    pb->ptr -= distance;
    pb->left += distance;

    pb->sock_state = STATE_READ_LINE;

    return +1;
}

int pbpal_read_len(pubnub_t* pb)
{
    return (char*)pb->ptr - pb->core.http_buf;
}


static int my_recv(void* p, size_t n)
{
    static short m_new = 1;
    int          to_read;
    if (string_or_uint8block_mask[(m_i + m_j) / 8] & (1 << (m_i + m_j) % 8)) {
        if (m_new) {
            assert(m_num_string_msgs_rcvd <= sizeof m_string_msg_array);
            if (m_i < m_num_string_msgs_rcvd) {
                m_read = m_string_msg_array[m_i];
                m_new  = 0;
                assert(m_read != NULL);
                if (strlen(m_read) == 0) {
                    /* an empty string sent(server response simulation),
                     * through function 'incoming',
                     * 'APIs' should interpret as:
                     * ('recv_from_platform_result' < 0) which gives
                     * the oportunity to simulate and test
                     * 'callback' environment in some degree.
                     */
                    ++m_i;
                    m_new = 1;
                    return -1;
                }
            }
            else {
                /* If there is no more 'incoming' string msgs when expected
                 * virtual platform simulates 'connection closed - server
                 * side'(0 bytes received)
                 */
                m_new = 1;
                return 0;
            }
        }
        assert(m_read != NULL);
        to_read = strlen(m_read);
        if (to_read > n) {
            to_read = n;
        }
        if (strlen(m_read) == to_read) {
            ++m_i;
            m_new = 1;
        }
    }
    else {
        if (m_new) {
            assert(m_num_uint8_data_blocks <= sizeof m_uint8_data_block_array);
            if (m_j < m_num_uint8_data_blocks) {
                m_read = (const char*)m_uint8_data_block_array[m_j]->block;
                m_new  = 0;
            }
            else {
                /* If there is no more 'incoming' 'uint8_t' data blocks when
                 * expected virtual platform simulates 'connection closed -
                 * server side'(0 bytes received)
                 */
                m_new = 1;
                return 0;
            }
        }
        assert(m_read != NULL);
        to_read = m_uint8_data_block_array[m_j]->size;
        if (to_read > n) {
            to_read = n;
        }
        if (!(m_uint8_data_block_array[m_j]->size -= to_read)) {
            ++m_j;
            m_new = 1;
        }
    }
    memcpy(p, m_read, to_read);
    m_read += to_read;
    return to_read;
}

enum pubnub_res pbpal_line_read_status(pubnub_t* pb)
{
    uint8_t c;

    PUBNUB_ASSERT_OPT(STATE_READ_LINE == pb->sock_state);

    if (pb->unreadlen == 0) {
        int recvres;
        PUBNUB_ASSERT_OPT((char*)(pb->ptr + pb->left)
                          == (char*)(pb->core.http_buf + PUBNUB_BUF_MAXLEN));
        recvres = my_recv((char*)pb->ptr, pb->left);
        if (recvres < 0) {
            return PNR_IN_PROGRESS;
        }
        else if (0 == recvres) {
            pb->sock_state = STATE_NONE;
            return PNR_TIMEOUT;
        }
        PUBNUB_ASSERT_OPT(recvres <= pb->left);
        PUBNUB_LOG_TRACE(
            "pb=%p have new data of length=%d: %.*s\n", pb, recvres, recvres, pb->ptr);
        pb->unreadlen = recvres;
        pb->left -= recvres;
    }

    while (pb->unreadlen > 0) {
        --pb->unreadlen;

        c = *pb->ptr++;
        if (c == '\n') {
            PUBNUB_LOG_TRACE("pb=%p, newline found, line length: %d, ",
                             pb,
                             pbpal_read_len(pb));
            WATCH_USHORT(pb->unreadlen);
            pb->sock_state = STATE_NONE;
            return PNR_OK;
        }
    }

    if (pb->left == 0) {
        PUBNUB_LOG_ERROR(
            "pbpal_line_read_status(pb=%p): buffer full but newline not found", pb);
        pb->sock_state = STATE_NONE;
        return PNR_TX_BUFF_TOO_SMALL;
    }

    return PNR_IN_PROGRESS;
}

int pbpal_start_read(pubnub_t* pb, size_t n)
{
    unsigned distance;

    PUBNUB_ASSERT_UINT_OPT(n, >, 0);
    PUBNUB_ASSERT_INT_OPT(pb->sock_state, ==, STATE_NONE);

    WATCH_USHORT(pb->unreadlen);
    WATCH_USHORT(pb->left);
    if (pb->unreadlen > 0) {
        PUBNUB_ASSERT_OPT((char*)(pb->ptr + pb->unreadlen)
                          <= (char*)(pb->core.http_buf + PUBNUB_BUF_MAXLEN));
        memmove(pb->core.http_buf, pb->ptr, pb->unreadlen);
    }
    distance = pb->ptr - (uint8_t*)pb->core.http_buf;
    WATCH_UINT(distance);
    PUBNUB_ASSERT_UINT(distance + pb->unreadlen + pb->left,
                       ==,
                       sizeof pb->core.http_buf / sizeof pb->core.http_buf[0]);
    pb->ptr -= distance;
    pb->left += distance;

    pb->sock_state = STATE_READ;
    pb->len        = n;

    return +1;
}

enum pubnub_res pbpal_read_status(pubnub_t* pb)
{
    int have_read;

    PUBNUB_ASSERT_OPT(STATE_READ == pb->sock_state);

    if (0 == pb->unreadlen) {
        unsigned to_recv = pb->len;
        if (to_recv > pb->left) {
            to_recv = pb->left;
        }
        PUBNUB_ASSERT_OPT(to_recv > 0);
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


bool pbpal_closed(pubnub_t* pb)
{
    return (bool)mock(pb);
}

void pbpal_forget(pubnub_t* pb)
{
    mock(pb);
}

int pbpal_close(pubnub_t* pb)
{
    pb->sock_state = STATE_NONE;
    return mock(pb);
}


/* The Pubnub version stubs */

char const* pubnub_sdk_name(void)
{
    return "unit-test";
}

char const* pubnub_version(void)
{
    return "0.1";
}

char const* pubnub_uname(void)
{
    return "unit-test-0.1";
}

char const* pubnub_uagent(void)
{
    return "POSIX-PubNub-C-core/" PUBNUB_SDK_VERSION;
}


/* Assert "catching" */
static bool        m_expect_assert;
static jmp_buf     m_assert_exp_jmpbuf;
static char const* m_expect_assert_file;

int _set_expected_assert(char const* file) {
    m_expect_assert      = true;                                           \
    m_expect_assert_file = file;                                           \

    return setjmp(m_assert_exp_jmpbuf);                    \
} 

void _test_expected_assert() {
    attest(!m_expect_assert);
}

void assert_handler(char const* s, const char* file, long i)
{
    printf("%s:%ld: Pubnub assert failed '%s'\n", file, i, s);
}

void test_assert_handler(char const* s, const char* file, long i)
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

void free_m_msgs(char** msg_array)
{
    int i;

    assert(m_num_string_msgs_rcvd < sizeof m_string_msg_array + 1);

    for (i = 0; i < m_num_string_msgs_rcvd; i++) {
        assert(m_string_msg_array[i] != NULL);
        free(m_string_msg_array[i]);
        m_string_msg_array[i] = NULL;
    }
}

void incoming(char const* str, struct uint8_block* p_data)
{
    if (str != NULL) {
        char* pmsg = malloc(sizeof(char) * (strlen(str) + 1));

        assert(pmsg != NULL);
        assert(m_num_string_msgs_rcvd < sizeof m_string_msg_array);
        /** Marks the bit for string to be read(received) in due time.
            If the bit is 0 uint8_data_block is expected.
         */
        string_or_uint8block_mask[(m_num_string_msgs_rcvd + m_num_uint8_data_blocks) / 8] |=
            1 << (m_num_string_msgs_rcvd + m_num_uint8_data_blocks) % 8;
        strcpy(pmsg, str);
        m_string_msg_array[m_num_string_msgs_rcvd++] = pmsg;
    }
    if (p_data != NULL) {
        assert(m_num_uint8_data_blocks < sizeof m_uint8_data_block_array);
        m_uint8_data_block_array[m_num_uint8_data_blocks++] = p_data;
    }
}

void pubnub_setup_mocks(pubnub_t** pbp) 
{
    pubnub_assert_set_handler((pubnub_assert_handler_t)assert_handler);
    m_read                  = NULL;
    m_num_string_msgs_rcvd  = 0;
    m_num_uint8_data_blocks = 0;
    m_i                     = 0;
    m_j                     = 0;
    *pbp                     = pubnub_alloc();
    assert(*pbp != NULL);
}

void pubnub_cleanup_mocks(pubnub_t* pbp) 
{
    if (pbp->state != PBS_IDLE) {
        expect(pbpal_close, when(pb, equals(pbp)), returns(0));
        expect(pbpal_closed, when(pb, equals(pbp)), returns(true));
        expect(pbpal_forget, when(pb, equals(pbp)));
        expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    }
    expect(pbpal_free, when(pb, equals(pbp)));
    attest(pubnub_free(pbp), equals(0));
    free_m_msgs(m_string_msg_array);
}

void expect_have_dns_for_pubnub_origin_on_ctx(pubnub_t* pbp)
{
    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbpal_resolv_and_connect,
           when(pb, equals(pbp)),
           returns(pbpal_connect_success));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
}

void expect_outgoing_with_url_on_ctx(pubnub_t* pbp, char const* url)
{
    expect(pbpal_send_str, when(s, streqs("GET ")), returns(0));
    expect(pbpal_send_status, returns(0));
    expect(pbpal_send_str, when(s, streqs(url)), returns(0));
    expect(pbpal_send_status, returns(0));
    expect(pbpal_send, when(data, streqs(" HTTP/1.1\r\nHost: ")), returns(0));
    expect(pbpal_send_status, returns(0));
    expect(pbpal_send_str, when(s, streqs(PUBNUB_ORIGIN)), returns(0));
    expect(pbpal_send_status, returns(0));
    expect(pbpal_send_str,
           when(s,
                streqs("\r\nUser-Agent: POSIX-PubNub-C-core/" PUBNUB_SDK_VERSION
                       "\r\n" ACCEPT_ENCODING "\r\n")),
           returns(0));
    expect(pbpal_send_status, returns(0));
    expect(pbntf_watch_in_events, when(pb, equals(pbp)), returns(0));
}

void expect_outgoing_with_url_no_params_on_ctx(pubnub_t* pbp, char const* url)
{
    expect(pbpal_send_str, when(s, streqs("GET ")), returns(0));
    expect(pbpal_send_status, returns(0));
    expect(pbpal_send_str, when(s, contains_string(url)), returns(0));
    expect(pbpal_send_status, returns(0));
    expect(pbpal_send, when(data, streqs(" HTTP/1.1\r\nHost: ")), returns(0));
    expect(pbpal_send_status, returns(0));
    expect(pbpal_send_str, when(s, streqs(PUBNUB_ORIGIN)), returns(0));
    expect(pbpal_send_status, returns(0));
    expect(pbpal_send_str,
           when(s,
                streqs("\r\nUser-Agent: POSIX-PubNub-C-core/" PUBNUB_SDK_VERSION
                       "\r\n" ACCEPT_ENCODING "\r\n")),
           returns(0));
    expect(pbpal_send_status, returns(0));
    expect(pbntf_watch_in_events, when(pb, equals(pbp)), returns(0));
}


