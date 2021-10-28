/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"

#include "pubnub_version_internal.h"
#include "pubnub_pubsubapi.h"
#include "pubnub_coreapi.h"
#include "pubnub_assert.h"
#include "pubnub_alloc.h"
#include "pubnub_log.h"

#include "pbpal.h"
#include "pubnub_test_helper.h"
#include "pubnub_proxy.h"

#include "pubnub_json_parse.h"
#include "pubnub_keep_alive.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <setjmp.h>

#define MAX_LEN 5000
#define MIN_LEN_WITH_ENCODED_CREDENTIALS 500

typedef struct mock_node {
    char              function_name[50];
    pubnub_t*         pb;
    char              data[MAX_LEN];
    int               return_value;
    struct mock_node* previous;
    struct mock_node* next;
} mock_node_t;

static pubnub_t* pbp;

/* A less chatty cgreen :) */

#define attest(rslt)                                                           \
    do {                                                                       \
        assert(rslt);                                                          \
        m_passes++;                                                            \
    } while (0)
/*
#define equals is_equal_to
#define streqs is_equal_to_string
#define differs is_not_equal_to
#define strdifs is_not_equal_to_string
#define ptreqs(val) is_equal_to_contents_of(&(val), sizeof(val))
#define ptrdifs(val) is_not_equal_to_contents_of(&(val), sizeof(val))
#define sets(par, val) will_set_contents_of_parameter(par, &(val), sizeof(val))
#define sets_ex will_set_contents_of_parameter
#define returns will_return
*/
/* Current (simulated) message received */
static char const* m_read;
/* Number of simulated messages received */
static int m_num_msgrcvd;
/* Array of simulated 'receive' messages */
static char* m_msg_array[20] = {
    NULL,
};
/* Index(in the array) of the current message used while receiving*/
static int m_i;
static int m_passes   = 0;
static int m_failures = 0;
static int m_tests    = 0;

static mock_node_t* m_list_head_mocked = NULL;
static mock_node_t* m_list_tail_mocked = NULL;

static void wait_milliseconds(unsigned time_in_milliseconds)
{
    clock_t  start = clock();
    unsigned time_passed_in_milliseconds;
    do {
        time_passed_in_milliseconds = (clock() - start) / (CLOCKS_PER_SEC/1000);
    } while (time_passed_in_milliseconds < time_in_milliseconds);
}

void expect(const char* function_name, pubnub_t* pb, const char* data, int return_value)
{
    mock_node_t* new_node_mocked = (mock_node_t*)malloc(sizeof(mock_node_t));

    assert(new_node_mocked != NULL);

    new_node_mocked->previous = NULL;
    new_node_mocked->next     = NULL;
    strcpy(new_node_mocked->function_name, function_name);
    new_node_mocked->pb = pb;
    strcpy(new_node_mocked->data, data);
    new_node_mocked->return_value = return_value;
    if (NULL == m_list_head_mocked) {
        m_list_head_mocked = new_node_mocked;
        m_list_tail_mocked = m_list_head_mocked;
        return;
    }
    m_list_tail_mocked->next  = new_node_mocked;
    new_node_mocked->previous = m_list_tail_mocked;
    m_list_tail_mocked        = new_node_mocked;
    return;
}

int mock(const char* function_name, pubnub_t* pb, const char* data)
{
    mock_node_t* node_mocked = m_list_head_mocked;
    int          return_value;
    bool         rslt;

    while ((node_mocked != NULL)
           && (strcmp(node_mocked->function_name, function_name) != 0)) {
        node_mocked = node_mocked->next;
    }
    if (node_mocked == NULL) {
        printf("----->Mocked function [%s] did not have an expectation that it "
               "would be called!\n",
               function_name);
        m_failures++;
        return 0;
    }
    return_value = node_mocked->return_value;
    if (node_mocked->next == NULL) {
        m_list_tail_mocked = node_mocked->previous;
        if (m_list_tail_mocked != NULL) {
            m_list_tail_mocked->next = NULL;
        }
    }
    else {
        node_mocked->next->previous = node_mocked->previous;
    }
    if (node_mocked->previous == NULL) {
        m_list_head_mocked = node_mocked->next;
        if (m_list_head_mocked != NULL) {
            m_list_head_mocked->previous = NULL;
        }
    }
    else {
        node_mocked->previous->next = node_mocked->next;
    }

    PUBNUB_LOG_TRACE("[%s][%s]\nexpected:[%s]\n", function_name, data, node_mocked->data);
    assert(node_mocked->pb == pbp);
    if (MIN_LEN_WITH_ENCODED_CREDENTIALS > strlen(node_mocked->data)) {
        attest(strlen(node_mocked->data) == strlen(data));
    }
    else {
        attest(MIN_LEN_WITH_ENCODED_CREDENTIALS <= strlen(data));
    }

    free(node_mocked);
    return return_value;
}


void check_residual_mocks(void)
{
    mock_node_t* node_mocked = m_list_head_mocked;
    char         function_name[50];

    while (m_list_head_mocked != NULL) {
        strcpy(function_name, m_list_head_mocked->function_name);
        printf("-----> Expected function [%s] was not called!\n", function_name);
        m_failures++;
        node_mocked = m_list_head_mocked;
        if (node_mocked->next != NULL) {
            node_mocked->next->previous = NULL;
        }
        m_list_head_mocked = node_mocked->next;
        free(node_mocked);
    }
    printf("----------------(passes: %d)---------------->\n", m_passes);
    printf("(#Tests: %d)-----(failures: %d)!\n", ++m_tests, m_failures);
}

/* The Pubnub PAL mocks and stubs */

static void buf_setup(pubnub_t* pb)
{
    pb->ptr  = (uint8_t*)pb->core.http_buf;
    pb->left = sizeof pb->core.http_buf;
    PUBNUB_LOG_TRACE("\n PUBNUB_BUF_MAXLEN= %d\n", PUBNUB_BUF_MAXLEN);
    PUBNUB_LOG_TRACE(" sizeof pb->core.http_buf= %d\n\n", pb->left);
}

void pbpal_init(pubnub_t* pb)
{
    buf_setup(pb);
}

enum pbpal_resolv_n_connect_result pbpal_resolv_and_connect(pubnub_t* pb)
{
    return (enum pbpal_resolv_n_connect_result)mock(
        "pbpal_resolv_and_connect", pb, "");
}

enum pbpal_resolv_n_connect_result pbpal_check_resolv_and_connect(pubnub_t* pb)
{
    return (enum pbpal_resolv_n_connect_result)mock(
        "pbpal_check_resolv_and_connect", pb, "");
}

#if defined(PUBNUB_CALLBACK_API)
#if PUBNUB_CHANGE_DNS_SERVERS
int pbpal_dns_rotate_server(pubnub_t *pb)
{
    return mock("pbpal_dns_rotate_server", pb, "");
}
#endif /* PUBNUB_CHANGE_DNS_SERVERS */
#endif /* defined(PUBNUB_CALLBACK_API) */

bool pbpal_connected(pubnub_t* pb)
{
    return (bool)mock("pbpal_connected", pb, "");
}

void pbpal_free(pubnub_t* pb)
{
    mock("pbpal_free", pb, "");
}

void pbpal_report_error_from_environment(pubnub_t* pb, char const* file, int line)
{
    mock("pbpal_report_error_from_environment", pb, "");
}

enum pbpal_resolv_n_connect_result pbpal_check_connect(pubnub_t* pb)
{
    return (enum pbpal_resolv_n_connect_result)mock("pbpal_check_connect", pb, "");
}

int pbpal_send(pubnub_t* pb, void const* data, size_t n)
{
    return mock("pbpal_send", pb, (char const*)data);
}

int pbpal_send_str(pubnub_t* pb, char const* s)
{
    return mock("pbpal_send_str", pb, s);
}


int pbpal_send_status(pubnub_t* pb)
{
    return mock("pbpal_send_status", pb, "");
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
    int to_read;

    assert(m_read != NULL);
    assert(m_num_msgrcvd < (sizeof m_msg_array + 1));

    if (m_read[0] == '\0') {
        /* In case of receiving from current string
         * look for next(string) in the array of simulated
         * messages. So, effectivly, it's a game(play) with lengths of
         * messages and http_ buffer(within context) available
         */
        if (m_i < m_num_msgrcvd - 1) {
            m_read = m_msg_array[++m_i];
            assert(m_read != NULL);

            if (strlen(m_read) == 0) {
                /* an empty string sent(server response simulation),
                 * through function 'incoming',
                 * 'APIs' should interpret as:
                 * ('recv_from_platform_result' < 0) which gives
                 * the oportunity to simulate and test
                 * 'callback' environment in some degree.
                 * NOTE: first string in the array must not be empty
                 * string unless it's ment to be the last
                 */
                return -1;
            }
        }
        else {
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
    PUBNUB_LOG_TRACE("\n to_read = %d\n", to_read);

    return to_read;
}

enum pubnub_res pbpal_line_read_status(pubnub_t* pb)
{
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
        uint8_t c = *pb->ptr++;
        --pb->unreadlen;

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
    return (bool)mock("pbpal_closed", pb, "");
}

void pbpal_forget(pubnub_t* pb)
{
    mock("pbpal_forget", pb, "");
}

int pbpal_close(pubnub_t* pb)
{
    pb->sock_state = STATE_NONE;
    return mock("pbpal_close", pb, "");
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
    return "Windows-PubNub-C-core/" PUBNUB_SDK_VERSION;
}


/* The Pubnub NTF mocks and stubs */

int pbntf_got_socket(pubnub_t* pb)
{
    return mock("pbntf_got_socket", pb, "");
}

void pbntf_lost_socket(pubnub_t* pb)
{
    mock("pbntf_lost_socket", pb, "");
}

void pbntf_update_socket(pubnub_t* pb)
{
    mock("pbntf_update_socket", pb, "");
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
    return mock("pbntf_requeue_for_processing", pb, "");
}

int pbntf_enqueue_for_processing(pubnub_t* pb)
{
    return mock("pbntf_enqueue_for_processing", pb, "");
}

int pbntf_watch_out_events(pubnub_t* pb)
{
    return mock("pbntf_watch_out_events", pb, "");
}

int pbntf_watch_in_events(pubnub_t* pb)
{
    return mock("pbntf_watch_in_events", pb, "");
}

void pbntf_trans_outcome(pubnub_t* pb, enum pubnub_state state)
{
    pb->state = state;
    mock("pbntf_trans_outcome", pb, "");
}

/* Assert "catching" */
static bool        m_expect_assert;
static jmp_buf     m_assert_exp_jmpbuf;
static char const* m_expect_assert_file;


void assert_handler(char const* s, const char* file, long i)
{
    PUBNUB_LOG_TRACE("%s:%ld: Pubnub assert failed '%s'\n", file, i, s);

    attest(m_expect_assert);
    attest(strcmp(m_expect_assert_file, file) == 0);
    if (m_expect_assert) {
        m_expect_assert = false;
        longjmp(m_assert_exp_jmpbuf, 1);
    }
}

#define expect_assert_in(expr, file)                                           \
    {                                                                          \
        m_expect_assert      = true;                                           \
        m_expect_assert_file = file;                                           \
        int val              = setjmp(m_assert_exp_jmpbuf);                    \
        if (0 == val)                                                          \
            expr;                                                              \
        attest(!m_expect_assert);                                              \
    }


void free_m_msgs(char** msg_array)
{
    int i;

    assert(m_num_msgrcvd < (sizeof m_msg_array + 1));

    for (i = 0; i < m_num_msgrcvd; i++) {
        assert(m_msg_array[i] != NULL);
        free(m_msg_array[i]);
        PUBNUB_LOG_TRACE("\n free(m_msg_array[%d])\n", i);
    }
}

static void expect_have_dns_for_proxy_server(void)
{
    expect("pbntf_enqueue_for_processing", pbp, "", 0);
    expect("pbpal_resolv_and_connect", pbp, "", pbpal_connect_success);
    expect("pbntf_got_socket", pbp, "", 0);
}

static void expect_have_dns_for_proxy_server_without_enqueue_for_processing(void)
{
    expect("pbpal_resolv_and_connect", pbp, "", pbpal_connect_success);
    expect("pbntf_got_socket", pbp, "", 0);
}

static void expect_first_outgoing_GET(const char* url)
{
    expect("pbpal_send_str", pbp, "GET ", 0);
    expect("pbpal_send_status", pbp, "", 0);
    expect("pbpal_send_str", pbp, "http://", 0);
    expect("pbpal_send_status", pbp, "", 0);
    expect("pbpal_send_str", pbp, PUBNUB_ORIGIN, 0);
    expect("pbpal_send_status", pbp, "", 0);
    expect("pbpal_send_str", pbp, url, 0);
    expect("pbpal_send_status", pbp, "", 0);
    expect("pbpal_send", pbp, " HTTP/1.1\r\nHost: ", 0);
    expect("pbpal_send_status", pbp, "", 0);
    expect("pbpal_send_str", pbp, PUBNUB_ORIGIN, 0);
    expect("pbpal_send_status", pbp, "", 0);
    expect("pbpal_send_str",
           pbp,
           "\r\nUser-Agent: Windows-PubNub-C-core/" PUBNUB_SDK_VERSION
           "\r\n" ACCEPT_ENCODING "\r\n",
           0);
    expect("pbpal_send_status", pbp, "", 0);
    expect("pbntf_watch_in_events", pbp, "", 0);
}

static void expect_first_outgoing_CONNECT(void)
{
    expect("pbpal_send", pbp, "CONNECT ", 0);
    expect("pbpal_send_status", pbp, "", 0);
    expect("pbpal_send_status", pbp, "", 0);
    expect("pbpal_send_str", pbp, PUBNUB_ORIGIN, 0);
    expect("pbpal_send_status", pbp, "", 0);
    expect("pbpal_send_str", pbp, ":80", 0);
    expect("pbpal_send_status", pbp, "", 0);
    expect("pbpal_send", pbp, " HTTP/1.1\r\nHost: ", 0);
    expect("pbpal_send_status", pbp, "", 0);
    expect("pbpal_send_str", pbp, PUBNUB_ORIGIN, 0);
    expect("pbpal_send_status", pbp, "", 0);
    expect("pbpal_send_str",
           pbp,
           "\r\nUser-Agent: Windows-PubNub-C-core/" PUBNUB_SDK_VERSION
           "\r\n" ACCEPT_ENCODING "\r\n",
           0);
    expect("pbpal_send_status", pbp, "", 0);
    expect("pbntf_watch_in_events", pbp, "", 0);
}


static void expect_outgoing_with_encoded_credentials_GET(char const* url,
                                                         char const* HTTP_proxy_header)
{
    expect("pbpal_send_str", pbp, "GET ", 0);
    expect("pbpal_send_status", pbp, "", 0);
    expect("pbpal_send_str", pbp, "http://", 0);
    expect("pbpal_send_status", pbp, "", 0);
    expect("pbpal_send_str", pbp, PUBNUB_ORIGIN, 0);
    expect("pbpal_send_status", pbp, "", 0);
    expect("pbpal_send_str", pbp, url, 0);
    expect("pbpal_send_status", pbp, "", 0);
    expect("pbpal_send", pbp, " HTTP/1.1\r\nHost: ", 0);
    expect("pbpal_send_status", pbp, "", 0);
    expect("pbpal_send_str", pbp, PUBNUB_ORIGIN, 0);
    expect("pbpal_send_status", pbp, "", 0);
    expect("pbpal_send_str", pbp, HTTP_proxy_header, 0);
    expect("pbpal_send_status", pbp, "", 0);
    expect("pbpal_send_str",
           pbp,
           "\r\nUser-Agent: Windows-PubNub-C-core/" PUBNUB_SDK_VERSION
           "\r\n" ACCEPT_ENCODING "\r\n",
           0);
    expect("pbpal_send_status", pbp, "", 0);
    expect("pbntf_watch_in_events", pbp, "", 0);
}

static void expect_outgoing_with_encoded_credentials_CONNECT(char const* url,
                                                             char const* HTTP_proxy_header)
{
    expect("pbpal_send", pbp, "CONNECT ", 0);
    expect("pbpal_send_status", pbp, "", 0);
    expect("pbpal_send_status", pbp, "", 0);
    expect("pbpal_send_str", pbp, PUBNUB_ORIGIN, 0);
    expect("pbpal_send_status", pbp, "", 0);
    expect("pbpal_send_str", pbp, ":80", 0);
    expect("pbpal_send_status", pbp, "", 0);
    expect("pbpal_send", pbp, " HTTP/1.1\r\nHost: ", 0);
    expect("pbpal_send_status", pbp, "", 0);
    expect("pbpal_send_str", pbp, PUBNUB_ORIGIN, 0);
    expect("pbpal_send_status", pbp, "", 0);
    expect("pbpal_send_str", pbp, HTTP_proxy_header, 0);
    expect("pbpal_send_status", pbp, "", 0);
    expect("pbpal_send_str",
           pbp,
           "\r\nUser-Agent: Windows-PubNub-C-core/" PUBNUB_SDK_VERSION
           "\r\n" ACCEPT_ENCODING "\r\n",
           0);
    expect("pbpal_send_status", pbp, "", 0);
    expect("pbntf_watch_in_events", pbp, "", 0);
}


static void expect_outgoing_with_url(char const* url)
{
    expect("pbpal_send_str", pbp, "GET ", 0);
    expect("pbpal_send_status", pbp, "", 0);
    expect("pbpal_send_status", pbp, "", 0);
    expect("pbpal_send_status", pbp, "", 0);
    expect("pbpal_send_str", pbp, url, 0);
    expect("pbpal_send_status", pbp, "", 0);
    expect("pbpal_send", pbp, " HTTP/1.1\r\nHost: ", 0);
    expect("pbpal_send_status", pbp, "", 0);
    expect("pbpal_send_str", pbp, PUBNUB_ORIGIN, 0);
    expect("pbpal_send_status", pbp, "", 0);
    expect("pbpal_send_str",
           pbp,
           "\r\nUser-Agent: Windows-PubNub-C-core/" PUBNUB_SDK_VERSION
           "\r\n" ACCEPT_ENCODING "\r\n",
           0);
    expect("pbpal_send_status", pbp, "", 0);
    expect("pbntf_watch_in_events", pbp, "", 0);
}

static void incoming(char const* str)
{
    char* pmsg = malloc(sizeof(char) * (strlen(str) + 1));

    assert(pmsg != NULL);
    assert(m_num_msgrcvd < sizeof m_msg_array);

    strcpy(pmsg, str);
    m_msg_array[m_num_msgrcvd++] = pmsg;
    if (m_num_msgrcvd == 1) {
        m_read = pmsg;
    }
}


static void incoming_and_close(char const* str)
{
    incoming(str);
    expect("pbpal_close", pbp, "", 0);
    expect("pbpal_forget", pbp, "", 0);
    expect("pbntf_trans_outcome", pbp, "", 0);
}


static void cancel_and_cleanup(pubnub_t* pbp)
{
    expect("pbpal_close", pbp, "", 0);
    expect("pbpal_closed", pbp, "", (int)true);
    expect("pbpal_forget", pbp, "", 0);
    expect("pbntf_trans_outcome", pbp, "", 0);
    pubnub_cancel(pbp);
    attest(pbp->core.last_result == PNR_CANCELLED);
}

static void BeforeEach(void)
{
    pubnub_assert_set_handler((pubnub_assert_handler_t)assert_handler);
    m_read        = NULL;
    m_num_msgrcvd = 0;
    m_i           = 0;
    pbp           = pubnub_alloc();
}

static void AfterEach(void)
{
    if (pbp->state != PBS_IDLE) {
        expect("pbpal_close", pbp, "", 0);
        expect("pbpal_closed", pbp, "", (int)true);
        expect("pbpal_forget", pbp, "", 0);
    }
    expect("pbntf_trans_outcome", pbp, "", 0);
    expect("pbpal_free", pbp, "", 0);
    attest(pubnub_free(pbp) == 0);
    check_residual_mocks();
    free_m_msgs(m_msg_array);
}

/* Tests */

/* GET NTLM */
static void proxy_establishes_GET_NTLM_connection(void)
{
    int proxy_port = 500;
    BeforeEach();
    pubnub_init(pbp, "publ-key", "sub-key");
    attest(pubnub_set_proxy_manual(pbp, pbproxyHTTP_GET, "proxy_server_url", proxy_port)
           == 0);
    attest(pubnub_set_proxy_authentication_username_password(pbp, "serious", "password")
           == 0);
    expect_have_dns_for_proxy_server();

    expect_first_outgoing_GET(
        "/subscribe/sub-key/health/0/0?pnsdk=unit-test-0.1");
    incoming("HTTP/1.1 407 ProxyAuthentication Required ( The ISA Server "
             "requires authorization to fulfill the request. Access to the Web "
             "proxy service is denied. )\r\n"
             "Via: 1.1 SPIRIT1B\r\n"
             "Proxy-Authenticate: Negotiate\r\n"
             "Proxy-Authenticate: Kerberos\r\n"
             "Proxy-Authenticate: NTLM\r\n"
             "Connection: close\r\n"
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
    expect("pbpal_close", pbp, "", 0);
    expect_have_dns_for_proxy_server_without_enqueue_for_processing();
    expect_outgoing_with_encoded_credentials_GET(
        "/subscribe/sub-key/health/0/0?pnsdk=unit-test-0.1",
        "\r\nProxy-Authorization: NTLM TlRMTVNTUAABAAAAB4IIogAAAAAAAAAAAAAAAAAAAAAKAKs/AAAADw==");
    incoming(
        "HTTP/1.1 407 ProxyAuthentication Required ( Access is denied. )\r\n"
        "Via: 1.1 SPIRIT1B\r\n"
        "Proxy-Authenticate: NTLM "
        "TlRMTVNTUAACAAAAEAAQADgAAAA1goriluCDYHcYI/"
        "sAAAAAAAAAAFQAVABIAAAABQLODgAAAA9TAFAASQBSAEkAVAAxAEIAAgAQAFMAUABJAFIA"
        "SQBUADEAQgABABAAUwBQAEkAUgBJAFQAMQBCAAQAEABzAHAAaQByAGkAdAAxAGIAAwAQAH"
        "MAcABpAHIAaQB0ADEAYgAAAAAA\r\n"
        "Connection: Keep-Alive\r\n"
        "Proxy-Connection: Keep-Alive\r\n"
        "Pragma: no-cache\r\n"
        "Cache-Control: no-cache\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: 11\r\n"
        "\r\n"
        "<comment>\r\n");
    expect_outgoing_with_encoded_credentials_GET(
        "/subscribe/sub-key/health/0/0?pnsdk=unit-test-0.1",
        "\r\nProxy-Authorization: NTLM "
        "TlRMTVNTUAADAAAAGAAYAIQAAADQANAAnAAAAAAAAABYAAAADgAOAFgAAAAeAB4AZgAAAA"
        "AAAABsAQAABYKIogoAqz8AAAAPccZQvLc9g1+"
        "Nren4B1Ib4HMAZQByAGkAbwB1AHMARABFAFMASwBUAE8AUAAtADcAMQA0ADQARgBSAEsAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAPYV7z8b1u4i3PisNiYQE1QEBAAAAAAAAukU1J0O"
        "40wGP8Kgb8CDzxgAAAAACABAAUwBQAEkAUgBJAFQAMQBCAAEAEABTAFAASQBSAEkAVAAxA"
        "EIABAAQAHMAcABpAHIAaQB0ADEAYgADABAAcwBwAGkAcgBpAHQAMQBiAAgAMAAwAAAAAAA"
        "AAAEAAAAAIAAAptCBZNmwVx+"
        "C4b7LJ01Abe4XUAITMf9HDfeJZOCOJR8KABAAAAAAAAAAAAAAAAAAAAAAAAkAAAAAAAAAA"
        "AAAAA==");
    incoming("HTTP/1.1 200 Connection established\r\n"
             "Via: 1.1 SPIRIT1B\r\n"
             "Content-Length: 26\r\n"
             "\r\n"
             "[[],\"1516014978925123457\"]");
    attest(pubnub_subscribe(pbp, "health", NULL) == PNR_STARTED);

    attest(pbnc_fsm(pbp) == 0);
    attest(pbnc_fsm(pbp) == 0);
    attest(pbnc_fsm(pbp) == 0);

    expect("pbntf_lost_socket", pbp, "", 0);
    expect("pbntf_trans_outcome", pbp, "", 0);
    attest(pbnc_fsm(pbp) == 0);

    attest(pubnub_get(pbp) == NULL);
    attest(pubnub_last_http_code(pbp) == 200);

    expect("pbntf_enqueue_for_processing", pbp, "", 0);
    expect("pbntf_got_socket", pbp, "", 0);
    expect_first_outgoing_GET(
        "/subscribe/sub-key/health/0/1516014978925123457?pnsdk=unit-test-0.1");
    incoming("HTTP/1.1 407 ProxyAuthentication Required ( The ISA Server "
             "requires authorization to fulfill the request. Access to the Web "
             "proxy service is denied. )\r\n"
             "Via: 1.1 SPIRIT1B\r\n"
             "Proxy-Authenticate: Negotiate\r\n"
             "Proxy-Authenticate: Kerberos\r\n"
             "Proxy-Authenticate: NTLM\r\n"
             "Connection: close\r\n"
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
    expect("pbpal_close", pbp, "", 0);
    expect_have_dns_for_proxy_server_without_enqueue_for_processing();
    expect_outgoing_with_encoded_credentials_GET(
        "/subscribe/sub-key/health/0/1516014978925123457?pnsdk=unit-test-0.1", "\r\nProxy-Authorization: NTLM TlRMTVNTUAABAAAAB4IIogAAAAAAAAAAAAAAAAAAAAAKAKs/AAAADw==");
    incoming(
        "HTTP/1.1 407 ProxyAuthentication Required ( Access is denied. )\r\n"
        "Via: 1.1 SPIRIT1B\r\n"
        "Proxy-Authenticate: NTLM "
        "TlRMTVNTUAACAAAAEAAQADgAAAA1goriluCDYHcYI/"
        "sAAAAAAAAAAFQAVABIAAAABQLODgAAAA9TAFAASQBSAEkAVAAxAEIAAgAQAFMAUABJAFIA"
        "SQBUADEAQgABABAAUwBQAEkAUgBJAFQAMQBCAAQAEABzAHAAaQByAGkAdAAxAGIAAwAQAH"
        "MAcABpAHIAaQB0ADEAYgAAAAAA\r\n"
        "Connection: Keep-Alive\r\n"
        "Proxy-Connection: Keep-Alive\r\n"
        "Pragma: no-cache\r\n"
        "Cache-Control: no-cache\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: 11\r\n"
        "\r\n"
        "<comment>\r\n");
    expect_outgoing_with_encoded_credentials_GET(
        "/subscribe/sub-key/health/0/1516014978925123457?pnsdk=unit-test-0.1",
        "\r\nProxy-Authorization: NTLM "
        "TlRMTVNTUAADAAAAGAAYAIQAAADQANAAnAAAAAAAAABYAAAADgAOAFgAAAAeAB4AZgAAAA"
        "AAAABsAQAABYKIogoAqz8AAAAPccZQvLc9g1+"
        "Nren4B1Ib4HMAZQByAGkAbwB1AHMARABFAFMASwBUAE8AUAAtADcAMQA0ADQARgBSAEsAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAPYV7z8b1u4i3PisNiYQE1QEBAAAAAAAAukU1J0O"
        "40wGP8Kgb8CDzxgAAAAACABAAUwBQAEkAUgBJAFQAMQBCAAEAEABTAFAASQBSAEkAVAAxA"
        "EIABAAQAHMAcABpAHIAaQB0ADEAYgADABAAcwBwAGkAcgBpAHQAMQBiAAgAMAAwAAAAAAA"
        "AAAEAAAAAIAAAptCBZNmwVx+"
        "C4b7LJ01Abe4XUAITMf9HDfeJZOCOJR8KABAAAAAAAAAAAAAAAAAAAAAAAAkAAAAAAAAAA"
        "AAAAA==");
    incoming("HTTP/1.1 200\r\n"
             "Content-Length: 56\r\n"
             "\r\n"
             "[[pomegranate_juice,papaya,mango],\"1516714978925123457\"]");
    attest(pubnub_subscribe(pbp, "health", NULL) == PNR_STARTED);

    attest(pbnc_fsm(pbp) == 0);
    attest(pbnc_fsm(pbp) == 0);
    attest(pbnc_fsm(pbp) == 0);

    expect("pbntf_lost_socket", pbp, "", 0);
    expect("pbntf_trans_outcome", pbp, "", 0);
    attest(pbnc_fsm(pbp) == 0);

    attest(pubnub_last_http_code(pbp) == 200);

    attest(strcmp(pubnub_get(pbp), "pomegranate_juice") == 0);
    attest(strcmp(pubnub_get(pbp), "papaya") == 0);
    attest(strcmp(pubnub_get(pbp), "mango") == 0);
    attest(pubnub_get(pbp) == NULL);
    attest(pubnub_get_channel(pbp) == NULL);
    attest(pubnub_last_http_code(pbp) == 200);

    AfterEach();
}

/* CONNECT NTLM */
static void proxy_establishes_CONNECT_NTLM_connection(void)
{
    int proxy_port = 500;
    BeforeEach();
    pubnub_init(pbp, "publ-key", "sub-key");
    attest(pubnub_set_proxy_manual(pbp, pbproxyHTTP_CONNECT, "proxy_server_url", proxy_port)
           == 0);
    attest(pubnub_set_proxy_authentication_username_password(pbp, "serious", "password")
           == 0);
    expect_have_dns_for_proxy_server();

    expect_first_outgoing_CONNECT();
    incoming("HTTP/1.1 407 ProxyAuthentication Required ( The ISA Server "
             "requires authorization to fulfill the request. Access to the Web "
             "proxy service is denied. )\r\n"
             "Via: 1.1 SPIRIT1B\r\n"
             "Proxy-Authenticate: Negotiate\r\n"
             "Proxy-Authenticate: Kerberos\r\n"
             "Proxy-Authenticate: NTLM\r\n"
             "Connection: close\r\n"
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
    expect("pbpal_close", pbp, "", 0);
    expect_have_dns_for_proxy_server_without_enqueue_for_processing();
    expect_outgoing_with_encoded_credentials_CONNECT(
        "/subscribe/sub-key/health/0/0?pnsdk=unit-test-0.1", "\r\nProxy-Authorization: NTLM TlRMTVNTUAABAAAAB4IIogAAAAAAAAAAAAAAAAAAAAAKAKs/AAAADw==");
    incoming(
        "HTTP/1.1 407 ProxyAuthentication Required ( Access is denied. )\r\n"
        "Via: 1.1 SPIRIT1B\r\n"
        "Proxy-Authenticate: NTLM "
        "TlRMTVNTUAACAAAAEAAQADgAAAA1goriluCDYHcYI/"
        "sAAAAAAAAAAFQAVABIAAAABQLODgAAAA9TAFAASQBSAEkAVAAxAEIAAgAQAFMAUABJAFIA"
        "SQBUADEAQgABABAAUwBQAEkAUgBJAFQAMQBCAAQAEABzAHAAaQByAGkAdAAxAGIAAwAQAH"
        "MAcABpAHIAaQB0ADEAYgAAAAAA\r\n"
        "Connection: Keep-Alive\r\n"
        "Proxy-Connection: Keep-Alive\r\n"
        "Pragma: no-cache\r\n"
        "Cache-Control: no-cache\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: 11\r\n"
        "\r\n"
        "<comment>\r\n");
    expect_outgoing_with_encoded_credentials_CONNECT(
        "/subscribe/sub-key/health/0/0?pnsdk=unit-test-0.1",
        "\r\nProxy-Authorization: NTLM "
        "TlRMTVNTUAADAAAAGAAYAIQAAADQANAAnAAAAAAAAABYAAAADgAOAFgAAAAeAB4AZgAAAA"
        "AAAABsAQAABYKIogoAqz8AAAAPccZQvLc9g1+"
        "Nren4B1Ib4HMAZQByAGkAbwB1AHMARABFAFMASwBUAE8AUAAtADcAMQA0ADQARgBSAEsAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAPYV7z8b1u4i3PisNiYQE1QEBAAAAAAAAukU1J0O"
        "40wGP8Kgb8CDzxgAAAAACABAAUwBQAEkAUgBJAFQAMQBCAAEAEABTAFAASQBSAEkAVAAxA"
        "EIABAAQAHMAcABpAHIAaQB0ADEAYgADABAAcwBwAGkAcgBpAHQAMQBiAAgAMAAwAAAAAAA"
        "AAAEAAAAAIAAAptCBZNmwVx+"
        "C4b7LJ01Abe4XUAITMf9HDfeJZOCOJR8KABAAAAAAAAAAAAAAAAAAAAAAAAkAAAAAAAAAA"
        "AAAAA==");
    incoming("HTTP/1.1 200 Connection established\r\n"
             "Via: 1.1 SPIRIT1B\r\n"
             "Expires: 7200\r\n"
             "Content-Length: 0\r\n"
             "\r\n");
    expect_outgoing_with_url(
        "/subscribe/sub-key/health/0/0?pnsdk=unit-test-0.1");
    incoming("HTTP/1.1 200 Connection established\r\n"
             "Via: 1.1 SPIRIT1B\r\n"
             "Content-Length: 26\r\n"
             "\r\n"
             "[[],\"1516014978925123457\"]");
    attest(pubnub_subscribe(pbp, "health", NULL) == PNR_STARTED);

    attest(pbnc_fsm(pbp) == 0);
    attest(pbnc_fsm(pbp) == 0);
    attest(pbnc_fsm(pbp) == 0);

    expect("pbntf_lost_socket", pbp, "", 0);
    expect("pbntf_trans_outcome", pbp, "", 0);
    attest(pbnc_fsm(pbp) == 0);

    attest(pubnub_get(pbp) == NULL);
    attest(pubnub_last_http_code(pbp) == 200);

    expect("pbntf_enqueue_for_processing", pbp, "", 0);
    expect("pbntf_got_socket", pbp, "", 0);
    expect_outgoing_with_url(
        "/subscribe/sub-key/health/0/1516014978925123457?pnsdk=unit-test-0.1");
    incoming("HTTP/1.1 200\r\n"
             "Via: 1.1 SPIRIT1B\r\n"
             "Content-Length: 56\r\n"
             "\r\n"
             "[[pomegranate_juice,papaya,mango],\"1516714978925123457\"]");
    expect("pbntf_lost_socket", pbp, "", 0);
    expect("pbntf_trans_outcome", pbp, "", 0);
    attest(pubnub_subscribe(pbp, "health", NULL) == PNR_OK);

    attest(pubnub_last_http_code(pbp) == 200);

    attest(strcmp(pubnub_get(pbp), "pomegranate_juice") == 0);
    attest(strcmp(pubnub_get(pbp), "papaya") == 0);
    attest(strcmp(pubnub_get(pbp), "mango") == 0);
    attest(pubnub_get(pbp) == NULL);
    attest(pubnub_get_channel(pbp) == NULL);
    attest(pubnub_last_http_code(pbp) == 200);

    AfterEach();
}

static void CONNECT_NTLM_proxy_closes_connection_on_407_dialogue_continues(void)
{
    int proxy_port = 500;
    BeforeEach();
    pubnub_init(pbp, "publ-key", "sub-key");
    attest(pubnub_set_proxy_manual(pbp, pbproxyHTTP_CONNECT, "proxy_server_url", proxy_port)
           == 0);
    attest(pubnub_set_proxy_authentication_username_password(pbp, "serious", "password")
           == 0);
    expect_have_dns_for_proxy_server();

    expect_first_outgoing_CONNECT();
    incoming("HTTP/1.1 407 ProxyAuthentication Required ( The ISA Server "
             "requires authorization to fulfill the request. Access to the Web "
             "proxy service is denied. )\r\n"
             "Via: 1.1 SPIRIT1B\r\n"
             "Proxy-Authenticate: Negotiate\r\n"
             "Proxy-Authenticate: Kerberos\r\n"
             "Proxy-Authenticate: NTLM\r\n"
             "Connection: close\r\n"
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
    expect("pbpal_close", pbp, "", 0);
    expect_have_dns_for_proxy_server_without_enqueue_for_processing();
    expect_outgoing_with_encoded_credentials_CONNECT(
        "/subscribe/sub-key/health/0/0?pnsdk=unit-test-0.1", "\r\nProxy-Authorization: NTLM TlRMTVNTUAABAAAAB4IIogAAAAAAAAAAAAAAAAAAAAAKAKs/AAAADw==");
    incoming(
        "HTTP/1.1 407 ProxyAuthentication Required ( Access is denied. )\r\n"
        "Via: 1.1 SPIRIT1B\r\n"
        "Proxy-Authenticate: NTLM "
        "TlRMTVNTUAACAAAAEAAQADgAAAA1goriluCDYHcYI/"
        "sAAAAAAAAAAFQAVABIAAAABQLODgAAAA9TAFAASQBSAEkAVAAxAEIAAgAQAFMAUABJAFIA"
        "SQBUADEAQgABABAAUwBQAEkAUgBJAFQAMQBCAAQAEABzAHAAaQByAGkAdAAxAGIAAwAQAH"
        "MAcABpAHIAaQB0ADEAYgAAAAAA\r\n"
        "Connection: close\r\n"
        "Proxy-Connection: Keep-Alive\r\n"
        "Pragma: no-cache\r\n"
        "Cache-Control: no-cache\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: 11\r\n"
        "\r\n"
        "<comment>\r\n");
    expect("pbpal_close", pbp, "", 0);
    expect_have_dns_for_proxy_server_without_enqueue_for_processing();
    expect_outgoing_with_encoded_credentials_CONNECT(
        "/subscribe/sub-key/health/0/0?pnsdk=unit-test-0.1",
        "\r\nProxy-Authorization: NTLM "
        "TlRMTVNTUAADAAAAGAAYAIQAAADQANAAnAAAAAAAAABYAAAADgAOAFgAAAAeAB4AZgAAAA"
        "AAAABsAQAABYKIogoAqz8AAAAPccZQvLc9g1+"
        "Nren4B1Ib4HMAZQByAGkAbwB1AHMARABFAFMASwBUAE8AUAAtADcAMQA0ADQARgBSAEsAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAPYV7z8b1u4i3PisNiYQE1QEBAAAAAAAAukU1J0O"
        "40wGP8Kgb8CDzxgAAAAACABAAUwBQAEkAUgBJAFQAMQBCAAEAEABTAFAASQBSAEkAVAAxA"
        "EIABAAQAHMAcABpAHIAaQB0ADEAYgADABAAcwBwAGkAcgBpAHQAMQBiAAgAMAAwAAAAAAA"
        "AAAEAAAAAIAAAptCBZNmwVx+"
        "C4b7LJ01Abe4XUAITMf9HDfeJZOCOJR8KABAAAAAAAAAAAAAAAAAAAAAAAAkAAAAAAAAAA"
        "AAAAA==");
    incoming("HTTP/1.1 200 Connection established\r\n"
             "Via: 1.1 SPIRIT1B\r\n"
             "Expires: 7200\r\n"
             "Content-Length: 0\r\n"
             "\r\n");
    expect_outgoing_with_url(
        "/subscribe/sub-key/health/0/0?pnsdk=unit-test-0.1");
    incoming("HTTP/1.1 200 Connection established\r\n"
             "Via: 1.1 SPIRIT1B\r\n"
             "Content-Length: 26\r\n"
             "\r\n"
             "[[],\"1516014978925123457\"]");
    attest(pubnub_subscribe(pbp, "health", NULL) == PNR_STARTED);

    attest(pbnc_fsm(pbp) == 0);
    attest(pbnc_fsm(pbp) == 0);
    attest(pbnc_fsm(pbp) == 0);

    expect("pbntf_lost_socket", pbp, "", 0);
    expect("pbntf_trans_outcome", pbp, "", 0);
    attest(pbnc_fsm(pbp) == 0);

    attest(pubnub_get(pbp) == NULL);
    attest(pubnub_last_http_code(pbp) == 200);

    expect("pbntf_enqueue_for_processing", pbp, "", 0);
    expect("pbntf_got_socket", pbp, "", 0);
    expect_outgoing_with_url(
        "/subscribe/sub-key/health/0/1516014978925123457?pnsdk=unit-test-0.1");
    incoming("HTTP/1.1 200\r\n"
             "Via: 1.1 SPIRIT1B\r\n"
             "Content-Length: 56\r\n"
             "\r\n"
             "[[pomegranate_juice,papaya,mango],\"1516714978925123457\"]");
    expect("pbntf_lost_socket", pbp, "", 0);
    expect("pbntf_trans_outcome", pbp, "", 0);
    attest(pubnub_subscribe(pbp, "health", NULL) == PNR_OK);

    attest(pubnub_last_http_code(pbp) == 200);

    attest(strcmp(pubnub_get(pbp), "pomegranate_juice") == 0);
    attest(strcmp(pubnub_get(pbp), "papaya") == 0);
    attest(strcmp(pubnub_get(pbp), "mango") == 0);
    attest(pubnub_get(pbp) == NULL);
    attest(pubnub_get_channel(pbp) == NULL);
    attest(pubnub_last_http_code(pbp) == 200);

    AfterEach();
}

static void proxy_CONNECT_NTLM_sets_timeout_and_max_operation_count_for_keep_alive(void)
{
    int    proxy_port = 500;

    BeforeEach();
    pubnub_init(pbp, "publ-key", "sub-key");
    attest(pubnub_set_proxy_manual(pbp, pbproxyHTTP_CONNECT, "proxy_server_url", proxy_port)
           == 0);
    attest(pubnub_set_proxy_authentication_username_password(pbp, "serious", "password")
           == 0);

    /* Has less than a second to finish up to 3 operations in 'keep_alive'
     * connection*/
    pubnub_set_keep_alive_param(pbp, 0, 3);

    expect_have_dns_for_proxy_server();
    expect_first_outgoing_CONNECT();
    incoming("HTTP/1.1 407 ProxyAuthentication Required ( The ISA Server "
             "requires authorization to fulfill the request. Access to the Web "
             "proxy service is denied. )\r\n"
             "Via: 1.1 SPIRIT1B\r\n"
             "Proxy-Authenticate: Negotiate\r\n"
             "Proxy-Authenticate: Kerberos\r\n"
             "Proxy-Authenticate: NTLM\r\n"
             "Connection: close\r\n"
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
    expect("pbpal_close", pbp, "", 0);
    expect_have_dns_for_proxy_server_without_enqueue_for_processing();
    expect_outgoing_with_encoded_credentials_CONNECT(
        "/subscribe/sub-key/health/0/0?pnsdk=unit-test-0.1", "\r\nProxy-Authorization: NTLM TlRMTVNTUAABAAAAB4IIogAAAAAAAAAAAAAAAAAAAAAKAKs/AAAADw==");
    incoming(
        "HTTP/1.1 407 ProxyAuthentication Required ( Access is denied. )\r\n"
        "Via: 1.1 SPIRIT1B\r\n"
        "Proxy-Authenticate: NTLM "
        "TlRMTVNTUAACAAAAEAAQADgAAAA1goriluCDYHcYI/"
        "sAAAAAAAAAAFQAVABIAAAABQLODgAAAA9TAFAASQBSAEkAVAAxAEIAAgAQAFMAUABJAFIA"
        "SQBUADEAQgABABAAUwBQAEkAUgBJAFQAMQBCAAQAEABzAHAAaQByAGkAdAAxAGIAAwAQAH"
        "MAcABpAHIAaQB0ADEAYgAAAAAA\r\n"
        "Connection: Keep-Alive\r\n"
        "Proxy-Connection: Keep-Alive\r\n"
        "Pragma: no-cache\r\n"
        "Cache-Control: no-cache\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: 11\r\n"
        "\r\n"
        "<comment>\r\n");
    expect_outgoing_with_encoded_credentials_CONNECT(
        "/subscribe/sub-key/health/0/0?pnsdk=unit-test-0.1",
        "\r\nProxy-Authorization: NTLM "
        "TlRMTVNTUAADAAAAGAAYAIQAAADQANAAnAAAAAAAAABYAAAADgAOAFgAAAAeAB4AZgAAAA"
        "AAAABsAQAABYKIogoAqz8AAAAPccZQvLc9g1+"
        "Nren4B1Ib4HMAZQByAGkAbwB1AHMARABFAFMASwBUAE8AUAAtADcAMQA0ADQARgBSAEsAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAPYV7z8b1u4i3PisNiYQE1QEBAAAAAAAAukU1J0O"
        "40wGP8Kgb8CDzxgAAAAACABAAUwBQAEkAUgBJAFQAMQBCAAEAEABTAFAASQBSAEkAVAAxA"
        "EIABAAQAHMAcABpAHIAaQB0ADEAYgADABAAcwBwAGkAcgBpAHQAMQBiAAgAMAAwAAAAAAA"
        "AAAEAAAAAIAAAptCBZNmwVx+"
        "C4b7LJ01Abe4XUAITMf9HDfeJZOCOJR8KABAAAAAAAAAAAAAAAAAAAAAAAAkAAAAAAAAAA"
        "AAAAA==");
    incoming("HTTP/1.1 200 Connection established\r\n"
             "Via: 1.1 SPIRIT1B\r\n"
             "Expires: 7200\r\n"
             "Content-Length: 0\r\n"
             "\r\n");
    expect_outgoing_with_url(
        "/subscribe/sub-key/health/0/0?pnsdk=unit-test-0.1");
    incoming("HTTP/1.1 200 Connection established\r\n"
             "Via: 1.1 SPIRIT1B\r\n"
             "Content-Length: 26\r\n"
             "\r\n"
             "[[],\"1516014978925123457\"]");
    attest(pubnub_subscribe(pbp, "health", NULL) == PNR_STARTED);

    attest(pbnc_fsm(pbp) == 0);
    attest(pbnc_fsm(pbp) == 0);
    attest(pbnc_fsm(pbp) == 0);

    expect("pbntf_lost_socket", pbp, "", 0);
    expect("pbntf_trans_outcome", pbp, "", 0);
    attest(pbnc_fsm(pbp) == 0);

    /* awaits # of milliseconds */
    wait_milliseconds(0);

    attest(pubnub_get(pbp) == NULL);
    attest(pubnub_last_http_code(pbp) == 200);

    expect("pbntf_enqueue_for_processing", pbp, "", 0);
    expect("pbntf_got_socket", pbp, "", 0);
    expect_outgoing_with_url(
        "/subscribe/sub-key/health/0/1516014978925123457?pnsdk=unit-test-0.1");
    incoming("HTTP/1.1 200\r\n"
             "Via: 1.1 SPIRIT1B\r\n"
             "Content-Length: 56\r\n"
             "\r\n"
             "[[pomegranate_juice,papaya,mango],\"1516714978925123457\"]");
    expect("pbntf_lost_socket", pbp, "", 0);
    expect("pbntf_trans_outcome", pbp, "", 0);
    attest(pubnub_subscribe(pbp, "health", NULL) == PNR_OK);

    attest(pubnub_last_http_code(pbp) == 200);

    attest(strcmp(pubnub_get(pbp), "pomegranate_juice") == 0);
    attest(strcmp(pubnub_get(pbp), "papaya") == 0);
    attest(strcmp(pubnub_get(pbp), "mango") == 0);
    attest(pubnub_get(pbp) == NULL);
    attest(pubnub_get_channel(pbp) == NULL);
    attest(pubnub_last_http_code(pbp) == 200);

    expect("pbntf_enqueue_for_processing", pbp, "", 0);
    expect("pbntf_got_socket", pbp, "", 0);
    expect_outgoing_with_url(
        "/publish/publ-key/sub-key/0/panama/0/%22ship%22?pnsdk=unit-test-0.1");
    incoming_and_close("HTTP/1.1 200\r\n"
                       "Via: 1.1 SPIRIT1B\r\n"
                       "Transfer-Encoding: chunked\r\n"
                       "\r\n"
                       "12\r\n"
                       "[1,\"Sent\",\"1417894\r\n"
                       "0C\r\n"
                       "0800777403\"]\r\n"
                       "0\r\n");
    attest(pubnub_publish(pbp, "panama", "\"ship\"") == PNR_OK);
    attest(pubnub_last_http_code(pbp) == 200);

    expect_have_dns_for_proxy_server();
    expect_first_outgoing_CONNECT();
    incoming("HTTP/1.1 407 ProxyAuthentication Required ( The ISA Server "
             "requires authorization to fulfill the request. Access to the Web "
             "proxy service is denied. )\r\n"
             "Via: 1.1 SPIRIT1B\r\n"
             "Proxy-Authenticate: Negotiate\r\n"
             "Proxy-Authenticate: Kerberos\r\n"
             "Proxy-Authenticate: NTLM\r\n"
             "Connection: close\r\n"
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
    expect("pbpal_close", pbp, "", 0);
    expect_have_dns_for_proxy_server_without_enqueue_for_processing();
    expect_outgoing_with_encoded_credentials_CONNECT(
        "/subscribe/sub-key/panama/0/1516714978925123457?pnsdk=unit-test-0.1", "\r\nProxy-Authorization: NTLM TlRMTVNTUAABAAAAB4IIogAAAAAAAAAAAAAAAAAAAAAKAKs/AAAADw==");
    incoming(
        "HTTP/1.1 407 ProxyAuthentication Required ( Access is denied. )\r\n"
        "Via: 1.1 SPIRIT1B\r\n"
        "Proxy-Authenticate: NTLM "
        "TlRMTVNTUAACAAAAEAAQADgAAAA1goriluCDYHcYI/"
        "sAAAAAAAAAAFQAVABIAAAABQLODgAAAA9TAFAASQBSAEkAVAAxAEIAAgAQAFMAUABJAFIA"
        "SQBUADEAQgABABAAUwBQAEkAUgBJAFQAMQBCAAQAEABzAHAAaQByAGkAdAAxAGIAAwAQAH"
        "MAcABpAHIAaQB0ADEAYgAAAAAA\r\n"
        "Connection: Keep-Alive\r\n"
        "Proxy-Connection: Keep-Alive\r\n"
        "Pragma: no-cache\r\n"
        "Cache-Control: no-cache\r\n"
        "Content-Type: text/html\r\n"
        "Content-Length: 11\r\n"
        "\r\n"
        "<comment>\r\n");
    expect_outgoing_with_encoded_credentials_CONNECT(
        "/subscribe/sub-key/panama/0/1516714978925123457?pnsdk=unit-test-0.1",
        "\r\nProxy-Authorization: NTLM "
        "TlRMTVNTUAADAAAAGAAYAIQAAADQANAAnAAAAAAAAABYAAAADgAOAFgAAAAeAB4AZgAAAA"
        "AAAABsAQAABYKIogoAqz8AAAAPccZQvLc9g1+"
        "Nren4B1Ib4HMAZQByAGkAbwB1AHMARABFAFMASwBUAE8AUAAtADcAMQA0ADQARgBSAEsAA"
        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAPYV7z8b1u4i3PisNiYQE1QEBAAAAAAAAukU1J0O"
        "40wGP8Kgb8CDzxgAAAAACABAAUwBQAEkAUgBJAFQAMQBCAAEAEABTAFAASQBSAEkAVAAxA"
        "EIABAAQAHMAcABpAHIAaQB0ADEAYgADABAAcwBwAGkAcgBpAHQAMQBiAAgAMAAwAAAAAAA"
        "AAAEAAAAAIAAAptCBZNmwVx+"
        "C4b7LJ01Abe4XUAITMf9HDfeJZOCOJR8KABAAAAAAAAAAAAAAAAAAAAAAAAkAAAAAAAAAA"
        "AAAAA==");
    incoming("HTTP/1.1 200 Connection established\r\n"
             "Via: 1.1 SPIRIT1B\r\n"
             "Expires: 7200\r\n"
             "Content-Length: 0\r\n"
             "\r\n");
    expect_outgoing_with_url(
        "/subscribe/sub-key/panama/0/1516714978925123457?pnsdk=unit-test-0.1");
    incoming("HTTP/1.1 200\r\n"
             "Via: 1.1 SPIRIT1B\r\n"
             "Content-Length: 44\r\n"
             "\r\n"
             "[[\"ship\",boat,tanker],\"1516714978925123458\"]");
    attest(pubnub_subscribe(pbp, "panama", NULL) == PNR_STARTED);

    attest(pbnc_fsm(pbp) == 0);
    attest(pbnc_fsm(pbp) == 0);
    attest(pbnc_fsm(pbp) == 0);

    expect("pbntf_lost_socket", pbp, "", 0);
    expect("pbntf_trans_outcome", pbp, "", 0);
    attest(pbnc_fsm(pbp) == 0);

    attest(strcmp(pubnub_get(pbp), "\"ship\"") == 0);
    attest(strcmp(pubnub_get(pbp), "boat") == 0);
    attest(strcmp(pubnub_get(pbp), "tanker") == 0);
    attest(pubnub_get(pbp) == NULL);
    attest(pubnub_get_channel(pbp) == NULL);
    attest(pubnub_last_http_code(pbp) == 200);

    AfterEach();
}

/* Test runner */
int main(int argc, char* argv[])
{
    proxy_establishes_GET_NTLM_connection();
    proxy_establishes_CONNECT_NTLM_connection();
    CONNECT_NTLM_proxy_closes_connection_on_407_dialogue_continues();
    proxy_CONNECT_NTLM_sets_timeout_and_max_operation_count_for_keep_alive();
}
