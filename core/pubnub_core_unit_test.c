/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "cgreen/cgreen.h"
#include "cgreen/mocks.h"
#include "test/pubnub_test_mocks.h"

#include "lib/pb_deprecated.h"
#include "pubnub_internal.h"
#include "pubnub_server_limits.h"
#include "pubnub_pubsubapi.h"
#include "pubnub_coreapi.h"
// TODO: move coreapi_ex to new module
#include "pubnub_coreapi_ex.h"
#if PUBNUB_USE_ADVANCED_HISTORY
#include "pubnub_memory_block.h"
#include "pubnub_advanced_history.h"
#endif
#include "pubnub_assert.h"
#include "pubnub_alloc.h"
#include "pubnub_log.h"

#include "pbpal.h"
#include "pubnub_version_internal.h"
#include "pubnub_keep_alive.h"
#include "test/pubnub_test_helper.h"

#include "pubnub_json_parse.h"

#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include <time.h>
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


/* The tests themselves */
Ensure(/*pbjson_parse, */ get_object_value_valid)
{
    char const* json = "{\"some\\key\\\"\": \"some\\value\",\"service\": "
                       "\"xxx\", \"error\": true, "
                       "\"payload\":{\"group\":\"gr\", \"some\\\"\\key\": "
                       "value,\"chan\":[1,2,3]}, \"message\":0}";
    struct pbjson_elem elem = { json, json + strlen(json) };
    struct pbjson_elem parsed;

    attest(pbjson_get_object_value(&elem, "some\\key\\\"", &parsed),
           equals(jonmpOK));
    attest(pbjson_elem_equals_string(&parsed, "some\\value"), is_false);
    attest(pbjson_elem_equals_string(&parsed, "\"some\\value\""), is_true);

    attest(pbjson_get_object_value(&elem, "error", &parsed), equals(jonmpOK));
    attest(pbjson_elem_equals_string(&parsed, "true"), is_true);

    attest(pbjson_get_object_value(&elem, "service", &parsed), equals(jonmpOK));
    attest(pbjson_elem_equals_string(&parsed, "\"xxx\""), is_true);

    attest(pbjson_get_object_value(&elem, "message", &parsed), equals(jonmpOK));
    attest(pbjson_elem_equals_string(&parsed, "0"), is_true);

    attest(pbjson_get_object_value(&elem, "payload", &parsed), equals(jonmpOK));
    attest(pbjson_elem_equals_string(
               &parsed,
               "{\"group\":\"gr\", \"some\\\"\\key\": value,\"chan\":[1,2,3]}"),
           is_true);
    attest(pbjson_object_name_parse_result_2_string(jonmpOK), differs(NULL));
    attest(strlen(pbjson_object_name_parse_result_2_string(jonmpOK)),
           is_greater_than(1));
}


Ensure(/*pbjson_parse, */ get_object_value_invalid)
{
    char const* json = "{\"service\": \"xxx\", \"error\": true, "
                       "\"payload\":{\"group\":\"gr\",\"chan\":[1,2,3]}, "
                       "\"message\":0}";
    struct pbjson_elem elem = { json, json + strlen(json) };
    struct pbjson_elem parsed;

    attest(pbjson_get_object_value(&elem, "", &parsed), equals(jonmpInvalidKeyName));
    attest(pbjson_object_name_parse_result_2_string(jonmpInvalidKeyName),
           differs(NULL));
    attest(strlen(pbjson_object_name_parse_result_2_string(jonmpInvalidKeyName)),
           is_greater_than(1));

    elem.end = elem.start;
    attest(pbjson_get_object_value(&elem, "payload", &parsed),
           equals(jonmpObjectIncomplete));
    attest(pbjson_object_name_parse_result_2_string(jonmpObjectIncomplete),
           differs(NULL));
    attest(strlen(pbjson_object_name_parse_result_2_string(jonmpObjectIncomplete)),
           is_greater_than(1));

    elem.end = elem.start + 1;
    attest(pbjson_get_object_value(&elem, "payload", &parsed),
           equals(jonmpKeyMissing));
    attest(pbjson_object_name_parse_result_2_string(jonmpKeyMissing), differs(NULL));
    attest(strlen(pbjson_object_name_parse_result_2_string(jonmpKeyMissing)),
           is_greater_than(1));

    elem.end = elem.start + 2;
    attest(pbjson_get_object_value(&elem, "payload", &parsed),
           equals(jonmpStringNotTerminated));
    attest(pbjson_object_name_parse_result_2_string(jonmpStringNotTerminated),
           differs(NULL));
    attest(strlen(pbjson_object_name_parse_result_2_string(jonmpStringNotTerminated)),
           is_greater_than(1));

    elem.end = elem.start + 10;
    attest(pbjson_get_object_value(&elem, "payload", &parsed),
           equals(jonmpMissingColon));
    attest(pbjson_object_name_parse_result_2_string(jonmpMissingColon),
           differs(NULL));
    attest(strlen(pbjson_object_name_parse_result_2_string(jonmpMissingColon)),
           is_greater_than(1));

    elem.end = elem.start + 11;
    attest(pbjson_get_object_value(&elem, "payload", &parsed),
           equals(jonmpMissingValueSeparator));
    attest(pbjson_object_name_parse_result_2_string(jonmpMissingValueSeparator),
           differs(NULL));
    attest(strlen(pbjson_object_name_parse_result_2_string(jonmpMissingValueSeparator)),
           is_greater_than(1));

    elem.end = elem.start + 12;
    attest(pbjson_get_object_value(&elem, "payload", &parsed),
           equals(jonmpMissingValueSeparator));

    elem.end = elem.start + 13;
    attest(pbjson_get_object_value(&elem, "payload", &parsed),
           equals(jonmpValueIncomplete));

    elem.end = elem.start + 17;
    attest(pbjson_get_object_value(&elem, "payload", &parsed),
           equals(jonmpObjectIncomplete));

    elem.end = elem.start + 18;
    attest(pbjson_get_object_value(&elem, "payload", &parsed),
           equals(jonmpKeyMissing));

    elem.end = elem.start + 19;
    attest(pbjson_get_object_value(&elem, "payload", &parsed),
           equals(jonmpKeyMissing));

    elem.end = elem.start + 20;
    attest(pbjson_get_object_value(&elem, "payload", &parsed),
           equals(jonmpStringNotTerminated));

    elem.end = elem.start + 26;
    attest(pbjson_get_object_value(&elem, "payload", &parsed),
           equals(jonmpMissingColon));

    elem.end = elem.start + 27;
    attest(pbjson_get_object_value(&elem, "payload", &parsed),
           equals(jonmpMissingValueSeparator));

    elem.start = json + 1;
    attest(pbjson_get_object_value(&elem, "payload", &parsed),
           equals(jonmpNoStartCurly));
    attest(pbjson_object_name_parse_result_2_string(jonmpNoStartCurly),
           differs(NULL));
    attest(strlen(pbjson_object_name_parse_result_2_string(jonmpNoStartCurly)),
           is_greater_than(1));

    char const* json_2 = "{x:2}";
    elem.start         = json_2;
    elem.end           = json_2 + strlen(json_2);
    attest(pbjson_get_object_value(&elem, "payload", &parsed),
           equals(jonmpKeyNotString));
    attest(pbjson_object_name_parse_result_2_string(jonmpKeyNotString),
           differs(NULL));
    attest(strlen(pbjson_object_name_parse_result_2_string(jonmpKeyNotString)),
           is_greater_than(1));

    char const* json_no_colon = "{\"x\" 2}";
    elem.start                = json_no_colon;
    elem.end                  = json_no_colon + strlen(json_no_colon);
    attest(pbjson_get_object_value(&elem, "payload", &parsed),
           equals(jonmpMissingColon));
}


Ensure(/*pbjson_parse, */ get_object_value_key_doesnt_exist)
{
    char const* json = "{\"service\": \"xxx\", \"error\": true, "
                       "\"payload\":{\"group\":\"gr\",\"chan\":[1,2,3]}, "
                       "\"message\":0}";
    struct pbjson_elem elem = { json, json + strlen(json) };
    struct pbjson_elem parsed;

    attest(pbjson_get_object_value(&elem, "zec", &parsed), equals(jonmpKeyNotFound));
    attest(pbjson_get_object_value(&elem, "xxx", &parsed), equals(jonmpKeyNotFound));
    attest(pbjson_get_object_value(&elem, "\"service\"", &parsed),
           equals(jonmpKeyNotFound));
    attest(pbjson_get_object_value(&elem, "servic", &parsed),
           equals(jonmpKeyNotFound));
    attest(pbjson_get_object_value(&elem, "ervice", &parsed),
           equals(jonmpKeyNotFound));
    attest(pbjson_get_object_value(&elem, "essage", &parsed),
           equals(jonmpKeyNotFound));
    attest(pbjson_get_object_value(&elem, "messag", &parsed),
           equals(jonmpKeyNotFound));
    attest(pbjson_get_object_value(&elem, "messagg", &parsed),
           equals(jonmpKeyNotFound));
    attest(pbjson_get_object_value(&elem, "mmessag", &parsed),
           equals(jonmpKeyNotFound));
    attest(pbjson_object_name_parse_result_2_string(jonmpKeyNotFound),
           differs(NULL));
    attest(strlen(pbjson_object_name_parse_result_2_string(jonmpKeyNotFound)),
           is_greater_than(1));
    attest(pbjson_object_name_parse_result_2_string(10), differs(NULL));
    attest(strlen(pbjson_object_name_parse_result_2_string(10)),
           is_greater_than(1));
}

Ensure(/*pbjson_parse, */ incomplete_json)
{
    char const* json = "{\"some\\key\": \"some\\value\",\"service\": \"xxx\", "
                       "\"error\": true, \"payload\":{\"group\":\"gr\", "
                       "\"some\\\\key\": value,\"chan\":[1," /*2,3]}, \"message\":0}"*/;
    struct pbjson_elem elem = { json, json + strlen(json) };
    struct pbjson_elem parsed;

    attest(pbjson_get_object_value(&elem, "some\\key", &parsed), equals(jonmpOK));
    attest(pbjson_elem_equals_string(&parsed, "some\\value"), is_false);
    attest(pbjson_elem_equals_string(&parsed, "\"some\\value\""), is_true);

    attest(pbjson_get_object_value(&elem, "error", &parsed), equals(jonmpOK));
    attest(pbjson_elem_equals_string(&parsed, "true"), is_true);

    attest(pbjson_get_object_value(&elem, "service", &parsed), equals(jonmpOK));
    attest(pbjson_elem_equals_string(&parsed, "\"xxx\""), is_true);
    attest(pbjson_get_object_value(&elem, "payload", &parsed), equals(jonmpValueIncomplete));

    char const* json_2 =
        "{\"some\\key\": \"some\\value\",\"service\": \"xxx\", \"erro";
    elem.start = json_2;
    elem.end   = json_2 + strlen(json_2);
    attest(pbjson_get_object_value(&elem, "error", &parsed),
           equals(jonmpStringNotTerminated));

    attest(pbjson_get_object_value(&elem, "service", &parsed), equals(jonmpOK));
    attest(pbjson_elem_equals_string(&parsed, "\"xxx\""), is_true);
    attest(pbjson_get_object_value(&elem, "payload", &parsed),
           equals(jonmpStringNotTerminated));

    char const* json_3 = "{\"some\\key\": \"some\\value\",\"service\": "
                         "\"xxx\", \"error\":tru\0 }";
    elem.start = json_3;
    elem.end   = json_3 + strlen(json_3) + 3;
    attest(pbjson_get_object_value(&elem, "error", &parsed), equals(jonmpValueIncomplete));

    char const* json_4 =
        "{\"some\\key\": \"some\\value\",\"ser\0ice\": \"xxx\"";
    elem.start = json_4;
    elem.end   = json_4 + strlen(json_4) + 11;
    attest(pbjson_get_object_value(&elem, "service", &parsed),
           equals(jonmpStringNotTerminated));
}

Ensure(/*pbjson_parse, */ gibberish_json)
{
    char const* json = "{\"some\\key\": \"some\\value\",\"service\": \"xxx\", "
                       "\"error\": true, \"payload\":{\"group\":\"gr\", "
                       "\"some\\key\": [{\"chan\":[1,2]}}]";
    struct pbjson_elem elem = { json, json + strlen(json) };
    struct pbjson_elem parsed;

    attest(pbjson_get_object_value(&elem, "some\\key", &parsed), equals(jonmpOK));
    attest(pbjson_elem_equals_string(&parsed, "some\\value"), is_false);
    attest(pbjson_elem_equals_string(&parsed, "\"some\\value\""), is_true);

    attest(pbjson_get_object_value(&elem, "payload", &parsed), equals(jonmpOK));
    attest(pbjson_elem_equals_string(
               &parsed, "{\"group\":\"gr\", \"some\\key\": [{\"chan\":[1,2]}}]"),
           is_true);
}


Describe(single_context_pubnub);

static pubnub_t* pbp;


BeforeEach(single_context_pubnub)
{
    pubnub_setup_mocks(&pbp);
    pubnub_origin_set(pbp, NULL);
}

AfterEach(single_context_pubnub)
{
    pubnub_cleanup_mocks(pbp);
}


void expect_have_dns_for_pubnub_origin()
{
    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbpal_resolv_and_connect,
           when(pb, equals(pbp)),
           returns(pbpal_connect_success));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
}


static inline void expect_outgoing_with_url(char const* url)
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


static inline void incoming_and_close(char const* str, struct uint8_block* p_data)
{
    incoming(str, p_data);
    expect(pbpal_close, when(pb, equals(pbp)), returns(0));
    //    expect(pbpal_closed, when(pb, equals(pbp)), returns(true));
    expect(pbpal_forget, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
}


static void cancel_and_cleanup(pubnub_t* pbp)
{
    expect(pbpal_close, when(pb, equals(pbp)), returns(0));
    expect(pbpal_closed, when(pb, equals(pbp)), returns(true));
    expect(pbpal_forget, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    pubnub_cancel(pbp);
    attest(pbp->core.last_result, equals(PNR_CANCELLED));
}


Ensure(single_context_pubnub, no_user_id_length_limit)
{
    pubnub_init(pbp, "pubkey", "subkey");
    char *user_id = "WC65Z7Gg9fnY6M2XxfxF6Q0JRH8pFRhwDiJsnqtgXs6keqzOE24SqQiE20Yzv4v5wuKtrn96iA7TjDKe3pMeAqW3wYRsjLAVWO1efmIdueuQwlB6DRRgoCSKReczOySSoH9KcJTLtjDZatNFTFyP2IF8jtYtef3P1SOwEWzGtJW6jV67gM0tTPJogEdcxetJhRYU5BtMTrsxA2EMmnZ8AWXZWCPAr3rZ";
    // Test 224 long user id
    pubnub_set_uuid(pbp, user_id);
    attest(pubnub_uuid_get(pbp), streqs(user_id));
    pubnub_set_user_id(pbp, user_id);
    attest(pubnub_user_id_get(pbp), streqs(user_id));
}

/* -- LEAVE operation -- */
Ensure(single_context_pubnub, leave_have_dns)
{
    pubnub_init(pbp, "pubkey", "subkey");
    pubnub_set_user_id(pbp, "test_id");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/subkey/channel/lamanche/"
                             "leave?pnsdk=unit-test-0.1&uuid=test_id");
    incoming("HTTP/1.1 200\r\nContent-Length: 2\r\n\r\n{}", NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_leave(pbp, "lamanche", NULL), equals(PNR_OK));
    attest(pubnub_last_publish_result(pbp), streqs(""));
    attest(pubnub_last_http_code(pbp), equals(200));
}

/* This tests the DNS resolution code. Since we know for sure it is
   the same for all Pubnub operations/transactions, we shall test it
   only for "leave".
*/
Ensure(single_context_pubnub, leave_wait_dns)
{
    pubnub_init(pbp, "pubkey", "subkey");
    pubnub_set_user_id(pbp, "test_id");

    /* DNS resolution not yet available... */
    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(+1));
    expect(pbpal_resolv_and_connect,
           when(pb, equals(pbp)),
           returns(pbpal_resolv_sent));
    expect(pbntf_watch_in_events, when(pb, equals(pbp)), returns(0));
    attest(pubnub_leave(pbp, "lamanche", NULL), equals(PNR_STARTED));

    /* ... still not available... */
    expect(pbpal_check_resolv_and_connect,
           when(pb, equals(pbp)),
           returns(pbpal_resolv_rcv_wouldblock));
    attest(pbnc_fsm(pbp), equals(0));

    /* ... and here it is: */
    expect(pbntf_update_socket, when(pb, equals(pbp)), returns(+1));
    expect(pbntf_watch_out_events, when(pb, equals(pbp)));
    expect(pbpal_check_resolv_and_connect,
           when(pb, equals(pbp)),
           returns(pbpal_connect_success));
    expect_outgoing_with_url("/v2/presence/sub-key/subkey/channel/lamanche/"
                             "leave?pnsdk=unit-test-0.1&uuid=test_id");
    incoming("HTTP/1.1 200\r\nContent-Length: 2\r\n\r\n{}", NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pbnc_fsm(pbp), equals(0));

    attest(pbp->core.last_result, equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));
}


Ensure(single_context_pubnub, leave_wait_dns_cancel)
{
    pubnub_init(pbp, "pubkey", "subkey");
    pubnub_set_user_id(pbp, "test_id");

    /* DNS resolution not yet available... */
    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(+1));
    expect(pbpal_resolv_and_connect,
           when(pb, equals(pbp)),
           returns(pbpal_resolv_sent));
    expect(pbntf_watch_in_events, when(pb, equals(pbp)), returns(0));
    attest(pubnub_leave(pbp, "lamanche", NULL), equals(PNR_STARTED));

    /* ... user is impatient... */
    cancel_and_cleanup(pbp);
}

/* This tests the TCP establishment code. Since we know for sure it is
   the same for all Pubnub operations/transactions, we shall test it
   only for "leave".
 */
Ensure(single_context_pubnub, leave_wait_tcp)
{
    pubnub_init(pbp, "pubkey", "subkey");
    pubnub_set_user_id(pbp, "test_id");

    /* DNS resolved but TCP connection not yet established... */
    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(+1));
    expect(pbpal_resolv_and_connect,
           when(pb, equals(pbp)),
           returns(pbpal_connect_wouldblock));
    attest(pubnub_leave(pbp, "lamanche", NULL), equals(PNR_STARTED));

    /* ... and here it is: */
    expect(pbpal_check_connect, when(pb, equals(pbp)), returns(pbpal_connect_success));
    expect_outgoing_with_url("/v2/presence/sub-key/subkey/channel/lamanche/"
                             "leave?pnsdk=unit-test-0.1&uuid=test_id");
    incoming("HTTP/1.1 200\r\nContent-Length: 2\r\n\r\n{}", NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pbnc_fsm(pbp), equals(0));

    attest(pbp->core.last_result, equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));
}


Ensure(single_context_pubnub, leave_wait_tcp_cancel)
{
    pubnub_init(pbp, "pubkey", "subkey");
    pubnub_set_user_id(pbp, "test_id");

    /* DNS resolved but TCP connection not yet established... */
    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbpal_resolv_and_connect,
           when(pb, equals(pbp)),
           returns(pbpal_connect_wouldblock));
    expect(pbpal_check_connect,
           when(pb, equals(pbp)),
           returns(pbpal_connect_wouldblock));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    attest(pubnub_leave(pbp, "lamanche", NULL), equals(PNR_STARTED));

    /* ... user is impatient... */
    cancel_and_cleanup(pbp);
}


Ensure(single_context_pubnub, leave_changroup)
{
    pubnub_init(pbp, "kpub", "ssub");
    pubnub_set_user_id(pbp, "test_id");

    /* Both channel and channel group set */
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/ssub/channel/k1/"
                             "leave?pnsdk=unit-test-0.1&uuid=test_id&channel-group=tnt");
    incoming("HTTP/1.1 200\r\nContent-Length: 2\r\n\r\n{}", NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_leave(pbp, "k1", "tnt"), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));

    /* Only channel group set */
    /* We dont have to do DNS resolution again on the same context already in
       use. The connection keeps beeing established by default. */
    //    expect_have_dns_for_pubnub_origin();
    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/v2/presence/sub-key/ssub/channel/,/"
                             "leave?pnsdk=unit-test-0.1&uuid=test_id&channel-group=mala");
    incoming("HTTP/1.1 200\r\nContent-Length: 2\r\n\r\n{}", NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_leave(pbp, NULL, "mala"), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));

    /* Neither channel nor channel group set */
    attest(pubnub_leave(pbp, NULL, NULL), equals(PNR_INVALID_CHANNEL));
}


Ensure(single_context_pubnub, leave_uuid_auth)
{
    pubnub_init(pbp, "pubX", "Xsub");
    attest(pbp->core.user_id_len, equals(0));

    /* Set UUID */
    pubnub_set_user_id(pbp, "DEDA-BABACECA-DECA");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/Xsub/channel/k/"
                             "leave?pnsdk=unit-test-0.1&uuid=DEDA-BABACECA-"
                             "DECA");
    incoming("HTTP/1.1 200\r\nContent-Length: 2\r\n\r\n{}", NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_leave(pbp, "k", NULL), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pbp->core.user_id_len, equals(18));

    /* Set auth, too */
    pubnub_set_auth(pbp, "super-secret-key");
    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/v2/presence/sub-key/Xsub/channel/k2/"
                             "leave?pnsdk=unit-test-0.1&uuid=DEDA-BABACECA-"
                             "DECA&auth=super-secret-key");
    incoming("HTTP/1.1 200\r\nContent-Length: 2\r\n\r\n{}", NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_leave(pbp, "k2", NULL), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));

    /* Change UUID */
    pubnub_set_user_id(pbp, "test_id");
    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/v2/presence/sub-key/Xsub/channel/k3/"
                             "leave?pnsdk=unit-test-0.1&uuid=test_id&auth=super-secret-key");
    incoming("HTTP/1.1 200\r\nContent-Length: 2\r\n\r\n{}", NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_leave(pbp, "k3", NULL), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));

    /* Reset auth, too */
    pubnub_set_auth(pbp, NULL);
    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url(
        "/v2/presence/sub-key/Xsub/channel/k4/leave?pnsdk=unit-test-0.1&uuid=test_id");
    incoming("HTTP/1.1 200\r\nContent-Length: 2\r\n\r\n{}", NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_leave(pbp, "k4", NULL), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));
}


Ensure(single_context_pubnub, leave_bad_response)
{
    pubnub_init(pbp, "pubkey", "subkey");
    pubnub_set_user_id(pbp, "test_id");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/subkey/channel/lamanche/"
                             "leave?pnsdk=unit-test-0.1&uuid=test_id");
    incoming("HTTP/1.1 200\r\nContent-Length: 2\r\n\r\n[]", NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_leave(pbp, "lamanche", NULL), equals(PNR_FORMAT_ERROR));
}


Ensure(single_context_pubnub, leave_in_progress)
{
    pubnub_init(pbp, "pubkey", "subkey");
    pubnub_set_user_id(pbp, "test_id");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/subkey/channel/lamanche/"
                             "leave?pnsdk=unit-test-0.1&uuid=test_id");
    incoming("HTTP/1.1 200\r\n", NULL);
    /* simulates 'callback' condition of PNR_IN_PROGRESS.
     * expl: recv_msg would return 0 otherwise as if the connection
     * closes from servers side.
     */
    incoming("", NULL);
    attest(pubnub_leave(pbp, "lamanche", NULL), equals(PNR_STARTED));
    attest(pubnub_leave(pbp, "lamanche", NULL), equals(PNR_IN_PROGRESS));

    cancel_and_cleanup(pbp);
}


/* -- TIME operation -- */


Ensure(single_context_pubnub, time)
{
    pubnub_init(pbp, "tkey", "subt");
    pubnub_set_user_id(pbp, "test_id");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/time/0?pnsdk=unit-test-0.1&uuid=test_id");
    incoming("HTTP/1.1 200\r\nContent-Length: 9\r\n\r\n[1643092]", NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_time(pbp), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_get(pbp), streqs("1643092"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), equals(NULL));
}


Ensure(single_context_pubnub, time_bad_response)
{
    pubnub_init(pbp, "tkey", "subt");
    pubnub_set_user_id(pbp, "test_id");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/time/0?pnsdk=unit-test-0.1&uuid=test_id");
    incoming("HTTP/1.1 200\r\nContent-Length: 9\r\n\r\n{1643092}", NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_time(pbp), equals(PNR_FORMAT_ERROR));

    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), equals(NULL));
}


Ensure(single_context_pubnub, time_in_progress)
{
    pubnub_init(pbp, "pubkey", "subkey");
    pubnub_set_user_id(pbp, "test_id");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/time/0?pnsdk=unit-test-0.1&uuid=test_id");
    incoming("HTTP/1.1 200\r\n", NULL);
    /* incoming empty string simulates conditions for PNR_IN_PROGRESS */
    incoming("", NULL);
    //    expect(pbpal_close, when(pb, equals(pbp)), returns(0));
    attest(pubnub_time(pbp), equals(PNR_STARTED));
    attest(pubnub_time(pbp), equals(PNR_IN_PROGRESS));

    cancel_and_cleanup(pbp);
}

/* -- PUBLISH operation -- */


Ensure(single_context_pubnub, publish)
{
    pubnub_init(pbp, "publkey", "subkey");
    pubnub_set_user_id(pbp, "test_id");

    expect_have_dns_for_pubnub_origin();

    expect_outgoing_with_url(
        "/publish/publkey/subkey/0/jarak/0/%22zec%22?pnsdk=unit-test-0.1&uuid=test_id");
    incoming("HTTP/1.1 200\r\nContent-Length: "
             "30\r\n\r\n[1,\"Sent\",\"14178940800777403\"]",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_last_publish_result(pbp), streqs(""));
    attest(pubnub_publish(pbp, "jarak", "\"zec\""), equals(PNR_OK));
    attest(pubnub_last_publish_result(pbp), streqs("\"Sent\""));
    attest(pubnub_last_http_code(pbp), equals(200));
}

Ensure(single_context_pubnub, publish_change_origin)
{
    pubnub_init(pbp, "publkey", "subkey");
    pubnub_set_user_id(pbp, "test_id");

    attest(pubnub_get_origin(pbp), streqs(PUBNUB_ORIGIN));
    attest(pubnub_origin_set(pbp, "new_origin_server"), equals(0));
    expect_have_dns_for_pubnub_origin();

    expect(pbpal_send_str, when(s, streqs("GET ")), returns(0));
    expect(pbpal_send_status, returns(0));
    expect(pbpal_send_str,
           when(s, streqs("/publish/publkey/subkey/0/jarak/0/%22zec%22?pnsdk=unit-test-0.1&uuid=test_id")),
           returns(0));
    expect(pbpal_send_status, returns(0));
    expect(pbpal_send, when(data, streqs(" HTTP/1.1\r\nHost: ")), returns(0));
    expect(pbpal_send_status, returns(0));
    expect(pbpal_send_str, when(s, streqs("new_origin_server")), returns(0));
    expect(pbpal_send_status, returns(0));
    expect(pbpal_send_str,
           when(s,
                streqs("\r\nUser-Agent: POSIX-PubNub-C-core/" PUBNUB_SDK_VERSION
                       "\r\n" ACCEPT_ENCODING "\r\n")),
           returns(0));
    expect(pbpal_send_status, returns(0));
    expect(pbntf_watch_in_events, when(pb, equals(pbp)), returns(0));
    incoming("HTTP/1.1 200\r\nContent-Length: "
             "30\r\n\r\n[1,\"Sent\",\"14178940800777403\"]",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_last_publish_result(pbp), streqs(""));
    attest(pubnub_publish(pbp, "jarak", "\"zec\""), equals(PNR_OK));
    attest(pubnub_last_publish_result(pbp), streqs("\"Sent\""));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_get_origin(pbp), streqs("new_origin_server"));
}

Ensure(single_context_pubnub, publish_http_chunked)
{
    pubnub_init(pbp, "publkey", "subkey");
    pubnub_set_user_id(pbp, "test_id");

    expect_have_dns_for_pubnub_origin();

    expect_outgoing_with_url(
        "/publish/publkey/subkey/0/jarak/0/%22zec%22?pnsdk=unit-test-0.1&uuid=test_id");
    incoming("HTTP/1.1 200\r\nTransfer-Encoding: "
             "chunked\r\n\r\n12\r\n[1,\"Sent\",\"1417894\r\n0C\r\n0800777403\"]"
             "\r\n0\r\n",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_last_publish_result(pbp), streqs(""));
    attest(pubnub_publish(pbp, "jarak", "\"zec\""), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_last_publish_result(pbp), streqs("\"Sent\""));
}

Ensure(single_context_pubnub, http_headers_no_content_length_or_chunked)
{
    pubnub_init(pbp, "publkey", "subkey");
    pubnub_set_user_id(pbp, "test_id");

    expect_have_dns_for_pubnub_origin();

    expect_outgoing_with_url(
        "/publish/publkey/subkey/0/,/0/%22zec%22?pnsdk=unit-test-0.1&uuid=test_id");
    /* No 'content-length' nor 'chunked' header field */
    incoming("HTTP/1.1 400\r\n\r\n[0,\"Invalid\",\"14178940800999505\"]", NULL);
    expect(pbpal_close, when(pb, equals(pbp)), returns(0));
    expect(pbpal_forget, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_last_publish_result(pbp), streqs(""));
    attest(pubnub_publish(pbp, ",", "\"zec\""), equals(PNR_IO_ERROR));
    attest(pubnub_last_publish_result(pbp), streqs(""));
}

Ensure(single_context_pubnub, publish_failed_invalid_channel)
{
    pubnub_init(pbp, "publkey", "subkey");
    pubnub_set_user_id(pbp, "test_id");

    expect_have_dns_for_pubnub_origin();

    expect_outgoing_with_url(
        "/publish/publkey/subkey/0/,/0/%22zec%22?pnsdk=unit-test-0.1&uuid=test_id");
    incoming("HTTP/1.1 400\r\nContent-Length: "
             "33\r\n\r\n[0,\"Invalid\",\"14178940800999505\"]",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_last_publish_result(pbp), streqs(""));
    attest(pubnub_publish(pbp, ",", "\"zec\""), equals(PNR_PUBLISH_FAILED));
    attest(pubnub_last_publish_result(pbp), streqs("\"Invalid\""));
}


Ensure(single_context_pubnub, publish_msg_too_long)
{
    pubnub_init(pbp, "publkey", "subkey");
    pubnub_set_user_id(pbp, "test_id");

    char msg[PUBNUB_BUF_MAXLEN + 1];
    memset(msg, 'A', sizeof msg);
    msg[sizeof msg - 1] = '\0';
    attest(pubnub_publish(pbp, "w", msg), equals(PNR_TX_BUFF_TOO_SMALL));

    /* URL encoded char */
    memset(msg, '"', sizeof msg);
    msg[sizeof msg - 1] = '\0';
    attest(pubnub_publish(pbp, "w", msg), equals(PNR_TX_BUFF_TOO_SMALL));
    attest(pubnub_last_publish_result(pbp), streqs(""));
}


Ensure(single_context_pubnub, publish_in_progress)
{
    pubnub_init(pbp, "pubkey", "subkey");
    pubnub_set_user_id(pbp, "test_id");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url(
        "/publish/pubkey/subkey/0/jarak/0/4443?pnsdk=unit-test-0.1&uuid=test_id");
    incoming("HTTP/1.1 200\r\n", NULL);
    /* simulates recv_msg() < 0 which saves from closed connection */
    incoming("", NULL);
    attest(pubnub_publish(pbp, "jarak", "4443"), equals(PNR_STARTED));
    attest(pubnub_publish(pbp, "x", "0"), equals(PNR_IN_PROGRESS));

    cancel_and_cleanup(pbp);
}


Ensure(single_context_pubnub, publish_uuid_auth)
{
    pubnub_init(pbp, "pubX", "Xsub");

    /* Set UUID */
    pubnub_set_user_id(pbp, "0ADA-BEDA-0000");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/publish/pubX/Xsub/0/k/0/"
                             "4443?pnsdk=unit-test-0.1&uuid=0ADA-BEDA-0000");
    incoming("HTTP/1.1 200\r\nContent-Length: 3\r\n\r\n[1]", NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_publish(pbp, "k", "4443"), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pbp->core.user_id_len, equals(14));

    /* Set auth, too */
    pubnub_set_auth(pbp, "bad-secret-key");
    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/publish/pubX/Xsub/0/k2/0/"
                             "443?pnsdk=unit-test-0.1&uuid=0ADA-BEDA-0000&auth="
                             "bad-secret-key");
    incoming("HTTP/1.1 200\r\nContent-Length: 3\r\n\r\n[1]", NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_publish(pbp, "k2", "443"), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));

    /* Change UUID */
    pubnub_set_user_id(pbp, "test_id");
    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/publish/pubX/Xsub/0/k3/0/"
                             "4443?pnsdk=unit-test-0.1&uuid=test_id&auth=bad-secret-key");
    incoming("HTTP/1.1 200\r\nContent-Length: 3\r\n\r\n[1]", NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_publish(pbp, "k3", "4443"), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));

    /* Reset auth, too */
    pubnub_set_auth(pbp, NULL);
    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url(
        "/publish/pubX/Xsub/0/k4/0/443?pnsdk=unit-test-0.1&uuid=test_id");
    incoming("HTTP/1.1 200\r\nContent-Length: 3\r\n\r\n[1]", NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_publish(pbp, "k4", "443"), equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));
}

Ensure(single_context_pubnub, publish_bad_response)
{
    pubnub_init(pbp, "tkey", "subt");
    pubnub_set_user_id(pbp, "test_id");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url(
        "/publish/tkey/subt/0/k6/0/443?pnsdk=unit-test-0.1&uuid=test_id");
    incoming("HTTP/1.1 200\r\nContent-Length: 9\r\n\r\n<\"1\":\"X\">", NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_publish(pbp, "k6", "443"), equals(PNR_FORMAT_ERROR));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_last_publish_result(pbp), streqs(""));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), equals(NULL));
}


Ensure(single_context_pubnub, publish_failed_server_side)
{
    pubnub_init(pbp, "tkey", "subt");
    pubnub_set_user_id(pbp, "test_id");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url(
        "/publish/tkey/subt/0/k6/0/443?pnsdk=unit-test-0.1&uuid=test_id");
    incoming("HTTP/1.1 200\r\nContent-Length: 9\r\n\r\n{\"1\":\"X\"}", NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_publish(pbp, "k6", "443"), equals(PNR_PUBLISH_FAILED));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_last_publish_result(pbp), streqs("\"1\":\"X\"}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), equals(NULL));
}

/* -- HISTORY operation -- */


Ensure(single_context_pubnub, history_without_timetoken)
{
    pubnub_init(pbp, "publhis", "subhis");
    pubnub_set_user_id(pbp, "test_id");

    /* Without time-token */
    expect_have_dns_for_pubnub_origin();

    expect_outgoing_with_url("/v2/history/sub-key/subhis/channel/"
                             "ch?pnsdk=unit-test-0.1&uuid=test_id&count=22&include_token="
                             "false");
    incoming("HTTP/1.1 200\r\nContent-Length: "
             "45\r\n\r\n[[1,2,3],14370854953886727,14370864554607266]",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_history(pbp, "ch", 22, false), equals(PNR_OK));

    attest(pubnub_get(pbp), streqs("[1,2,3]"));
    attest(pubnub_get(pbp), streqs("14370854953886727"));
    attest(pubnub_get(pbp), streqs("14370864554607266"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
}

Ensure(single_context_pubnub, history_with_timetoken)
{
    pubnub_init(pbp, "publhis", "subhis");
    pubnub_set_user_id(pbp, "test_id");

    /* With time-token */
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/history/sub-key/subhis/channel/"
                             "ch?pnsdk=unit-test-0.1&uuid=test_id&count=22&include_token="
                             "true");
    incoming("HTTP/1.1 200\r\nContent-Length: "
             "171\r\n\r\n[[{\"message\":1,\"timetoken\":14370863460777883},{"
             "\"message\":2,\"timetoken\":14370863461279046},{\"message\":3,"
             "\"timetoken\":14370863958459501}],14370863460777883,"
             "14370863958459501]",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_history(pbp, "ch", 22, true), equals(PNR_OK));

    attest(pubnub_get(pbp),
           streqs("[{\"message\":1,\"timetoken\":14370863460777883},{"
                  "\"message\":2,\"timetoken\":14370863461279046},{\"message\":"
                  "3,\"timetoken\":14370863958459501}]"));
    attest(pubnub_get(pbp), streqs("14370863460777883"));
    attest(pubnub_get(pbp), streqs("14370863958459501"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
}


Ensure(single_context_pubnub, history_in_progress)
{
    pubnub_init(pbp, "publhis", "subhis");
    pubnub_set_user_id(pbp, "test_id");

    expect_have_dns_for_pubnub_origin();

    expect_outgoing_with_url("/v2/history/sub-key/subhis/channel/"
                             "ch?pnsdk=unit-test-0.1&uuid=test_id&count=22&include_token="
                             "false");
    incoming("HTTP/1.1 200\r\n", NULL);
    incoming("", NULL);
    attest(pubnub_history(pbp, "ch", 22, false), equals(PNR_STARTED));
    attest(pubnub_history(pbp, "x", 55, false), equals(PNR_IN_PROGRESS));

    cancel_and_cleanup(pbp);
}


Ensure(single_context_pubnub, history_auth)
{
    pubnub_init(pbp, "pubX", "Xsub");
    pubnub_set_user_id(pbp, "test_id");

    pubnub_set_auth(pbp, "go-secret-key");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/history/sub-key/Xsub/channel/"
                             "hhh?pnsdk=unit-test-0.1&uuid=test_id&count="
                             "40&include_token=false&auth=go-secret-key");
    incoming("HTTP/1.1 200\r\nContent-Length: 2\r\n\r\n[]", NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_history(pbp, "hhh", 40, false), equals(PNR_OK));

    attest(pubnub_last_http_code(pbp), equals(200));
}


Ensure(single_context_pubnub, history_bad_response)
{
    pubnub_init(pbp, "pubkey", "Xsub");
    pubnub_set_user_id(pbp, "test_id");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/history/sub-key/Xsub/channel/"
                             "ttt?pnsdk=unit-test-0.1&uuid=test_id&count=10&include_token="
                             "false");
    incoming("HTTP/1.1 200\r\nContent-Length: 2\r\n\r\n{}", NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_history(pbp, "ttt", 10, false), equals(PNR_FORMAT_ERROR));
}

/* -- ADVANCED HISTORY message_counts -- */

#if PUBNUB_USE_ADVANCED_HISTORY
Ensure(single_context_pubnub, gets_advanced_history_message_counts_for_two_channels_since_timetoken)
{
    size_t io_count = 2;
    struct pubnub_chan_msg_count chan_msg_counters[2];
    pubnub_chamebl_t channel_1 = {"some", sizeof "some" - 1};
    pubnub_chamebl_t channel_2 = {"other", sizeof "other" - 1};
    
    pubnub_init(pbp, "pub-nina", "sub-pinta");
    pubnub_set_user_id(pbp, "test_id");

    expect_have_dns_for_pubnub_origin();

    expect_outgoing_with_url("/v3/history/sub-key/sub-pinta/message-counts/"
                             "some,other?pnsdk=unit-test-0.1&uuid=test_id&timetoken=14378854953886727");
    incoming("HTTP/1.1 200\r\nContent-Length: 85\r\n\r\n"
             "{\"status\":200, \"error\": false, \"error_message\": \"\", \"channels\": {\"some\":1,\"other\":5}}",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_message_counts(pbp, "some,other", "14378854953886727"), equals(PNR_OK));

    attest(pubnub_get_chan_msg_counts_size(pbp), equals(2));
    attest(pubnub_get_chan_msg_counts(pbp, &io_count, chan_msg_counters), equals(0));
    attest(io_count, equals(2));
    attest(strncmp(chan_msg_counters[0].channel.ptr, channel_1.ptr, channel_1.size), equals(0));
    attest(chan_msg_counters[0].channel.size, equals(channel_1.size));
    attest(chan_msg_counters[0].message_count, equals(1));
    attest(strncmp(chan_msg_counters[1].channel.ptr, channel_2.ptr, channel_2.size), equals(0));
    attest(chan_msg_counters[1].channel.size, equals(channel_2.size));
    attest(chan_msg_counters[1].message_count, equals(5));
    
    attest(pubnub_last_http_code(pbp), equals(200));
}

Ensure(single_context_pubnub, gets_message_counts_for_two_channels_since_channel_timetokens)
{
    size_t io_count = 2;
    struct pubnub_chan_msg_count chan_msg_counters[2];
    pubnub_chamebl_t channel_1 = {"ocean", sizeof "ocean" - 1};
    pubnub_chamebl_t channel_2 = {"wind", sizeof "wind" - 1};
    
    pubnub_init(pbp, "pub-mission", "sub-santa-maria");
    pubnub_set_user_id(pbp, "test_id");

    expect_have_dns_for_pubnub_origin();

    expect_outgoing_with_url("/v3/history/sub-key/sub-santa-maria/message-counts/"
                             "ocean,wind?pnsdk=unit-test-0.1&uuid=test_id&channelsTimetoken="
                             "14378854953886727,14378856783886727");
    incoming("HTTP/1.1 200\r\nContent-Length: 87\r\n\r\n"
             "{\"status\":200, \"error\": false, \"error_message\": \"\", \"channels\": {\"wind\":18,\"ocean\":76}}",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_message_counts(pbp,
                                 "ocean,wind",
                                 "14378854953886727,14378856783886727"),
           equals(PNR_OK));

    attest(pubnub_get_chan_msg_counts_size(pbp), equals(2));
    attest(pubnub_get_chan_msg_counts(pbp, &io_count, chan_msg_counters), equals(0));
    attest(io_count, equals(2));
    attest(strncmp(chan_msg_counters[0].channel.ptr, channel_2.ptr, channel_2.size), equals(0));
    attest(chan_msg_counters[0].channel.size, equals(channel_2.size));
    attest(chan_msg_counters[0].message_count, equals(18));
    attest(strncmp(chan_msg_counters[1].channel.ptr, channel_1.ptr, channel_1.size), equals(0));
    attest(chan_msg_counters[1].channel.size, equals(channel_1.size));
    attest(chan_msg_counters[1].message_count, equals(76));
    /* Message is read */
    attest(pubnub_get_chan_msg_counts(pbp, &io_count, chan_msg_counters), equals(0));
    attest(io_count, equals(0));
    
    attest(pubnub_last_http_code(pbp), equals(200));
}

Ensure(single_context_pubnub,
       get_message_counts_gets_message_counts_for_three_channels_since_channel_timetokens)
{
    int o_count[3];
    
    pubnub_init(pbp, "pub-delta", "sub-echo");
    pubnub_set_user_id(pbp, "test_id");

    expect_have_dns_for_pubnub_origin();

    expect_outgoing_with_url("/v3/history/sub-key/sub-echo/message-counts/"
                             "bravo,alfa,charlie?pnsdk=unit-test-0.1&uuid=test_id&channelsTimetoken="
                             "15378854953886727,15378856783886727,15378856783886727");
    incoming("HTTP/1.1 200\r\nContent-Length: 106\r\n\r\n"
             "{\"status\":200, \"error\": false, \"error_message\": \"\", \"channels\": {\"charlie\":25,\"alfa\":196 ,  \"bravo\":3 }  }",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_message_counts(pbp,
                                 "bravo,alfa,charlie",
                                 "15378854953886727,15378856783886727,15378856783886727"),
           equals(PNR_OK));

    attest(pubnub_get_chan_msg_counts_size(pbp), equals(sizeof o_count/sizeof o_count[0]));
    attest(pubnub_get_message_counts(pbp, "alfa,charlie,bravo", o_count), equals(0));
    attest(o_count[0], equals(196));
    attest(o_count[1], equals(25));
    attest(o_count[2], equals(3));
    /* Message is already read in full */
    attest(pubnub_get_chan_msg_counts_size(pbp), equals(0));
    
    attest(pubnub_last_http_code(pbp), equals(200));
}

Ensure(single_context_pubnub,
       handles_an_error_reported_from_server_on_message_counts_request)
{
    int o_count[3];
    char msg[sizeof "there must be some kind of mistake"];
    pubnub_chamebl_t o_msg = {msg,}; 
    
    pubnub_init(pbp, "pub-foxtrot", "sub-golf");
    pubnub_set_user_id(pbp, "test_id");

    expect_have_dns_for_pubnub_origin();

    expect_outgoing_with_url("/v3/history/sub-key/sub-golf/message-counts/"
                             "hotel?pnsdk=unit-test-0.1&uuid=test_id&timetoken="
                             "15578854953886727");
    incoming("HTTP/1.1 404\r\nContent-Length: 105\r\n\r\n"
             "{\"status\":404 , \"error\": true, \"error_message\"  : \"there must be some kind of mistake\", \"channels\": {}  }",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_message_counts(pbp,
                                 "hotel",
                                 "15578854953886727"),
           equals(PNR_ERROR_ON_SERVER));

    attest(pubnub_get_error_message(pbp, &o_msg), equals(0));
    attest(o_msg.size, equals(sizeof "there must be some kind of mistake" - 1));
    attest(strncmp(o_msg.ptr, "there must be some kind of mistake", o_msg.size), equals(0));
    /* Using the same test, but assuming that it was successful response on corresponding
       pubnub_message_counts() request that got object 'channels', but without any values in it.
       pubnub_get_message_counts() will set corresponding array members(message counters) to
       negative values.
     */
    attest(pubnub_get_message_counts(pbp, "kilo,lima,mike", o_count), equals(0));
    attest(o_count[0], is_less_than(0));
    attest(o_count[1], is_less_than(0));
    attest(o_count[2], is_less_than(0));
    
    attest(pubnub_last_http_code(pbp), equals(404));
}

Ensure(single_context_pubnub,
       handles_wrong_format_in_the_response_on_message_counts_request)
{
    char msg[sizeof "some kind of mistake"];
    pubnub_chamebl_t o_msg = {msg,}; 

    pubnub_init(pbp, "pub-november", "sub-oscar");
    pubnub_set_user_id(pbp, "test_id");

    expect_have_dns_for_pubnub_origin();

    expect_outgoing_with_url("/v3/history/sub-key/sub-oscar/message-counts/"
                             "papa,quebec?pnsdk=unit-test-0.1&uuid=test_id&channelsTimetoken="
                             "15578854953886727,15516381360410684");
    /* Starting 'curly' is missing */
    incoming("HTTP/1.1 404\r\nContent-Length: 104\r\n\r\n"
             "\"status\":404 , \"error\": true, \"error_message\"  : \"there must be some kind of mistake\", \"channels\": {}  }",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_message_counts(pbp,
                                 "papa,quebec",
                                 "15578854953886727,15516381360410684"),
           equals(PNR_FORMAT_ERROR));

    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/v3/history/sub-key/sub-oscar/message-counts/"
                             "papa?pnsdk=unit-test-0.1&uuid=test_id&timetoken="
                             "15578854953886727");
    /* 'error_message' is missing */
    incoming("HTTP/1.1 404\r\nContent-Length: 47\r\n\r\n"
             "{\"status\":404 , \"error\": true, \"channels\": {} }",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_message_counts(pbp,
                                 "papa",
                                 "15578854953886727"),
           equals(PNR_ERROR_ON_SERVER));
    attest(pubnub_get_error_message(pbp, &o_msg), equals(-1));
    
    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/v3/history/sub-key/sub-oscar/message-counts/"
                             "papa?pnsdk=unit-test-0.1&uuid=test_id&timetoken="
                             "15378854953886727");
    /* 'error' is missing */
    incoming("HTTP/1.1 404\r\nContent-Length: 90\r\n\r\n"
             "{\"status\":404 , \"error_message\"  : \"there must be some kind of mistake\", \"channels\": {}  }",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_message_counts(pbp,
                                 "papa",
                                 "15378854953886727"),
           equals(PNR_FORMAT_ERROR));

    attest(pubnub_last_http_code(pbp), equals(404));
    
    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/v3/history/sub-key/sub-oscar/message-counts/"
                             "papa?pnsdk=unit-test-0.1&uuid=test_id&timetoken="
                             "15378854953886727");
    /* 'channels' are missing */
    incoming("HTTP/1.1 200\r\nContent-Length: 76\r\n\r\n"
             "{\"status\":200 , \"error\": false, \"error_message\"  : \"some kind of mistake\"  }",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_message_counts(pbp,
                                 "papa",
                                 "15378854953886727"),
           equals(PNR_FORMAT_ERROR));
    
    attest(pubnub_last_http_code(pbp), equals(200));
}

Ensure(single_context_pubnub,
       handles_irregular_use_of_advanced_history_message_counts_functions)
{
    size_t io_count;
    struct pubnub_chan_msg_count chan_msg_counters[1];
    int o_count[3];
    char msg[10];
    pubnub_chamebl_t o_msg = {msg,}; 
    
    pubnub_init(pbp, "pub-india", "sub-juliett");
    pubnub_set_user_id(pbp, "test_id");

    expect_have_dns_for_pubnub_origin();

    expect_outgoing_with_url("/v3/history/sub-key/sub-juliett/message-counts/"
                             "kilo,lima,mike?pnsdk=unit-test-0.1&uuid=test_id&channelsTimetoken="
                             "15378856783886727,15378856783886727,15378854953886727");
    incoming("HTTP/1.1 200\r\nContent-Length: 130\r\n\r\n"
             "{\"status\":200, \"error\": false, \"error_message\"",
             NULL);
    /* incoming empty string simulates conditions for PNR_IN_PROGRESS */
    incoming("", NULL);
    attest(pubnub_message_counts(pbp,
                                 "kilo,lima,mike",
                                 "15378856783886727,15378856783886727,15378854953886727"),
           equals(PNR_STARTED));
    /* None of these functions should do anything since transaction is in progress */
    attest(pubnub_get_error_message(pbp, &o_msg), equals(-1));
    attest(pubnub_get_chan_msg_counts_size(pbp), equals(-1));
    attest(pubnub_get_chan_msg_counts(pbp, &io_count, chan_msg_counters), equals(-1));
    attest(pubnub_get_message_counts(pbp, "kilo,lima,mike", o_count), equals(-1));
    attest(pubnub_message_counts(pbp,
                                 "bravo,alfa,charlie",
                                 "15378856783886727,15378856783886727,15378854953886727"),
           equals(PNR_IN_PROGRESS));
    
    incoming("  : \"there is no mistake\", \"channels\": { \"mike\" :  52 , \"kilo\":75 ,  \"lima\" : 0 }  }",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pbnc_fsm(pbp), equals(0));
    attest(pbnc_fsm(pbp), equals(0));
    attest(pbp->core.last_result, equals(PNR_OK));

    /* Transaction just finished. Have to read the response first before we start a new one */
    attest(pubnub_message_counts(pbp,
                                 "bravo,alfa,charlie",
                                 "15578856783886727,15578856783886727,15578854953886727"),
           equals(PNR_RX_BUFF_NOT_EMPTY));
    
    attest(pubnub_get_chan_msg_counts_size(pbp), equals(sizeof o_count/sizeof o_count[0]));
    attest(pubnub_get_message_counts(pbp, "kilo,lima,mike ", o_count), equals(0));
    attest(o_count[0], equals(75));
    attest(o_count[1], equals(0));
    attest(o_count[2], equals(52));
    
    attest(pbp->core.last_result, equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));
}

Ensure(single_context_pubnub,
       get_chan_msg_counts_gets_message_counts_for_two_channels_but_the_answer_has_three)
{
    struct pubnub_chan_msg_count chan_msg_counters[2];
    size_t io_count = sizeof chan_msg_counters/sizeof chan_msg_counters[0];
    pubnub_chamebl_t channel_1 = {"tango", sizeof "tango" - 1};
    pubnub_chamebl_t channel_2 = {"sierra", sizeof "sierra" - 1};
    
    pubnub_init(pbp, "pub-quebeq", "sub-romeo");
    pubnub_set_user_id(pbp, "test_id");

    expect_have_dns_for_pubnub_origin();

    expect_outgoing_with_url("/v3/history/sub-key/sub-romeo/message-counts/"
                             "sierra,tango?pnsdk=unit-test-0.1&uuid=test_id&channelsTimetoken="
                             "15378854953886727,15378856783886727");
    incoming("HTTP/1.1 200\r\nContent-Length: 112\r\n\r\n"
             "{\"status\":200, \"error\": false, \"error_message\": \"\", \"channels\": { \"tango\":38 ,  \"sierra\":17 , \"uniform\": 51 }  }",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_message_counts(pbp,
                                 "sierra,tango",
                                 "15378854953886727,15378856783886727"),
           equals(PNR_OK));

    attest(pubnub_get_chan_msg_counts_size(pbp), equals(3));
    attest(pubnub_get_chan_msg_counts(pbp, &io_count, chan_msg_counters), equals(0));
    attest(io_count, equals(sizeof chan_msg_counters/sizeof chan_msg_counters[0]));
    attest(strncmp(chan_msg_counters[0].channel.ptr, channel_1.ptr, channel_1.size), equals(0));
    attest(chan_msg_counters[0].channel.size, equals(channel_1.size));
    attest(chan_msg_counters[0].message_count, equals(38));
    attest(strncmp(chan_msg_counters[1].channel.ptr, channel_2.ptr, channel_2.size), equals(0));
    attest(chan_msg_counters[1].channel.size, equals(channel_2.size));
    attest(chan_msg_counters[1].message_count, equals(17));

    /* Message is read */
    attest(pubnub_get_chan_msg_counts_size(pbp), equals(0));
    
    attest(pubnub_last_http_code(pbp), equals(200));
}

Ensure(single_context_pubnub,
       advanced_history_message_counts_handles_invalid_timetokens)
{
    pubnub_init(pbp, "pub-delta", "sub-echo");
    pubnub_set_user_id(pbp, "test_id");

    attest(pubnub_message_counts(pbp, "victor", "153"), equals(PNR_INVALID_PARAMETERS));
    attest(pubnub_message_counts(pbp,
                                 "victor",
                                 " 15378854953886727"),
           equals(PNR_INVALID_PARAMETERS));
    attest(pubnub_message_counts(pbp,
                                 "victor,whiskey",
                                 "15378854953886727 ,15378856783886727"),
           equals(PNR_INVALID_PARAMETERS));
    attest(pubnub_message_counts(pbp,
                                 "victor,whiskey",
                                 "15378854953886727,727"),
           equals(PNR_INVALID_PARAMETERS));
    attest(pubnub_message_counts(pbp,
                                 "victor,whiskey",
                                 "727,15378854953886727"),
           equals(PNR_INVALID_PARAMETERS));
    attest(pubnub_message_counts(pbp,
                                 "victor",
                                 "15378854953886727,15378856783886727"),
           equals(PNR_INVALID_PARAMETERS));
    attest(pubnub_message_counts(pbp,
                                 "x-ray, yankee, zulu",
                                 "15378854953886727,15378856783886727"),
           equals(PNR_INVALID_PARAMETERS));
}

Ensure(single_context_pubnub,
       advanced_history_message_counts_handles_invalid_channels)
{
    char channel_name_too_long[PUBNUB_MAX_CHANNEL_NAME_LENGTH + 4];
    
    pubnub_init(pbp, "pub-delta", "sub-echo");
    pubnub_set_user_id(pbp, "test_id");

    memset(channel_name_too_long, 'a', sizeof channel_name_too_long - 1);
    channel_name_too_long[sizeof channel_name_too_long - 1] = '\0';
    attest(pubnub_message_counts(pbp,
                                 channel_name_too_long,
                                 "15378854953886727"),
           equals(PNR_INVALID_PARAMETERS));
    /* comma just after the beginning of the channel list(one character in between) */
    channel_name_too_long[1] = ',';
    attest(pubnub_message_counts(pbp,
                                 channel_name_too_long,
                                 "15378854953886727"),
           equals(PNR_INVALID_PARAMETERS));
    attest(pubnub_message_counts(pbp,
                                 channel_name_too_long,
                                 "15378854953886727,15378856783886727"),
           equals(PNR_INVALID_PARAMETERS));
    /* comma just before the end of channel list string(one character in between) */
    channel_name_too_long[1] = 'b';
    channel_name_too_long[sizeof channel_name_too_long - 3] = ',';
    attest(pubnub_message_counts(pbp,
                                 channel_name_too_long,
                                 "15378854953886727"),
           equals(PNR_INVALID_PARAMETERS));
    attest(pubnub_message_counts(pbp,
                                 channel_name_too_long,
                                 "15378854953886727,15378856783886727"),
           equals(PNR_INVALID_PARAMETERS));
}

#endif /* -- ADVANCED HISTORY message_counts -- */

/* -- SET_STATE operation -- */


Ensure(single_context_pubnub, set_state)
{
    pubnub_init(pbp, "publhis", "subhis");

    /* with uuid from context */
    pubnub_set_user_id(pbp, "universal");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/subhis/channel/ch/uuid/"
                             "universal/data?pnsdk=unit-test-0.1&uuid=universal&state=%7B%7D");
    incoming("HTTP/1.1 200\r\nContent-Length: 67\r\n\r\n{\"status\": "
             "200,\"message\":\"OK\", \"service\": \"Presence\", "
             "\"payload\":{}}",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_set_state(pbp, "ch", NULL, NULL, "{}"), equals(PNR_OK));
    attest(pbp->core.user_id_len, equals(9));

    attest(pubnub_get(pbp), streqs("{\"status\": 200,\"message\":\"OK\", \"service\": \"Presence\", \"payload\":{}}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));

    /* with 'auth' set in context and 'uuid' from parameter to the function */
    pubnub_set_auth(pbp, "auth-key");
    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/v2/presence/sub-key/subhis/channel/ch/uuid/"
                             "shazalakazoo/"
                             "data?pnsdk=unit-test-0.1&uuid=universal&auth=auth-key&state=%7B%7D");
    incoming("HTTP/1.1 200\r\nContent-Length: "
             "63\r\n\r\n{\"status\":200,\"message\":\"OK\",\"service\":"
             "\"Presence\",\"payload\":{}}",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_set_state(pbp, "ch", NULL, "shazalakazoo", "{}"), equals(PNR_OK));

    attest(pubnub_get(pbp), streqs("{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\",\"payload\":{}}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));

    /* set state to 'channel group(s)' with 'auth' set in context and 'uuid'
     * from parameter to the function */
    pubnub_set_auth(pbp, "authentic");
    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/v2/presence/sub-key/subhis/channel/,/uuid/"
                             "melwokee/"
                             "data?pnsdk=unit-test-0.1&channel-"
                             "group=[gr1,gr2]&uuid=universal&auth=authentic&state=%7BIOW%7D");
    incoming("HTTP/1.1 200\r\nContent-Length: "
             "66\r\n\r\n{\"status\":200,\"message\":\"OK\",\"service\":"
             "\"Presence\",\"payload\":{IOW}}",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_set_state(pbp, NULL, "[gr1,gr2]", "melwokee", "{IOW}"),
           equals(PNR_OK));

    attest(pubnub_get(pbp), streqs("{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\",\"payload\":{IOW}}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));

    /* set state to 'channel(s)' and 'channel group(s)' with 'auth' set in
     * context and 'uuid' from parameter to the function */
    pubnub_set_auth(pbp, "three");
    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/v2/presence/sub-key/subhis/channel/[ch1,ch2]/"
                             "uuid/linda-darnell/"
                             "data?pnsdk=unit-test-0.1&channel-group="
                             "[gr3,gr4]&uuid=universal&auth=three&state=%7BI%7D");
    incoming("HTTP/1.1 200\r\nContent-Length: "
             "64\r\n\r\n{\"status\":200,\"message\":\"OK\",\"service\":"
             "\"Presence\",\"payload\":{I}}",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_set_state(pbp, "[ch1,ch2]", "[gr3,gr4]", "linda-darnell", "{I}"),
           equals(PNR_OK));

    attest(pubnub_get(pbp), streqs("{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\",\"payload\":{I}}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));

    /* set state for no 'channel' and no 'channel group'.
     */
    pubnub_set_auth(pbp, NULL); // with or without this line
    attest(pubnub_set_state(pbp, NULL, NULL, "linda-darnell", "{I}"),
           equals(PNR_INVALID_CHANNEL));
}

Ensure(single_context_pubnub, set_state_in_progress)
{
    pubnub_init(pbp, "publ-one", "sub-one");
    pubnub_set_user_id(pbp, "test_id");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url(
        "/v2/presence/sub-key/sub-one/channel/ch/uuid/blackbeard/"
        "data?pnsdk=unit-test-0.1&uuid=test_id&state=%7B%22the_pirate%22%3A%22true%22%7D");
    incoming("HTTP/1.1 200\r\n", NULL);
    incoming("", NULL);
    attest(pubnub_set_state(pbp, "ch", NULL, "blackbeard", "{\"the_pirate\":\"true\"}"),
           equals(PNR_STARTED));
    attest(pubnub_set_state(
               pbp, "ch", NULL, "blackbeard", "{\"the_pirate\":\"arrrrrrr_arrrrr\"}"),
           equals(PNR_IN_PROGRESS));

    cancel_and_cleanup(pbp);
}

Ensure(single_context_pubnub, set_state_in_progress_interrupted_and_accomplished)
{
    pubnub_init(pbp, "publ-one", "sub-one");
    pubnub_set_user_id(pbp, "test_id");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url(
        "/v2/presence/sub-key/sub-one/channel/ch/uuid/blackbeard/"
        "data?pnsdk=unit-test-0.1&uuid=test_id&state=%7B%22the_pirate%22%3A%22true%22%7D");
    /* incoming first message */
    incoming("HTTP/1.1 200\r\n", NULL);
    incoming("", NULL);

    attest(pubnub_set_state(pbp, "ch", NULL, "blackbeard", "{\"the_pirate\":\"true\"}"),
           equals(PNR_STARTED));
    attest(pubnub_set_state(
               pbp, "ch", NULL, "blackbeard", "{\"the_pirate\":\"arrrrrrr_arrrrr\"}"),
           equals(PNR_IN_PROGRESS));

    /* incoming second and last message */
    incoming("Content-Length: "
             "82\r\n\r\n{\"status\":200,\"message\":\"OK\",\"service\":"
             "\"Presence\",\"payload\":{\"the_pirate\":\"true\"}}",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pbnc_fsm(pbp), equals(0));
    attest(pbp->core.last_result, equals(PNR_OK));

    attest(pubnub_get(pbp), streqs("{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\",\"payload\":{\"the_pirate\":\"true\"}}"));
    attest(pubnub_get(pbp), equals(NULL));


    attest(pbp->core.last_result, equals(PNR_OK));
    attest(pubnub_last_http_code(pbp), equals(200));
}


Ensure(single_context_pubnub, set_state_set_auth_and_uuid)
{
    pubnub_init(pbp, "pubX", "Xsub");

    pubnub_set_auth(pbp, "portobello");
    pubnub_set_user_id(pbp, "morgan");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/Xsub/channel/ch/uuid/morgan/"
                             "data?pnsdk=unit-test-0.1&uuid=morgan&auth=portobello&state=%7B%22the_privateer%22%3A%22letter_of_marque%22%7D");
    incoming("HTTP/1.1 200\r\nContent-Length: "
             "96\r\n\r\n{\"status\":200,\"message\":\"OK\",\"service\":"
             "\"Presence\",\"payload\":{\"the_privateer\":\"letter_of_"
             "marque\"}",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_set_state(
               pbp, "ch", NULL, NULL, "{\"the_privateer\":\"letter_of_marque\"}"),
           equals(PNR_OK));

    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pbp->core.user_id_len, equals(6));
}

Ensure(single_context_pubnub, set_state_bad_response)
{
    pubnub_init(pbp, "pubkey", "Xsub");

    pubnub_set_user_id(pbp, "chili_peppers");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url(
        "/v2/presence/sub-key/Xsub/channel/ch/uuid/chili_peppers/"
        "data?pnsdk=unit-test-0.1&uuid=chili_peppers&state=%7B%22chili%22%3A%22red%22%7D");
    incoming("HTTP/1.1 200\r\nContent-Length: 2\r\n\r\n<chiki, chiki bum, "
             "chiki bum]",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_set_state(pbp, "ch", NULL, NULL, "{\"chili\":\"red\"}"),
           equals(PNR_FORMAT_ERROR));
    attest(pbp->core.user_id_len, equals(13));
}


/* STATE_GET operation */

Ensure(single_context_pubnub, state_get_1channel)
{
    pubnub_init(pbp, "key", "subY");

    /* with uuid from context */
    pubnub_set_user_id(pbp, "speedy");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url(
        "/v2/presence/sub-key/subY/channel/ch/uuid/speedy?pnsdk=unit-test-0.1&uuid=speedy");
    incoming("HTTP/1.1 200\r\nContent-Length: 76\r\n\r\n{\"status\": "
             "200,\"message\":\"OK\", \"service\": \"Presence\", "
             "\"payload\":{\"running\"}}",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_state_get(pbp, "ch", NULL, NULL), equals(PNR_OK));
    attest(pbp->core.user_id_len, equals(6));

    attest(pubnub_get(pbp), streqs("{\"status\": 200,\"message\":\"OK\", \"service\": \"Presence\", \"payload\":{\"running\"}}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));

    /* with 'auth' set in context and 'uuid' from parameter to the function */
    pubnub_set_auth(pbp, "auth-key");
    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/v2/presence/sub-key/subY/channel/ch/uuid/"
                             "brza_fotografija?pnsdk=unit-test-0.1&uuid=speedy&auth=auth-"
                             "key");
    incoming("HTTP/1.1 200\r\nContent-Length: "
             "72\r\n\r\n{\"status\":200,\"message\":\"OK\",\"service\":"
             "\"Presence\",\"payload\":{key:value}}",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_state_get(pbp, "ch", NULL, "brza_fotografija"), equals(PNR_OK));

    attest(pubnub_get(pbp), streqs("{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\",\"payload\":{key:value}}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
}

Ensure(single_context_pubnub, state_get_channelgroup)
{
    pubnub_init(pbp, "key", "subY");
    pubnub_set_user_id(pbp, "test_id");

    /* state_get to 'channel group(s)' with 'auth' set in context and 'uuid'
     * from parameter to the function */
    pubnub_set_auth(pbp, "mouth");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/subY/channel/,/uuid/"
                             "fireworks?pnsdk=unit-test-0.1&channel-group=[gr1,"
                             "gr2]&uuid=test_id&auth=mouth");
    incoming("HTTP/1.1 200\r\nContent-Length: "
             "141\r\n\r\n{\"status\":200,\"message\":\"OK\",\"service\":"
             "\"Presence\",\"payload\":{\"channels\":{\"alfa\":{key:value},"
             "\"bravo\":{},\"mike\":{},\"charlie\":{},\"foxtrot\":{}}}}",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_state_get(pbp, NULL, "[gr1,gr2]", "fireworks"), equals(PNR_OK));

    attest(pubnub_get(pbp),
           streqs("{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\","
                  "\"payload\":{\"channels\":{\"alfa\":{key:value},\"bravo\":{}"
                  ",\"mike\":{},\"charlie\":{},\"foxtrot\":{}}}}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));

    /* set state to 'channel(s)' and 'channel group(s)' with 'auth' set in
     * context and 'uuid' from parameter to the function */
    pubnub_set_auth(pbp, "cat");
    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/v2/presence/sub-key/subY/channel/[ch1,ch2]/uuid/"
                             "leslie-mann?pnsdk=unit-test-0.1&channel-group=["
                             "gr3,gr4]&uuid=test_id&auth=cat");
    incoming("HTTP/1.1 200\r\nContent-Length: "
             "153\r\n\r\n{\"status\":200,\"message\":\"OK\",\"service\":"
             "\"Presence\",\"payload\":{\"channels\":{\"ch1\":{jason_state},"
             "\"ch2\":{},\"this_one\":{},\"that_one\":{},\"and_another_one\":{}"
             "}}}",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_state_get(pbp, "[ch1,ch2]", "[gr3,gr4]", "leslie-mann"),
           equals(PNR_OK));

    attest(pubnub_get(pbp),
           streqs("{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\","
                  "\"payload\":{\"channels\":{\"ch1\":{jason_state},\"ch2\":{},"
                  "\"this_one\":{},\"that_one\":{},\"and_another_one\":{}}}}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));

    /* state_get for no 'channel' and no 'channel group'.
     */
    pubnub_set_auth(pbp, NULL); // with or without this line
    attest(pubnub_state_get(pbp, NULL, NULL, "leslie"), equals(PNR_INVALID_CHANNEL));
}

Ensure(single_context_pubnub, state_get_in_progress_interrupted_and_accomplished)
{
    pubnub_init(pbp, "publ-key", "sub-key");
    pubnub_set_user_id(pbp, "test_id");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/sub-key/channel/ch/uuid/"
                             "dragon?pnsdk=unit-test-0.1&uuid=test_id");
    /* incoming first message */
    incoming("HTTP/1.1 200\r\n", NULL);
    /* incoming empty string simulates conditions for PNR_IN_PROGRESS */
    incoming("", NULL);
    attest(pubnub_state_get(pbp, "ch", NULL, "dragon"), equals(PNR_STARTED));
    attest(pubnub_state_get(pbp, "night_channel", NULL, "knight"),
           equals(PNR_IN_PROGRESS));

    /* incoming next message */
    incoming("Content-Length: 82\r\n\r\n{", NULL);
    /* incoming the last message */
    incoming("\"status\":200,\"message\":\"OK\",\"service\":\"Presence\","
             "\"payload\":{free_state_of_jones}}",
             NULL);
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
}

Ensure(single_context_pubnub, state_get_bad_response)
{
    pubnub_init(pbp, "publkey", "Xsub");

    pubnub_set_user_id(pbp, "annoying");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/Xsub/channel/ch/uuid/"
                             "annoying?pnsdk=unit-test-0.1&uuid=annoying");
    incoming("HTTP/1.1 200\r\nContent-Length: 18\r\n\r\n[incorrect answer]", NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_state_get(pbp, "ch", NULL, NULL), equals(PNR_FORMAT_ERROR));
    attest(pbp->core.user_id_len, equals(8));
}

// TODO: move coreapi_ex to new module
Ensure(single_context_pubnub, set_state_ex_defaults)
{
    pubnub_init(pbp, "publhis", "subhis");

    /* with uuid from context */
    pubnub_set_user_id(pbp, "universal");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/subhis/channel/ch/uuid/"
                             "universal/data?pnsdk=unit-test-0.1&uuid=universal&state=%7B%7D");
    incoming("HTTP/1.1 200\r\nContent-Length: 67\r\n\r\n{\"status\": "
             "200,\"message\":\"OK\", \"service\": \"Presence\", "
             "\"payload\":{}}",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));

    attest(pubnub_set_state_ex(pbp, "ch", "{}", pubnub_set_state_defopts()), equals(PNR_OK));

    attest(pubnub_get(pbp), streqs("{\"status\": 200,\"message\":\"OK\", \"service\": \"Presence\", \"payload\":{}}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
} 

// TODO: move coreapi_ex to new module
Ensure(single_context_pubnub, state_get_with_heartbeat)
{ 
    pubnub_init(pbp, "pub-key", "sub-key");
    pubnub_set_user_id(pbp, "test-id");
 
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/sub-key/channel/ch/"
                             "heartbeat?pnsdk=unit-test-0.1&uuid=test-id&state=%7B%22ch%22%3A%7B%7D%7D");
    incoming("HTTP/1.1 200\r\nContent-Length: 67\r\n\r\n{\"status\": "
             "200,\"message\":\"OK\", \"service\": \"Presence\", "
             "\"payload\":{}}",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    
    struct pubnub_set_state_options options = pubnub_set_state_defopts();
    options.heartbeat = true;
    attest(pubnub_set_state_ex(pbp, "ch", "{}", options), equals(PNR_OK));
 
    attest(pubnub_get(pbp), streqs("{\"status\": 200,\"message\":\"OK\", \"service\": \"Presence\", \"payload\":{}}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
} 
 
/* HERE_NOW operation */

Ensure(single_context_pubnub, here_now_channel)
{
    pubnub_init(pbp, "publZ", "subZ");
    pubnub_set_user_id(pbp, "test_id");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url(
        "/v2/presence/sub-key/subZ/channel/shade?pnsdk=unit-test-0.1&uuid=test_id");
    incoming("HTTP/1.1 200\r\nContent-Length: 98\r\n\r\n{\"status\": "
             "200,\"message\":\"OK\", \"service\": \"Presence\", "
             "\"uuids\":[jack,johnnie,chivas],\"occupancy\":3}",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_here_now(pbp, "shade", NULL), equals(PNR_OK));

    attest(pubnub_get(pbp), streqs("{\"status\": 200,\"message\":\"OK\", \"service\": \"Presence\", \"uuids\":[jack,johnnie,chivas],\"occupancy\":3}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
}

Ensure(single_context_pubnub, here_now_channel_with_auth)
{
    pubnub_init(pbp, "publZ", "subZ");
    pubnub_set_user_id(pbp, "test_id");

    pubnub_set_auth(pbp, "auth-key");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/subZ/channel/"
                             "channel?pnsdk=unit-test-0.1&uuid=test_id&auth=auth-key");
    incoming("HTTP/1.1 200\r\nContent-Length: "
             "102\r\n\r\n{\"status\":200,\"message\":\"OK\",\"service\":"
             "\"Presence\",\"uuids\":[daniel's,walker,regal,beam],"
             "\"occupancy\":4}}",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_here_now(pbp, "channel", NULL), equals(PNR_OK));

    attest(pubnub_get(pbp),
           streqs("{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\","
                  "\"uuids\":[daniel's,walker,regal,beam],\"occupancy\":4}}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
}

Ensure(single_context_pubnub, here_now_channelgroups)
{
    pubnub_init(pbp, "publZ", "subZ");
    pubnub_set_user_id(pbp, "test_id");

    /* set_auth */
    pubnub_set_auth(pbp, "mouse");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/subZ/channel/"
                             ",?pnsdk=unit-test-0.1&channel-group=[gr2,gr1]&"
                             "uuid=test_id&auth=mouse");
    incoming("HTTP/1.1 200\r\nContent-Length: "
             "233\r\n\r\n{\"status\":200,\"message\":\"OK\",\"service\":"
             "\"Presence\",\"payload\":{channels:{\"ch1\":{\"uuids\":[uuid1,"
             "uuid2],\"occupancy\":2},\"ch2\":{\"uuids\":[uuid3],\"occupancy\":"
             "1},\"ch3\":{etc.},\"ch4\":{},\"ch5\":{}},\"total_channels\":5,"
             "\"total_occupancy\":something}}",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_here_now(pbp, NULL, "[gr2,gr1]"), equals(PNR_STARTED));
    attest(pbnc_fsm(pbp), equals(0));
    attest(pbp->core.last_result, equals(PNR_OK));

    attest(pubnub_get(pbp),
           streqs("{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\","
                  "\"payload\":{channels:{\"ch1\":{\"uuids\":[uuid1,uuid2],"
                  "\"occupancy\":2},\"ch2\":{\"uuids\":[uuid3],\"occupancy\":1}"
                  ",\"ch3\":{etc.},\"ch4\":{},\"ch5\":{}},\"total_channels\":5,"
                  "\"total_occupancy\":something}}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
}


Ensure(single_context_pubnub, here_now_channel_and_channelgroups)
{
    pubnub_init(pbp, "publZ", "subZ");

    /* here_now on 'channel(s)' and 'channel group(s)' with 'auth' and 'uuid' */
    pubnub_set_auth(pbp, "globe");
    pubnub_set_user_id(pbp, "12345");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/subZ/channel/"
                             "[ch1,ch2]?pnsdk=unit-test-0.1&channel-group=[gr3,"
                             "gr4]&uuid=12345&auth=globe");
    incoming("HTTP/1.1 200\r\nContent-Length: "
             "290\r\n\r\n{\"status\":200,\"message\":\"OK\",\"service\":"
             "\"Presence\",\"payload\":{channels:{\"ch1\":{\"uuids\":[uuid1,"
             "uuid2,uuid3],\"occupancy\":3},\"ch2\":{\"uuids\":[uuid3,uuid4],"
             "\"occupancy\":2},\"ch3\":{\"uuids\":[uuid7],\"occupancy\":1},"
             "\"ch4\":{\"uuids\":[],\"occupancy\":0},\"ch5\":{etc.}},\"total_"
             "channels\":5,\"total_occupancy\":8}}",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_here_now(pbp, "[ch1,ch2]", "[gr3,gr4]"), equals(PNR_STARTED));
    attest(pbnc_fsm(pbp), equals(0));
    attest(pbp->core.last_result, equals(PNR_OK));
    attest(pbp->core.user_id_len, equals(5));

    attest(pubnub_get(pbp),
           streqs("{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\","
                  "\"payload\":{channels:{\"ch1\":{\"uuids\":[uuid1,uuid2,"
                  "uuid3],\"occupancy\":3},\"ch2\":{\"uuids\":[uuid3,uuid4],"
                  "\"occupancy\":2},\"ch3\":{\"uuids\":[uuid7],\"occupancy\":1}"
                  ",\"ch4\":{\"uuids\":[],\"occupancy\":0},\"ch5\":{etc.}},"
                  "\"total_channels\":5,\"total_occupancy\":8}}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
}

/*
Ensure(single_context_pubnub, here_now_no_channel_and_no_channelgroups) {
    pubnub_init(pbp, "publZ", "subZ");
    attest(pubnub_here_now(pbp, NULL, NULL), equals(PNR_INVALID_CHANNEL));
}
*/

/* processing of chunked response is common for all operations */
Ensure(single_context_pubnub, here_now_channel_and_channelgroups_chunked)
{
    pubnub_init(pbp, "publZ", "subZ");

    /* here_now on 'channel(s)' and 'channel group(s)' with 'auth' and 'uuid' */
    pubnub_set_auth(pbp, "globe");
    pubnub_set_user_id(pbp, "12345");
    expect_have_dns_for_pubnub_origin();

    /* Don't forget that chunk lengths should be in hexadecimal representation
     */
    expect_outgoing_with_url("/v2/presence/sub-key/subZ/channel/"
                             "[ch1,ch2]?pnsdk=unit-test-0.1&channel-group=[gr3,"
                             "gr4]&uuid=12345&auth=globe");
    incoming("HTTP/1.1 200\r\nTransfer-Encoding: chunked\r\n\r\n "
             "118\r\n{\"status\":200,\"message\":\"OK\",\"service\":"
             "\"Presence\",\"payload\":{channels:{\"ch1\":{\"uuids\":[uuid1,"
             "uuid2,uuid3],\"occupancy\":3},\"ch2\":{\"uuids\":[uuid3,uuid4],"
             "\"occupancy\":2},\"ch3\":{\"uuids\":[uuid7],\"occupancy\":1},"
             "\"ch4\":{\"uuids\":[],\"occupancy\":0},\"ch5\":{etc.}},\"total_"
             "channels\":5,\"total_occu\r\na\r\npancy\":8}}\r\n0\r\n",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_here_now(pbp, "[ch1,ch2]", "[gr3,gr4]"), equals(PNR_STARTED));
    attest(pbnc_fsm(pbp), equals(0));
    attest(pbp->core.last_result, equals(PNR_OK));
    attest(pbp->core.user_id_len, equals(5));

    attest(pubnub_get(pbp),
           streqs("{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\","
                  "\"payload\":{channels:{\"ch1\":{\"uuids\":[uuid1,uuid2,"
                  "uuid3],\"occupancy\":3},\"ch2\":{\"uuids\":[uuid3,uuid4],"
                  "\"occupancy\":2},\"ch3\":{\"uuids\":[uuid7],\"occupancy\":1}"
                  ",\"ch4\":{\"uuids\":[],\"occupancy\":0},\"ch5\":{etc.}},"
                  "\"total_channels\":5,\"total_occupancy\":8}}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
}


Ensure(single_context_pubnub, here_now_in_progress_interrupted_and_accomplished)
{
    pubnub_init(pbp, "publ-one", "sub-one");

    /* here_now on 'channel(s)' and 'channel group(s)' with 'auth' and 'uuid' */
    pubnub_set_auth(pbp, "lion");
    pubnub_set_user_id(pbp, "cub");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/sub-one/channel/"
                             "[ch5,ch7]?pnsdk=unit-test-0.1&channel-group=[gr1,"
                             "gr2]&uuid=cub&auth=lion");
    incoming("HTTP/1.1 200\r\nTransfer-Encoding: "
             "chunked\r\n\r\n122\r\n{\"status\":200,\"mes",
             NULL);

    attest(pubnub_here_now(pbp, "[ch5,ch7]", "[gr1,gr2]"), equals(PNR_STARTED));
    attest(pubnub_here_now(pbp, "ch", NULL), equals(PNR_IN_PROGRESS));
    attest(pbp->core.user_id_len, equals(3));

    /* finish chunked */
    incoming("sage\":\"OK\",\"service\":\"Presence\",\"payload\":{channels:{"
             "\"ch1\":{\"uuids\":[uuid1,uuid2,uuid3],\"occupancy\":3},\"ch2\":{"
             "\"uuids\":[uuid3,uuid4],\"occupancy\":2},\"ch3\":{\"uuids\":["
             "uuid7],\"occupancy\":1},\"ch4\":{\"uuids\":[],\"occupancy\":0},"
             "\"ch5\":{etc.}},\"total_channels\":5,\"total_occupancy\":8}}"
             "\r\n0\r\n",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pbnc_fsm(pbp), equals(0));
    attest(pbp->core.last_result, equals(PNR_OK));

    attest(pubnub_get(pbp),
           streqs("{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\","
                  "\"payload\":{channels:{\"ch1\":{\"uuids\":[uuid1,uuid2,"
                  "uuid3],\"occupancy\":3},\"ch2\":{\"uuids\":[uuid3,uuid4],"
                  "\"occupancy\":2},\"ch3\":{\"uuids\":[uuid7],\"occupancy\":1}"
                  ",\"ch4\":{\"uuids\":[],\"occupancy\":0},\"ch5\":{etc.}},"
                  "\"total_channels\":5,\"total_occupancy\":8}}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
}

/* GLOBAL_HERE_NOW operation */

Ensure(single_context_pubnub, global_here_now)
{
    pubnub_init(pbp, "publ-white", "sub-white");
    pubnub_set_user_id(pbp, "test_id");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url(
        "/v2/presence/sub-key/sub-white?pnsdk=unit-test-0.1&uuid=test_id");
    incoming("HTTP/1.1 200\r\nContent-Length: "
             "334\r\n\r\n{\"status\":200,\"message\":\"OK\",\"service\":"
             "\"Presence\",\"payload\":{channels:{\"ch1\":{\"uuids\":[uuid1,"
             "uuid2,uuid3],\"occupancy\":3},\"ch2\":{\"uuids\":[uuid3,uuid4],"
             "\"occupancy\":2},\"ch3\":{\"uuids\":[uuid7],\"occupancy\":1},"
             "\"ch4\":{\"uuids\":[],\"occupancy\":0},\"ch5\":{\"uuids\":[prle, "
             "tihi, mrki, paja], "
             "\"occupancy\":4}},\"total_channels\":5,\"total_occupancy\":12}}",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_global_here_now(pbp), equals(PNR_STARTED));
    attest(pubnub_global_here_now(pbp), equals(PNR_IN_PROGRESS));

    attest(pbnc_fsm(pbp), equals(0));
    attest(pbp->core.last_result, equals(PNR_OK));

    attest(pubnub_get(pbp),
           streqs("{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\","
                  "\"payload\":{channels:{\"ch1\":{\"uuids\":[uuid1,uuid2,"
                  "uuid3],\"occupancy\":3},\"ch2\":{\"uuids\":[uuid3,uuid4],"
                  "\"occupancy\":2},\"ch3\":{\"uuids\":[uuid7],\"occupancy\":1}"
                  ",\"ch4\":{\"uuids\":[],\"occupancy\":0},\"ch5\":{\"uuids\":["
                  "prle, tihi, mrki, paja], "
                  "\"occupancy\":4}},\"total_channels\":5,\"total_occupancy\":"
                  "12}}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
}

Ensure(single_context_pubnub, global_here_now_chunked)
{
    pubnub_init(pbp, "publ-beo", "sub-beo");

    /* With uuid & auth */
    pubnub_set_auth(pbp, "beograd");
    pubnub_set_user_id(pbp, "pobednik");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/"
                             "sub-beo?pnsdk=unit-test-0.1&uuid=pobednik&auth=beograd");
    /* Chunk lengths are in hexadecimal representation */
    incoming("HTTP/1.1 200\r\nTransfer-Encoding: "
             "chunked\r\n\r\n1\r\n{\r\n12c\r\n\"status\":200,\"message\":"
             "\"OK\",\"service\":\"Presence\",\"payload\":{channels:{\"ch1\":{"
             "\"uuids\":[uuid1,uuid2,uuid3],\"occupancy\":3},\"ch2\":{"
             "\"uuids\":[uuid3,uuid4],\"occupancy\":2},\"ch3\":{\"uuids\":["
             "uuid7],\"occupancy\":1},\"ch4\":{\"uuids\":[],\"occupancy\":0},"
             "\"ch5\":{\"uuids\":[prle, tihi, mrki, paja], "
             "\"occupancy\":4}},\"total_c\r\n21\r\nhannels\":5,\"total_"
             "occupancy\":12}}\r\n0\r\n",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_global_here_now(pbp), equals(PNR_STARTED));
    attest(pbp->core.user_id_len, equals(8));

    attest(pbnc_fsm(pbp), equals(0));
    attest(pbp->core.last_result, equals(PNR_OK));

    attest(pubnub_get(pbp),
           streqs("{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\","
                  "\"payload\":{channels:{\"ch1\":{\"uuids\":[uuid1,uuid2,"
                  "uuid3],\"occupancy\":3},\"ch2\":{\"uuids\":[uuid3,uuid4],"
                  "\"occupancy\":2},\"ch3\":{\"uuids\":[uuid7],\"occupancy\":1}"
                  ",\"ch4\":{\"uuids\":[],\"occupancy\":0},\"ch5\":{\"uuids\":["
                  "prle, tihi, mrki, paja], "
                  "\"occupancy\":4}},\"total_channels\":5,\"total_occupancy\":"
                  "12}}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
}

Ensure(single_context_pubnub, global_here_now_in_progress_interrupted_and_acomplished)
{
    pubnub_init(pbp, "publ-my", "sub-my");
    pubnub_set_user_id(pbp, "test_id");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/sub-my?pnsdk=unit-test-0.1&uuid=test_id");
    incoming(
        "HTTP/1.1 200\r\nContent-Length: 334\r\n\r\n{\"status\":200,\"mess", NULL);
    /* Keeps fsm in progress */
    incoming("", NULL);
    attest(pubnub_global_here_now(pbp), equals(PNR_STARTED));
    attest(pubnub_global_here_now(pbp), equals(PNR_IN_PROGRESS));

    incoming("age\":\"OK\",\"service\":\"Presence\",\"payload\":{channels:{"
             "\"ch1\":{\"uuids\":[uuid1,uuid2,uuid3],\"occupancy\":3},\"ch2\":{"
             "\"uuids\":[uuid3,uuid4],\"occupancy\":2},\"ch3\":{\"uuids\":["
             "uuid7],\"occupancy\":1},\"ch4\":{\"uuids\":[],\"occupancy\":0},"
             "\"ch5\":{\"uuids\":[prle, tihi, mrki, paja], "
             "\"occupancy\":4}},\"total_channels\":5,\"total_occupancy\":12}}",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));

    /* roll the rock */
    attest(pbnc_fsm(pbp), equals(0));
    attest(pbnc_fsm(pbp), equals(0));
    attest(pbp->core.last_result, equals(PNR_OK));

    attest(pubnub_get(pbp),
           streqs("{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\","
                  "\"payload\":{channels:{\"ch1\":{\"uuids\":[uuid1,uuid2,"
                  "uuid3],\"occupancy\":3},\"ch2\":{\"uuids\":[uuid3,uuid4],"
                  "\"occupancy\":2},\"ch3\":{\"uuids\":[uuid7],\"occupancy\":1}"
                  ",\"ch4\":{\"uuids\":[],\"occupancy\":0},\"ch5\":{\"uuids\":["
                  "prle, tihi, mrki, paja], "
                  "\"occupancy\":4}},\"total_channels\":5,\"total_occupancy\":"
                  "12}}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
}

/* WHERE_NOW operation */

Ensure(single_context_pubnub, where_now)
{
    pubnub_init(pbp, "publ-where", "sub-where");
    pubnub_set_user_id(pbp, "test_id");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url(
        "/v2/presence/sub-key/sub-where/uuid/shane(1953)?pnsdk=unit-test-0.1&uuid=test_id");
    incoming("HTTP/1.1 200\r\nContent-Length: "
             "89\r\n\r\n{\"status\":200,\"message\":\"OK\",\"service\":"
             "\"Presence\",\"Payload\":{\"channels\":[tcm,retro,mgm]}}",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_where_now(pbp, "shane(1953)"), equals(PNR_OK));

    attest(pubnub_get(pbp), streqs("{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\",\"Payload\":{\"channels\":[tcm,retro,mgm]}}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
}

Ensure(single_context_pubnub, where_now_set_uuid)
{
    pubnub_init(pbp, "publ-her", "sub-her");

    pubnub_set_user_id(pbp, "50fb7a0b-1688-45b9-9f27-ea83308464d8-ab3817df");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url(
        "/v2/presence/sub-key/sub-her/uuid/50fb7a0b-1688-45b9-9f27-ea83308464d8"
        "-ab3817df?pnsdk=unit-test-0.1&uuid=50fb7a0b-1688-45b9-9f27-ea83308464d8-ab3817df");
    incoming("HTTP/1.1 200\r\nContent-Length: "
             "100\r\n\r\n{\"status\":200,\"message\":\"OK\",\"service\":"
             "\"Presence\",\"Payload\":{\"channels\":[discovery,nat_geo,nature]"
             "}}",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    /* Recognizes the uuid set from the context */
    attest(pubnub_where_now(pbp, NULL), equals(PNR_OK));
    attest(pbp->core.user_id_len, equals(45));

    attest(pubnub_get(pbp),
           streqs("{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\","
                  "\"Payload\":{\"channels\":[discovery,nat_geo,nature]}}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
}

Ensure(single_context_pubnub, where_now_set_auth)
{
    pubnub_init(pbp, "publ-sea", "sub-sea");

    pubnub_set_user_id(pbp, "fish");
    pubnub_set_auth(pbp, "big");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url(
        "/v2/presence/sub-key/sub-sea/uuid/whale?pnsdk=unit-test-0.1&uuid=fish&auth=big");
    incoming("HTTP/1.1 200\r\nContent-Length: "
             "107\r\n\r\n{\"status\":200,\"message\":\"OK\",\"service\":"
             "\"Presence\",\"Payload\":{\"channels\":[first,second,third,"
             "fourth,fifth]}}",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    /* Chooses the uuid from the call not the one set in context */
    attest(pubnub_where_now(pbp, "whale"), equals(PNR_OK));
    attest(pbp->core.user_id_len, equals(4));

    attest(pubnub_get(pbp),
           streqs("{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\","
                  "\"Payload\":{\"channels\":[first,second,third,fourth,fifth]}"
                  "}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
}

Ensure(single_context_pubnub, where_now_in_progress_interrupted_and_accomplished)
{
    pubnub_init(pbp, "publ-good", "sub-good");

    pubnub_set_user_id(pbp, "man_with_no_name");
    pubnub_set_auth(pbp, "west");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url(
        "/v2/presence/sub-key/sub-good/uuid/bad?pnsdk=unit-test-0.1&uuid=man_with_no_name&auth=west");
    incoming("HTTP/1.1 200\r\nContent-Length: "
             "140\r\n\r\n{\"status\":200,\"message\":\"OK\",\"ser",
             NULL);
    /* server won't close connection after sending this much */
    incoming("", NULL);
    attest(pubnub_where_now(pbp, "bad"), equals(PNR_STARTED));
    attest(pubnub_where_now(pbp, "ugly"), equals(PNR_IN_PROGRESS));
    attest(pbp->core.user_id_len, equals(16));

    /* client reciving rest of the message */
    incoming("vice\":\"Presence\",\"Payload\":{\"channels\":[western,here,"
             "there,everywhere,fifth_channel,small_town,boot_hill]}}",
             NULL);

    /* Push until finished */
    attest(pbnc_fsm(pbp), equals(0));
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pbnc_fsm(pbp), equals(0));
    attest(pbp->core.last_result, equals(PNR_OK));

    attest(pubnub_get(pbp),
           streqs("{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\","
                  "\"Payload\":{\"channels\":[western,here,there,everywhere,"
                  "fifth_channel,small_town,boot_hill]}}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
}


/* HEARTBEAT operation */

Ensure(single_context_pubnub, heartbeat_channel)
{
    pubnub_init(pbp, "publ-beat", "sub-beat");
    pubnub_set_user_id(pbp, "test_id");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/sub-beat/channel/panama/"
                             "heartbeat?pnsdk=unit-test-0.1&uuid=test_id");
    incoming("HTTP/1.1 200\r\nContent-Length: "
             "50\r\n\r\n{\"status\":200,\"message\":\"OK\",\"service\":"
             "\"Presence\"}",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_heartbeat(pbp, "panama", NULL), equals(PNR_OK));

    attest(
        pubnub_get(pbp),
        streqs("{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\"}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
}

Ensure(single_context_pubnub, heartbeat_channelgroups)
{
    pubnub_init(pbp, "publ-beat", "sub-beat");
    pubnub_set_user_id(pbp, "test_id");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/sub-beat/channel/,/"
                             "heartbeat?pnsdk=unit-test-0.1&channel-group=["
                             "deep,shallow]&uuid=test_id");
    incoming("HTTP/1.1 200\r\nContent-Length: "
             "50\r\n\r\n{\"status\":200,\"message\":\"OK\",\"service\":"
             "\"Presence\"}",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_heartbeat(pbp, NULL, "[deep,shallow]"), equals(PNR_OK));

    attest(
        pubnub_get(pbp),
        streqs("{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\"}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
}

Ensure(single_context_pubnub, heartbeat_channel_and_channelgroups)
{
    pubnub_init(pbp, "publ-ocean", "sub-ocean");

    pubnub_set_auth(pbp, "sailing");
    pubnub_set_user_id(pbp, "capetan");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/sub-ocean/channel/"
                             "young_and_salty/"
                             "heartbeat?pnsdk=unit-test-0.1&channel-group=["
                             "deep,shallow]&uuid=capetan&auth=sailing");
    incoming("HTTP/1.1 200\r\nContent-Length: "
             "50\r\n\r\n{\"status\":200,\"message\":\"OK\",\"service\":"
             "\"Presence\"}",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_heartbeat(pbp, "young_and_salty", "[deep,shallow]"),
           equals(PNR_OK));
    attest(pbp->core.user_id_len, equals(7));

    attest(
        pubnub_get(pbp),
        streqs("{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\"}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
}


Ensure(single_context_pubnub, heartbeat_no_channel_and_no_channelgroups)
{
    pubnub_init(pbp, "publ-", "sub-");
    pubnub_set_user_id(pbp, "test_id");
    attest(pubnub_heartbeat(pbp, NULL, NULL), equals(PNR_INVALID_CHANNEL));
}

Ensure(single_context_pubnub,
       heartbeat_channel_and_channelgroups_interrupted_and_accomplished)
{
    pubnub_init(pbp, "publ-game", "sub-game");

    pubnub_set_auth(pbp, "white");
    pubnub_set_user_id(pbp, "player");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/sub-game/channel/moves/"
                             "heartbeat?pnsdk=unit-test-0.1&channel-group=["
                             "fast,slow]&uuid=player&auth=white");
    incoming("HTTP/1.1 200\r\nContent-Length:", NULL);
    incoming("", NULL);
    attest(pubnub_heartbeat(pbp, "moves", "[fast,slow]"), equals(PNR_STARTED));
    attest(pubnub_heartbeat(pbp, "punches", "[fast,slow]"), equals(PNR_IN_PROGRESS));
    attest(pbp->core.user_id_len, equals(6));

    incoming(" 50\r\n\r\n{\"status\":200,\"message\":\"OK\",\"service\":"
             "\"Presence\"}",
             NULL);
    /* Push until finished */
    attest(pbnc_fsm(pbp), equals(0));
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pbnc_fsm(pbp), equals(0));
    attest(pbp->core.last_result, equals(PNR_OK));

    attest(
        pubnub_get(pbp),
        streqs("{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\"}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
}


/* ADD_CHANNEL_TO_GROUP operation */

Ensure(single_context_pubnub, add_channel_to_group)
{
    pubnub_init(pbp, "publ-kc", "sub-kc");
    pubnub_set_user_id(pbp, "test_id");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v1/channel-registration/sub-key/sub-kc/"
                             "channel-group/"
                             "ch_group?pnsdk=unit-test-0.1&uuid=test_id&add=ch_one");
    incoming("HTTP/1.1 200\r\nContent-Length: "
             "72\r\n\r\n{\"service\":\"channel-registry\",\"status\":200,"
             "\"error\":false,\"message\":\"OK\"}",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_add_channel_to_group(pbp, "ch_one", "ch_group"), equals(PNR_OK));

    attest(pubnub_get_channel(pbp), streqs("{\"service\":\"channel-registry\",\"status\":200,\"error\":false,\"message\":\"OK\"}"));
    attest(pubnub_get_channel(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
}

Ensure(single_context_pubnub, add_channel_to_group_interrupted_and_accomplished)
{
    pubnub_init(pbp, "publ-kc", "sub-kc");
    pubnub_set_user_id(pbp, "test_id");
    /* With auth */
    pubnub_set_auth(pbp, "rice_chocolate");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url(
        "/v1/channel-registration/sub-key/sub-kc/channel-group/"
        "ch_group?pnsdk=unit-test-0.1&uuid=test_id&add=ch_one&auth=rice_chocolate");
    incoming("HTTP/1.1 200\r\nContent-Length: 72\r\n\r\n{\"service\"", NULL);
    /* won't close connection */
    incoming("", NULL);

    attest(pubnub_add_channel_to_group(pbp, "ch_one", "ch_group"),
           equals(PNR_STARTED));
    attest(pubnub_add_channel_to_group(pbp, "ch", "ch_group"),
           equals(PNR_IN_PROGRESS));

    incoming(":\"channel-registry\",\"status\":200,\"error\":false,\"message\":"
             "\"OK\"}",
             NULL);
    /* keep 'turning' it until finished */
    attest(pbnc_fsm(pbp), equals(0));
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pbnc_fsm(pbp), equals(0));
    attest(pbp->core.last_result, equals(PNR_OK));

    attest(pubnub_get_channel(pbp), streqs("{\"service\":\"channel-registry\",\"status\":200,\"error\":false,\"message\":\"OK\"}"));
    attest(pubnub_get_channel(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
}

/* REMOVE_CHANNEL_FROM_GROUP operation */

Ensure(single_context_pubnub, remove_channel_from_group)
{
    pubnub_init(pbp, "publ-kum_Ruzvelt", "sub-kum_Ruzvelt");
    pubnub_set_user_id(pbp, "test_id");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v1/channel-registration/sub-key/sub-kum_Ruzvelt/"
                             "channel-group/"
                             "ch_group?pnsdk=unit-test-0.1&uuid=test_id&remove=ch_one");
    incoming("HTTP/1.1 200\r\nContent-Length: "
             "72\r\n\r\n{\"service\":\"channel-registry\",\"status\":200,"
             "\"error\":false,\"message\":\"OK\"}",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_remove_channel_from_group(pbp, "ch_one", "ch_group"),
           equals(PNR_OK));

    attest(pubnub_get_channel(pbp), streqs("{\"service\":\"channel-registry\",\"status\":200,\"error\":false,\"message\":\"OK\"}"));
    attest(pubnub_get_channel(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
}

Ensure(single_context_pubnub, remove_channel_from_group_interrupted_and_accomplished)
{
    pubnub_init(pbp, "publ-Teheran", "sub-Teheran");
    pubnub_set_user_id(pbp, "test_id");
    /* With auth */
    pubnub_set_auth(pbp, "dates");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url(
        "/v1/channel-registration/sub-key/sub-Teheran/channel-group/"
        "ch_group?pnsdk=unit-test-0.1&uuid=test_id&remove=ch_one&auth=dates");
    incoming("HTTP/1.1 200\r\nContent-", NULL);
    /* won't close connection */
    incoming("", NULL);

    attest(pubnub_remove_channel_from_group(pbp, "ch_one", "ch_group"),
           equals(PNR_STARTED));
    attest(pubnub_remove_channel_from_group(pbp, "ch", "ch_group"),
           equals(PNR_IN_PROGRESS));

    incoming("Length: "
             "72\r\n\r\n{\"service\":\"channel-registry\",\"status\":200,"
             "\"error\":false,\"message\":\"OK\"}",
             NULL);
    /* keep 'turning' it until finished */
    attest(pbnc_fsm(pbp), equals(0));
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pbnc_fsm(pbp), equals(0));
    attest(pbp->core.last_result, equals(PNR_OK));

    attest(pubnub_get_channel(pbp), streqs("{\"service\":\"channel-registry\",\"status\":200,\"error\":false,\"message\":\"OK\"}"));
    attest(pubnub_get_channel(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
}

/* REMOVE_CHANNEL_GROUP operation */

Ensure(single_context_pubnub, remove_channel_group)
{
    pubnub_init(pbp, "publ-bell", "sub-bell");
    pubnub_set_user_id(pbp, "test_id");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v1/channel-registration/sub-key/sub-bell/"
                             "channel-group/group_name/"
                             "remove?pnsdk=unit-test-0.1&uuid=test_id");
    incoming("HTTP/1.1 200\r\nContent-Length: 82\r\n\r\n{\"service\": "
             "\"channel-registry\" , \"status\"  : 200 ,\"error\" "
             ":false,\"message\":  \"OK\"}",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_remove_channel_group(pbp, "group_name"), equals(PNR_OK));

    attest(pubnub_get_channel(pbp), streqs("{\"service\": \"channel-registry\" , \"status\"  : 200 ,\"error\" :false,\"message\":  \"OK\"}"));
    attest(pubnub_get_channel(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
}

Ensure(single_context_pubnub, remove_channel_group_interrupted_and_accomplished)
{
    pubnub_init(pbp, "publ-blue", "sub-blue");
    pubnub_set_user_id(pbp, "test_id");
    /* With auth */
    pubnub_set_auth(pbp, "sky");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v1/channel-registration/sub-key/sub-blue/"
                             "channel-group/group/"
                             "remove?pnsdk=unit-test-0.1&uuid=test_id&auth=sky");
    incoming("HTTP/1.1 200\r\nContent-Len", NULL);
    /* won't close connection */
    incoming("", NULL);

    attest(pubnub_remove_channel_group(pbp, "group"), equals(PNR_STARTED));
    attest(pubnub_remove_channel_group(pbp, "another_group"),
           equals(PNR_IN_PROGRESS));

    incoming("gth: 79\r\n\r\n{ \"service\":\"channel-registry\" ,\"status\" : "
             "200,\"error\" : false ,\"message\":\"OK\"}",
             NULL);
    /* keep 'turning' it until finished */
    attest(pbnc_fsm(pbp), equals(0));
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pbnc_fsm(pbp), equals(0));
    attest(pbp->core.last_result, equals(PNR_OK));

    attest(pubnub_get_channel(pbp), streqs("{ \"service\":\"channel-registry\" ,\"status\" : 200,\"error\" : false ,\"message\":\"OK\"}"));
    attest(pubnub_get_channel(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
}

/* LIST_CHANNEL_GROUP operation */

Ensure(single_context_pubnub, list_channel_group)
{
    pubnub_init(pbp, "publ-science", "sub-science");
    pubnub_set_user_id(pbp, "test_id");
    /* With auth */
    pubnub_set_auth(pbp, "research");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v1/channel-registration/sub-key/sub-science/"
                             "channel-group/"
                             "info?pnsdk=unit-test-0.1&uuid=test_id&auth=research");
    incoming("HTTP/1.1 200\r\nContent-Length: 154\r\n\r\n{\"service\": "
             "\"channel-registry\" , \"status\": 200 ,\"error\" "
             ":false,\"payload\":{\"group\":\"info\",\"channels\":{\"weather\","
             " \"polution\",\"resources\",\"consumtion\",...}}}",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_list_channel_group(pbp, "info"), equals(PNR_OK));

    attest(pubnub_get_channel(pbp),
           streqs("{\"service\": \"channel-registry\" , \"status\": 200 "
                  ",\"error\" "
                  ":false,\"payload\":{\"group\":\"info\",\"channels\":{"
                  "\"weather\", "
                  "\"polution\",\"resources\",\"consumtion\",...}}}"));
    attest(pubnub_get_channel(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
}

Ensure(single_context_pubnub, list_channel_group_interrupted_and_accomplished)
{
    pubnub_init(pbp, "publ-air-traffic", "sub-air-traffic");
    pubnub_set_user_id(pbp, "test_id");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v1/channel-registration/sub-key/sub-air-traffic/"
                             "channel-group/airborne?pnsdk=unit-test-0.1&uuid=test_id");
    incoming("HTTP/1.1 200\r\nContent-Len", NULL);
    /* won't close connection */
    incoming("", NULL);

    attest(pubnub_list_channel_group(pbp, "airborne"), equals(PNR_STARTED));
    attest(pubnub_list_channel_group(pbp, "landed"), equals(PNR_IN_PROGRESS));

    incoming("gth: 128\r\n\r\n{ \"service\":\"channel-registry\" ,\"status\" : "
             "200,\"error\" : false , "
             "\"payload\":{\"group\":\"airborne\",\"channels\":{\"pw45\", "
             "\"xg37\",...}}}",
             NULL);
    /* keep 'turning' it until finished */
    attest(pbnc_fsm(pbp), equals(0));
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pbnc_fsm(pbp), equals(0));
    attest(pbp->core.last_result, equals(PNR_OK));

    attest(pubnub_get_channel(pbp),
           streqs("{ \"service\":\"channel-registry\" ,\"status\" : "
                  "200,\"error\" : false , "
                  "\"payload\":{\"group\":\"airborne\",\"channels\":{\"pw45\", "
                  "\"xg37\",...}}}"));
    attest(pubnub_get_channel(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
}

/* SUBSCRIBE operation */

Ensure(single_context_pubnub, subscribe)
{
    pubnub_init(pbp, "publ-magazin", "sub-magazin");
    pubnub_set_user_id(pbp, "test_id");

    attest(pubnub_auth_get(pbp), streqs(NULL));
    attest(pubnub_last_time_token(pbp), streqs("0"));
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url(
        "/subscribe/sub-magazin/health/0/0?pnsdk=unit-test-0.1&uuid=test_id");
    incoming("HTTP/1.1 200\r\nContent-Length: "
             "26\r\n\r\n[[],\"1516014978925123457\"]",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(pbp, "health", NULL), equals(PNR_OK));

    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_last_time_token(pbp), streqs("1516014978925123457"));
    /* Not publish operation */
    attest(pubnub_last_publish_result(pbp), streqs(""));

    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/subscribe/sub-magazin/health/0/"
                             "1516014978925123457?pnsdk=unit-test-0.1&uuid=test_id");
    incoming("HTTP/1.1 200\r\nContent-Length: "
             "56\r\n\r\n[[pomegranate_juice,papaya,mango],"
             "\"1516714978925123457\"]",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(pbp, "health", NULL), equals(PNR_OK));
    attest(pubnub_last_time_token(pbp), streqs("1516714978925123457"));

    attest(pubnub_get(pbp), streqs("pomegranate_juice"));
    attest(pubnub_get(pbp), streqs("papaya"));
    attest(pubnub_get(pbp), streqs("mango"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), streqs(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
}

Ensure(single_context_pubnub, subscribe_channel_groups)
{
    pubnub_init(pbp, "publ-bulletin", "sub-bulletin");
    pubnub_set_user_id(pbp, "test_id");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/subscribe/sub-bulletin/,/0/"
                             "0?pnsdk=unit-test-0.1&channel-group=updates&uuid=test_id");
    incoming(
        "HTTP/1.1 200\r\nContent-Length: 25\r\n\r\n[[],\"251614978925123457\"]",
        NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(pbp, NULL, "updates"), equals(PNR_OK));

    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), streqs(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_last_time_token(pbp), streqs("251614978925123457"));

    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/subscribe/sub-bulletin/,/0/"
                             "251614978925123457?pnsdk=unit-test-0.1&channel-"
                             "group=updates&uuid=test_id");
    incoming("HTTP/1.1 200\r\nContent-Length: "
             "110\r\n\r\n[[skype,web_brouser,text_editor],"
             "\"251624978925123457\",\"updates,updates,updates\",\"messengers,"
             "brousers,editors\"]",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(pbp, NULL, "updates"), equals(PNR_OK));
    attest(pubnub_last_time_token(pbp), streqs("251624978925123457"));

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
    expect_outgoing_with_url("/subscribe/sub-bulletin/,/0/"
                             "251624978925123457?pnsdk=unit-test-0.1&channel-"
                             "group=updates&uuid=test_id");
    incoming("HTTP/1.1 200\r\nContent-Length: "
             "51\r\n\r\n[[virtualbox],\"251624978925123457\",\"updates\","
             "\"VMs\"]",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(pbp, NULL, "updates"), equals(PNR_OK));

    attest(pubnub_get(pbp), streqs("virtualbox"));
    attest(pubnub_get_channel(pbp), streqs("VMs"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), streqs(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
}

Ensure(single_context_pubnub, subscribe_channels_and_channel_groups)
{
    pubnub_init(pbp, "publ-key", "sub-Key");

    pubnub_set_user_id(pbp, "admin");
    pubnub_set_auth(pbp, "msgs");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/subscribe/sub-Key/[ch1,ch2]/0/"
                             "0?pnsdk=unit-test-0.1&channel-group=[chgr2,chgr3,"
                             "chgr4]&uuid=admin&auth=msgs");
    incoming("HTTP/1.1 200\r\nContent-Length: "
             "26\r\n\r\n[[],\"3516149789251234578\"]",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(pbp, "[ch1,ch2]", "[chgr2,chgr3,chgr4]"),
           equals(PNR_OK));
    attest(pbp->core.user_id_len, equals(5));

    attest(pubnub_get_channel(pbp), streqs(NULL));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));

    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/subscribe/sub-Key/[ch1,ch2]/0/"
                             "3516149789251234578?pnsdk=unit-test-0.1&channel-"
                             "group=[chgr2,chgr3,chgr4]&uuid=admin&auth=msgs");
    incoming("HTTP/1.1 200\r\nContent-Length: "
             "150\r\n\r\n[[msg1,msg2,{\"text\":\"Hello "
             "World!\"},msg4,msg5,{\"key\":\"val\\ue\"}],"
             "\"352624978925123458\",\"chgr4,chgr2,chgr3,chgr4,chgr7,chgr4\","
             "\"ch5,ch8,ch6,ch17,ch1,ch2\"]",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(pbp, "[ch1,ch2]", "[chgr2,chgr3,chgr4]"),
           equals(PNR_OK));

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
    expect_outgoing_with_url("/subscribe/sub-Key/ch17/0/"
                             "352624978925123458?pnsdk=unit-test-0.1&uuid="
                             "admin&auth=msgs");
    incoming("HTTP/1.1 200\r\nContent-Length: "
             "47\r\n\r\n[[message],\"352624979925123457\",\"chgr4\",\"ch17\"]",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(pbp, "ch17", NULL), equals(PNR_OK));
    attest(pubnub_subscribe(pbp, NULL, "chgr2"), equals(PNR_RX_BUFF_NOT_EMPTY));

    attest(pubnub_get(pbp), streqs("message"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), streqs("ch17"));
    attest(pubnub_get_channel(pbp), streqs(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_user_id_get(pbp), streqs("admin"));
    attest(pubnub_auth_get(pbp), streqs("msgs"));
}

Ensure(single_context_pubnub,
       subscribe_channel_groups_interrupted_and_accomplished_chunked)
{
    pubnub_init(pbp, "publ-measurements", "sub-measurements");

    pubnub_set_user_id(pbp, "technician");
    pubnub_set_auth(pbp, "weather-conditions");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/subscribe/sub-measurements/,/0/"
                             "0?pnsdk=unit-test-0.1&channel-group=[air-"
                             "temperature,humidity,wind-speed-and-direction,"
                             "pressure]&uuid=technician&auth=weather-"
                             "conditions");
    incoming("HTTP/1.1 200\r\nContent-Length: "
             "26\r\n\r\n[[],\"1516149789251234578\"]",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(
               pbp, NULL, "[air-temperature,humidity,wind-speed-and-direction,pressure]"),
           equals(PNR_OK));
    attest(pbp->core.user_id_len, equals(10));

    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), streqs(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));

    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/subscribe/sub-measurements/,/0/"
                             "1516149789251234578?pnsdk=unit-test-0.1&channel-"
                             "group=[air-temperature,humidity,wind-speed-and-"
                             "direction,pressure]&uuid=technician&auth=weather-"
                             "conditions");
    incoming("HTTP/1.1 200\r\nTransfer-Encoding: "
             "chunked\r\n\r\n9d\r\n[[{\"uuid1\":\"-2\"},{\"uuid2\":\"-5\"},{"
             "\"Kingston\":\"+22\"},{\"Manila\":\"+38\"},{\"bouy76\":\"+5\"},{"
             "\"Pr. Astrid "
             "Coast\":\"Unable_to_fetch_temperature\"}],"
             "\"1516149789251234583\"\r\n97\r\n,\"air-temperature,air-"
             "temperature,air-temperature,air-temperature,air-temperature,air-"
             "temperature\",\"ch-atmp,ch-atmp,ch-atmp,ch-atmp,ch-atmp,ch-"
             "atmp\"]\r\n0\r\n",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(
               pbp, NULL, "[air-temperature,humidity,wind-speed-and-direction,pressure]"),
           equals(PNR_STARTED));
    attest(pubnub_subscribe(pbp, "some_channels", "some_channel_goups"),
           equals(PNR_IN_PROGRESS));

    /* 'push' until finished */
    attest(pbnc_fsm(pbp), equals(0));
    attest(pbp->core.last_result, equals(PNR_OK));
    attest(pubnub_subscribe(pbp, NULL, "some_channel_goups"),
           equals(PNR_RX_BUFF_NOT_EMPTY));

    attest(pubnub_get(pbp), streqs("{\"uuid1\":\"-2\"}"));
    attest(pubnub_get_channel(pbp), streqs("ch-atmp"));
    attest(pubnub_get(pbp), streqs("{\"uuid2\":\"-5\"}"));
    attest(pubnub_get(pbp), streqs("{\"Kingston\":\"+22\"}"));
    attest(pubnub_get(pbp), streqs("{\"Manila\":\"+38\"}"));
    attest(pubnub_get(pbp), streqs("{\"bouy76\":\"+5\"}"));
    attest(pubnub_subscribe(pbp, NULL, "wind-speed-and-direction"),
           equals(PNR_RX_BUFF_NOT_EMPTY));
    attest(pubnub_get(pbp),
           streqs("{\"Pr. Astrid Coast\":\"Unable_to_fetch_temperature\"}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));

    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/subscribe/sub-measurements/,/0/"
                             "1516149789251234583?pnsdk=unit-test-0.1&channel-"
                             "group=[air-temperature,humidity,wind-speed-and-"
                             "direction,pressure]&uuid=technician&auth=weather-"
                             "conditions");
    incoming("HTTP/1.1 200\r\nTransfer-Encoding: "
             "chunked\r\n\r\n101\r\n[[{\"uuid1\":{\"dir\":w,\"speed\":\"2mph\","
             "\"blows\":\"3mph\"}},{\"uuid2\":{\"dir\":nw,\"speed\":\"5mph\","
             "\"blows\":\"7mph\"}},{\"Sebu, "
             "Philippines\":{\"dir\":nne,\"speed\":\"4mph\",\"blows\":\"4mph\"}"
             "},{\"Sri Jayawardenepura Kotte, Sri "
             "Lanka\":{\"dir\":ne,\"speed\":\"7mph\",\"blows\":\"7mph\"}}],"
             "\"151614\r\n93\r\n9789251234597\",\"wind-speed-and-direction,"
             "wind-speed-and-direction,wind-speed-and-direction,wind-speed-and-"
             "direction\",\"ch-ws1,ch-ws2,ch-ws3,ch-ws1\"]\r\n0\r\n",
             NULL);
    attest(pubnub_subscribe(
               pbp, NULL, "[air-temperature,humidity,wind-speed-and-direction,pressure]"),
           equals(PNR_STARTED));
    attest(pubnub_subscribe(pbp, NULL, "humidity"), equals(PNR_IN_PROGRESS));

    /* 'push' until finished */
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pbnc_fsm(pbp), equals(0));
    attest(pbp->core.last_result, equals(PNR_OK));

    attest(pubnub_get_channel(pbp), streqs("ch-ws1"));
    attest(
        pubnub_get(pbp),
        streqs(
            "{\"uuid1\":{\"dir\":w,\"speed\":\"2mph\",\"blows\":\"3mph\"}}"));
    attest(
        pubnub_get(pbp),
        streqs(
            "{\"uuid2\":{\"dir\":nw,\"speed\":\"5mph\",\"blows\":\"7mph\"}}"));
    attest(pubnub_get(pbp), streqs("{\"Sebu, Philippines\":{\"dir\":nne,\"speed\":\"4mph\",\"blows\":\"4mph\"}}"));
    attest(pubnub_get(pbp), streqs("{\"Sri Jayawardenepura Kotte, Sri Lanka\":{\"dir\":ne,\"speed\":\"7mph\",\"blows\":\"7mph\"}}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
}

Ensure(single_context_pubnub, subscribe_no_channel_and_no_channelgroups)
{
    pubnub_init(pbp, "publ-something", "sub-something");
    pubnub_set_user_id(pbp, "test_id");
    attest(pubnub_subscribe(pbp, NULL, NULL), equals(PNR_INVALID_CHANNEL));
}

Ensure(single_context_pubnub, subscribe_parse_response_format_error)
{
    pubnub_init(pbp, "publ-fe", "sub-fe");
    pubnub_set_user_id(pbp, "test_id");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/subscribe/sub-fe/[ch2]/0/"
                             "0?pnsdk=unit-test-0.1&channel-group=[chgr2,"
                             "chgr4]&uuid=test_id");
    incoming("HTTP/1.1 200\r\nContent-Length: "
             "26\r\n\r\n{[],\"3516149789251234578\"}",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(pbp, "[ch2]", "[chgr2,chgr4]"),
           equals(PNR_FORMAT_ERROR));
}

Ensure(single_context_pubnub, subscribe_reestablishing_broken_keep_alive_conection)
{
    pubnub_init(pbp, "publ-key", "sub-Key");

    pubnub_set_user_id(pbp, "admin");
    pubnub_set_auth(pbp, "msgs");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/subscribe/sub-Key/[ch1,ch2]/0"
                            "/0?pnsdk=unit-test-0.1&channel-group=[chgr2,chgr3,chgr4]"
                            "&uuid=admin&auth=msgs");
    incoming("HTTP/1.1 200\r\nContent-Length: "
             "26\r\n\r\n[[],\"3516149789251234578\"]",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(pbp, "[ch1,ch2]", "[chgr2,chgr3,chgr4]"),
           equals(PNR_OK));
    attest(pbp->core.user_id_len, equals(5));

    attest(pubnub_get_channel(pbp), streqs(NULL));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));

    /* Sending GET request returns failure which means broken connection */
    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect(pbpal_send_str, when(s, streqs("GET ")), returns(-1));
    /* Connection is not closed instantaneously */
    expect(pbpal_close, when(pb, equals(pbp)), returns(+1));
    expect(pbpal_closed, when(pb, equals(pbp)), returns(true));
    expect(pbpal_forget, when(pb, equals(pbp)));
    /* Renewing DNS resolution and reestablishing connection */
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/subscribe/sub-Key/[ch1,ch2]/0/"
                             "3516149789251234578?pnsdk=unit-test-0.1&channel-group=[chgr2,chgr3,chgr4]"
                             "&uuid=admin&auth=msgs");
    incoming("HTTP/1.1 200\r\nContent-Length: "
             "150\r\n\r\n[[msg1,msg2,{\"text\":\"Hello "
             "World!\"},msg4,msg5,{\"key\":\"val\\ue\"}],"
             "\"352624978925123458\",\"chgr4,chgr2,chgr3,chgr4,chgr7,chgr4\","
             "\"ch5,ch8,ch6,ch17,ch1,ch2\"]",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(pbp, "[ch1,ch2]", "[chgr2,chgr3,chgr4]"),
           equals(PNR_OK));

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
    expect_outgoing_with_url("/subscribe/sub-Key/ch17/0/"
                             "352624978925123458?pnsdk=unit-test-0.1&uuid="
                             "admin&auth=msgs");
    incoming("HTTP/1.1 200\r\nContent-Length: "
             "47\r\n\r\n[[message],\"352624979925123457\",\"chgr4\",\"ch17\"]",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(pbp, "ch17", NULL), equals(PNR_OK));
    attest(pubnub_subscribe(pbp, NULL, "chgr2"), equals(PNR_RX_BUFF_NOT_EMPTY));

    attest(pubnub_get(pbp), streqs("message"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), streqs("ch17"));
    attest(pubnub_get_channel(pbp), streqs(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
}

Ensure(single_context_pubnub, subscribe_not_using_keep_alive_connection)
{
    pubnub_init(pbp, "publ-key", "sub-Key");

    /* Won't be using default 'keep-alive' connection */
    pubnub_dont_use_http_keep_alive(pbp);

    /* Shouldn't make any difference having set these parameters */
    pubnub_set_keep_alive_param(pbp, 49, 50);

    pubnub_set_user_id(pbp, "admin");
    pubnub_set_auth(pbp, "msgs");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/subscribe/sub-Key/[ch1,ch2]/0/"
                             "0?pnsdk=unit-test-0.1&channel-group=[chgr2,chgr3,"
                             "chgr4]&uuid=admin&auth=msgs");
    incoming_and_close("HTTP/1.1 200\r\nContent-Length: "
                       "26\r\n\r\n[[],\"3516149789251234578\"]",
                       NULL);
    attest(pubnub_subscribe(pbp, "[ch1,ch2]", "[chgr2,chgr3,chgr4]"),
           equals(PNR_OK));
    attest(pbp->core.user_id_len, equals(5));

    attest(pubnub_get_channel(pbp), streqs(NULL));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
}

Ensure(single_context_pubnub, subscribe_not_using_and_than_using_keep_alive_connection)
{
    pubnub_init(pbp, "publ-key", "sub-Key");

    /* Won't be using default 'keep-alive' connection */
    pubnub_dont_use_http_keep_alive(pbp);

    pubnub_set_user_id(pbp, "admin");
    pubnub_set_auth(pbp, "msgs");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/subscribe/sub-Key/[ch1,ch2]/0/"
                             "0?pnsdk=unit-test-0.1&channel-group=[chgr2,chgr3,"
                             "chgr4]&uuid=admin&auth=msgs");
    incoming_and_close("HTTP/1.1 200\r\nContent-Length: "
                       "26\r\n\r\n[[],\"3516149789251234578\"]",
                       NULL);
    attest(pubnub_subscribe(pbp, "[ch1,ch2]", "[chgr2,chgr3,chgr4]"),
           equals(PNR_OK));
    attest(pbp->core.user_id_len, equals(5));

    attest(pubnub_get_channel(pbp), streqs(NULL));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));

    /* After finishing following transaction switches connection to 'keep_alive'
     */
    pubnub_use_http_keep_alive(pbp);
    /* Renewing DNS resolution with new request */
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/subscribe/sub-Key/[ch1,ch2]/0/"
                             "3516149789251234578?pnsdk=unit-test-0.1&channel-"
                             "group=[chgr2,chgr3,chgr4]&uuid=admin&auth=msgs");
    incoming("HTTP/1.1 200\r\nContent-Length: "
             "150\r\n\r\n[[msg1,msg2,{\"text\":\"Hello "
             "World!\"},msg4,msg5,{\"key\":\"val\\ue\"}],"
             "\"352624978925123458\",\"chgr4,chgr2,chgr3,chgr4,chgr7,chgr4\","
             "\"ch5,ch8,ch6,ch17,ch1,ch2\"]",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(pbp, "[ch1,ch2]", "[chgr2,chgr3,chgr4]"),
           equals(PNR_OK));

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
    expect_outgoing_with_url("/subscribe/sub-Key/ch17/0/"
                             "352624978925123458?pnsdk=unit-test-0.1&uuid="
                             "admin&auth=msgs");
    /* simulates 'callback' condition of PNR_IN_PROGRESS.
     * (expl: recv_msg would return 0 otherwise as if the connection
     * closes from servers side.)
     */
    incoming("", NULL);
    expect(pbpal_close, when(pb, equals(pbp)), returns(0));
    expect(pbpal_forget, when(pb, equals(pbp)));
    expect(pbntf_requeue_for_processing, when(pb, equals(pbp)), returns(0));
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/subscribe/sub-Key/ch17/0/"
                             "352624978925123458?pnsdk=unit-test-0.1&uuid="
                             "admin&auth=msgs");
    incoming("HTTP/1.1 200\r\nContent-Length: "
             "47\r\n\r\n[[message],\"352624979925123457\",\"chgr4\",\"ch17\"]",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    /* Transaction timer expires while expecting the answer because the connection
       was broken at some place, at some point, from some reason and, in meantime,
       reestablished again on that stretch, while our connection with origin server
       no longer exists... */
    attest(pubnub_subscribe(pbp, "ch17", NULL), equals(PNR_STARTED));
    pbnc_stop(pbp, PNR_TIMEOUT);
    pbnc_fsm(pbp);
    attest(pubnub_subscribe(pbp, NULL, "chgr2"), equals(PNR_RX_BUFF_NOT_EMPTY));

    attest(pubnub_get(pbp), streqs("message"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), streqs("ch17"));
    attest(pubnub_get_channel(pbp), streqs(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
}

/* KEEP_ALIVE_ADVANCED */

Ensure(single_context_pubnub, keeps_connection_alive_for_certain_number_of_operations)
{
    pubnub_init(pbp, "publ-persian", "sub-persian");
    pubnub_set_user_id(pbp, "test_id");

    attest(pubnub_auth_get(pbp), streqs(NULL));
    attest(pubnub_last_time_token(pbp), streqs("0"));

    /* Keep connection alive for up to 1 second or up to 3 operations */
    pubnub_set_keep_alive_param(pbp, 0, 3);

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url(
        "/subscribe/sub-persian/civilization/0/0?pnsdk=unit-test-0.1&uuid=test_id");
    incoming("HTTP/1.1 200\r\nContent-Length: "
             "26\r\n\r\n[[],\"1516014978925123458\"]",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(pbp, "civilization", NULL), equals(PNR_OK));
    /* Awaits given number of seconds. If changed to greater value test fails.
     */
    wait_time_in_seconds(0);

    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_last_time_token(pbp), streqs("1516014978925123458"));
    /* Not publish operation */
    attest(pubnub_last_publish_result(pbp), streqs(""));

    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/subscribe/sub-persian/civilization/0/"
                             "1516014978925123458?pnsdk=unit-test-0.1&uuid=test_id");
    incoming("HTTP/1.1 200\r\nContent-Length: "
             "46\r\n\r\n[[\"tigris\",\"euphrates\"],\"1516014978925123459\"]",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(pbp, "civilization", NULL), equals(PNR_OK));

    /* Same outcome for any delay between last two operations,
       if one before the last was accomplished within given time interval */
    wait_time_in_seconds(0);

    attest(pubnub_last_time_token(pbp), streqs("1516014978925123459"));

    attest(pubnub_get(pbp), streqs("\"tigris\""));
    attest(pubnub_get(pbp), streqs("\"euphrates\""));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), streqs(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));

    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/publish/publ-persian/sub-persian/0/civilization/"
                             "0/%22hanging_gardens%22?pnsdk=unit-test-0.1&uuid=test_id");
    incoming_and_close("HTTP/1.1 200\r\nContent-Length: "
                       "32\r\n\r\n[1,\"Sent\",\"1516014978925123459\"]",
                       NULL);
    attest(pubnub_last_publish_result(pbp), streqs(""));
    attest(pubnub_publish(pbp, "civilization", "\"hanging_gardens\""),
           equals(PNR_OK));
    attest(pubnub_last_publish_result(pbp), streqs("\"Sent\""));
    attest(pubnub_last_http_code(pbp), equals(200));

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/subscribe/sub-persian/civilization/0/"
                             "1516014978925123459?pnsdk=unit-test-0.1&uuid=test_id");
    incoming("HTTP/1.1 200\r\nContent-Length: 61\r\n\r\n[[\"hanging_gardens\", "
             "\"arabian_nights\"],\"1516714978925123460\"]",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(pbp, "civilization", NULL), equals(PNR_OK));
    attest(pubnub_last_time_token(pbp), streqs("1516714978925123460"));

    attest(pubnub_get(pbp), streqs("\"hanging_gardens\""));
    attest(pubnub_get(pbp), streqs(" \"arabian_nights\""));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), streqs(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
}

Ensure(single_context_pubnub,
       doesnt_keep_connection_alive_confinement_number_of_operations)
{
    pubnub_init(pbp, "publ-some", "sub-some");
    pubnub_set_user_id(pbp, "test_id");

    attest(pubnub_auth_get(pbp), streqs(NULL));
    attest(pubnub_last_time_token(pbp), streqs("0"));

    /* Keep connection alive for up to 3 seconds or up to 0 operations */
    pubnub_set_keep_alive_param(pbp, 2, 0);

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url(
        "/subscribe/sub-some/light/0/0?pnsdk=unit-test-0.1&uuid=test_id");
    incoming_and_close("HTTP/1.1 200\r\nContent-Length: "
                       "26\r\n\r\n[[],\"1516014978925123458\"]",
                       NULL);
    attest(pubnub_subscribe(pbp, "light", NULL), equals(PNR_OK));

    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_last_time_token(pbp), streqs("1516014978925123458"));
    /* Not publish operation */
    attest(pubnub_last_publish_result(pbp), streqs(""));

    /* Keep connection alive for up to 3 seconds or up to 1 operation */
    pubnub_set_keep_alive_param(pbp, 2, 1);

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url(
        "/subscribe/sub-some/light/0/1516014978925123458?pnsdk=unit-test-0.1&uuid=test_id");
    incoming_and_close("HTTP/1.1 200\r\nContent-Length: "
                       "35\r\n\r\n[[warm,cold],\"1516014978925123459\"]",
                       NULL);
    attest(pubnub_subscribe(pbp, "light", NULL), equals(PNR_OK));

    attest(pubnub_last_time_token(pbp), streqs("1516014978925123459"));

    attest(pubnub_get(pbp), streqs("warm"));
    attest(pubnub_get(pbp), streqs("cold"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), streqs(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
}

Ensure(single_context_pubnub, keep_alive_connection_closes_time_runs_out)
{
    pubnub_init(pbp, "publ-some", "sub-some");
    pubnub_set_user_id(pbp, "test_id");

    attest(pubnub_auth_get(pbp), streqs(NULL));
    attest(pubnub_last_time_token(pbp), streqs("0"));

    /* Keep connection alive for up to 1 second or up to 3 operations */
    pubnub_set_keep_alive_param(pbp, 0, 3);

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url(
        "/subscribe/sub-some/discharge/0/0?pnsdk=unit-test-0.1&uuid=test_id");
    incoming("HTTP/1.1 200\r\nContent-Length: "
             "26\r\n\r\n[[],\"1516014978925123458\"]",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(pbp, "discharge", NULL), equals(PNR_OK));

    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
    attest(pubnub_last_time_token(pbp), streqs("1516014978925123458"));
    /* Not publish operation */
    attest(pubnub_last_publish_result(pbp), streqs(""));
    /* Time runs out. Connection closes after following operation */
    wait_time_in_seconds(1);

    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/subscribe/sub-some/discharge/0/"
                             "1516014978925123458?pnsdk=unit-test-0.1&uuid=test_id");
    incoming_and_close("HTTP/1.1 200\r\nContent-Length: "
                       "35\r\n\r\n[[lightning],\"1516014978925123459\"]",
                       NULL);
    attest(pubnub_subscribe(pbp, "discharge", NULL), equals(PNR_OK));

    attest(pubnub_last_time_token(pbp), streqs("1516014978925123459"));

    attest(pubnub_get(pbp), streqs("lightning"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), streqs(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
}

Ensure(single_context_pubnub,
       subscribe_not_using_keep_alive_and_than_using_it_again_with_parameters)
{
    pubnub_init(pbp, "publ-key", "sub-Key");
    pubnub_set_user_id(pbp, "test_id");

    /* Won't be using default 'keep-alive' connection */
    pubnub_dont_use_http_keep_alive(pbp);

    /* Don't influence and shouldn't be influenced by operations while
     * 'keep_alive' is off */
    pubnub_set_keep_alive_param(pbp, 5, 2);

    pubnub_set_user_id(pbp, "admin");
    pubnub_set_auth(pbp, "msgs");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/subscribe/sub-Key/[ch1,ch2]/0/"
                             "0?pnsdk=unit-test-0.1&channel-group=[chgr2,chgr3,"
                             "chgr4]&uuid=admin&auth=msgs");
    incoming_and_close("HTTP/1.1 200\r\nContent-Length: "
                       "26\r\n\r\n[[],\"3516149789251234578\"]",
                       NULL);
    attest(pubnub_subscribe(pbp, "[ch1,ch2]", "[chgr2,chgr3,chgr4]"),
           equals(PNR_OK));
    attest(pbp->core.user_id_len, equals(5));

    attest(pubnub_get_channel(pbp), streqs(NULL));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));

    /* Turns back on 'keep-alive' */
    pubnub_use_http_keep_alive(pbp);

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/subscribe/sub-Key/ch1/0/"
                             "3516149789251234578?pnsdk=unit-test-0.1&uuid="
                             "admin&auth=msgs");
    incoming("HTTP/1.1 200\r\nContent-Length: "
             "30\r\n\r\n[[msg1],\"3516149789251234579\"]",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(pbp, "ch1", NULL), equals(PNR_OK));

    attest(pubnub_get(pbp), streqs("msg1"));
    attest(pubnub_get_channel(pbp), streqs(NULL));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));

    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/subscribe/sub-Key/ch1/0/"
                             "3516149789251234579?pnsdk=unit-test-0.1&uuid="
                             "admin&auth=msgs");
    incoming_and_close("HTTP/1.1 200\r\nContent-Length: "
                       "30\r\n\r\n[[msg2],\"3516149789251234580\"]",
                       NULL);
    attest(pubnub_subscribe(pbp, "ch1", NULL), equals(PNR_OK));

    attest(pubnub_get(pbp), streqs("msg2"));
    attest(pubnub_get_channel(pbp), streqs(NULL));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
}


/* DATA COMPRESSION */

Ensure(single_context_pubnub, subscribe_gzip_response)
{
    uint8_t gzip_chunk0[] = { 0x1f, 0x8b, 0x08, 0x00, 0xae, 0x8b, 0xe3,
                              0x5a, 0x02, 0x03, 0xad, 0x8d, 0x3b, 0x0b,
                              0xc2, 0x30, 0x14, 0x85, 0xff, 0x8a, 0xdc,
                              0xd5, 0x54, 0x48, 0xda, 0xf4, 0xe1, 0x26,
                              0x8e, 0x22, 0xb8, 0x38, 0x95, 0x52, 0x6e,
                              0xdb, 0x68, 0x03, 0x6d, 0x52, 0xd2 };
    struct uint8_block chunk_block0  = { 41, gzip_chunk0 };
    uint8_t            gzip_chunk1[] = {
        0x9b, 0x41, 0xa4, 0xff, 0xdd, 0x3e, 0x16, 0x47, 0x07, 0xa7, 0xcb, 0xf7,
        0x9d, 0x73, 0xb8, 0x79, 0xfe, 0x06, 0xef, 0x75, 0xc3, 0xe1, 0x08, 0x81,
        0x80, 0x89, 0x6d, 0x28, 0x16, 0x94, 0x2b, 0x5e, 0xb4, 0x79, 0x8e, 0x64,
        0xcd, 0x6c, 0xf6, 0x62, 0x6b, 0x5c, 0xd1, 0xe8, 0x0e, 0x17, 0x11, 0xa6,
        0xab, 0xa8, 0xac, 0x7f, 0x25, 0xf1, 0x22, 0xb6, 0xcd, 0xcd, 0x1d, 0x76,
        0xa7, 0x91, 0x9c, 0x6e, 0x76, 0x67, 0x8b, 0x23, 0xcd, 0xc9, 0xdd, 0x60,
        0xd5, 0xa9, 0x92, 0x6c, 0xf9, 0x50, 0x54, 0xb7, 0x25, 0xa9, 0x7e, 0x50,
        0x0e, 0xc9, 0x3b, 0x05, 0x53, 0xc1, 0x80, 0x4b, 0x1e, 0xf3, 0x28, 0x4b,
        0xd2, 0x4c, 0x48, 0x2e, 0xc2, 0x48, 0xa6, 0x21, 0x30, 0x40, 0xed, 0x82,
        0xaf, 0x26, 0xfb, 0x33, 0xcf, 0x1f, 0xea, 0x36, 0x40, 0xea, 0x07, 0xf6,
        0xe3, 0x85, 0xe2, 0x03, 0x8a, 0x9f, 0x27, 0xee, 0x32, 0x01, 0x00, 0x00
    };
    struct uint8_block chunk_block1 = { 132, gzip_chunk1 };
    uint8_t            gzip_body2[] = {
        0x1f, 0x8b, 0x08, 0x00, 0xa7, 0x37, 0xe3, 0x5a, 0x02, 0x03, 0xad, 0xce,
        0xcb, 0x0a, 0x83, 0x30, 0x10, 0x05, 0xd0, 0x5f, 0x91, 0x59, 0xc7, 0x45,
        0xa2, 0x69, 0xaa, 0x9f, 0xd0, 0x76, 0x51, 0xe8, 0x52, 0x5c, 0x44, 0x33,
        0x60, 0x68, 0x1b, 0x83, 0x0f, 0x42, 0x11, 0xff, 0xbd, 0x51, 0xc1, 0x07,
        0x74, 0xd9, 0xd5, 0xcd, 0x30, 0x39, 0x37, 0xc9, 0xb2, 0x01, 0xfa, 0x5e,
        0x2b, 0x0a, 0xe9, 0x00, 0x4a, 0x37, 0x90, 0x3a, 0x02, 0xad, 0x45, 0x54,
        0x90, 0x02, 0x7b, 0xdb, 0x0a, 0x08, 0x14, 0xaf, 0xda, 0xb5, 0x7e, 0x8c,
        0xa6, 0x71, 0x1c, 0xc9, 0x22, 0xd8, 0x2a, 0xcc, 0x8e, 0xf0, 0x23, 0x11,
        0x2b, 0x79, 0x60, 0xd1, 0x93, 0xe0, 0x5e, 0xe9, 0x97, 0xb6, 0x56, 0x1b,
        0x6c, 0x37, 0x6e, 0x70, 0xf3, 0xf1, 0xd1, 0xc7, 0x9b, 0x6f, 0x74, 0x70,
        0x91, 0x1f, 0xe9, 0x64, 0xa3, 0xd0, 0xa0, 0xed, 0x1b, 0x19, 0x5c, 0xeb,
        0xae, 0x43, 0x12, 0x4c, 0xab, 0x9b, 0x34, 0x4f, 0xb9, 0x55, 0xee, 0x1a,
        0xc5, 0xcf, 0x1f, 0xe5, 0x04, 0x28, 0xa7, 0x27, 0x1a, 0x27, 0xe2, 0x9c,
        0x30, 0x4e, 0x59, 0x14, 0xf3, 0x44, 0xf8, 0x7b, 0x4e, 0x1b, 0x15, 0xce,
        0x36, 0x94, 0xfe, 0xe4, 0xeb, 0xb0, 0xec, 0x74, 0x6d, 0xc8, 0xff, 0x16,
        0xfe, 0x95, 0xb2, 0x0a, 0x5d, 0x4b, 0xc9, 0x1c, 0x6c, 0x89, 0x68, 0x09,
        0x0a, 0xf9, 0x17, 0x1e, 0xf5, 0xc8, 0xfc, 0x94, 0x01, 0x00, 0x00
    };
    struct uint8_block body_block2 = { 203, gzip_body2 };

    pubnub_init(pbp, "publ-measurements", "sub-measurements");

    pubnub_set_user_id(pbp, "technician");
    pubnub_set_auth(pbp, "weather-conditions");
    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/subscribe/sub-measurements/,/0/"
                             "0?pnsdk=unit-test-0.1&channel-group=[air-"
                             "temperature,humidity,wind-speed-and-direction,"
                             "pressure]&uuid=technician&auth=weather-"
                             "conditions");
    incoming("HTTP/1.1 200\r\nContent-Length: "
             "26\r\n\r\n[[],\"1516149789251234578\"]",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(
               pbp, NULL, "[air-temperature,humidity,wind-speed-and-direction,pressure]"),
           equals(PNR_OK));
    attest(pbp->core.user_id_len, equals(10));

    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), streqs(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));

    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/subscribe/sub-measurements/,/0/"
                             "1516149789251234578?pnsdk=unit-test-0.1&channel-"
                             "group=[air-temperature,humidity,wind-speed-and-"
                             "direction,pressure]&uuid=technician&auth=weather-"
                             "conditions");
    incoming("HTTP/1.1 200\r\n"
             "Transfer-Encoding: chunked\r\n"
             "Content-Encoding: gzip\r\n"
             "\r\n"
             "29\r\n",
             &chunk_block0);
    incoming("\r\n84\r\n", &chunk_block1);
    incoming("\r\n0\r\n", NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(
               pbp, NULL, "[air-temperature,humidity,wind-speed-and-direction,pressure]"),
           equals(PNR_STARTED));
    attest(pubnub_subscribe(pbp, "some_channels", "some_channel_goups"),
           equals(PNR_IN_PROGRESS));

    /* 'push' until finished */
    attest(pbnc_fsm(pbp), equals(0));
    attest(pbnc_fsm(pbp), equals(0));
    attest(pbp->core.last_result, equals(PNR_OK));

    attest(pubnub_subscribe(pbp, NULL, "some_channel_goups"),
           equals(PNR_RX_BUFF_NOT_EMPTY));

    attest(pubnub_get(pbp), streqs("{\"uuid1\":\"-2\"}"));
    attest(pubnub_get_channel(pbp), streqs("ch-atmp"));
    attest(pubnub_get(pbp), streqs("{\"uuid2\":\"-5\"}"));
    attest(pubnub_get(pbp), streqs("{\"Kingston\":\"+22\"}"));
    attest(pubnub_get(pbp), streqs("{\"Manila\":\"+38\"}"));
    attest(pubnub_get(pbp), streqs("{\"bouy76\":\"+5\"}"));
    attest(pubnub_subscribe(pbp, NULL, "wind-speed-and-direction"),
           equals(PNR_RX_BUFF_NOT_EMPTY));
    attest(pubnub_get(pbp),
           streqs("{\"Pr. Astrid Coast\":\"Unable_to_fetch_temperature\"}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));

    pubnub_set_user_id(pbp, "bird");
    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/v2/presence/sub-key/sub-measurements/uuid/"
                             "bird?pnsdk=unit-test-0.1&uuid=bird&auth=weather-"
                             "conditions");
    incoming("HTTP/1.1 200\r\nContent-Length: "
             "100\r\n\r\n{\"status\":200,\"message\":\"OK\",\"service\":"
             "\"Presence\",\"Payload\":{\"channels\":[discovery,nat_geo,nature]"
             "}}",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    /* Recognizes the uuid set from the context */
    attest(pubnub_where_now(pbp, NULL), equals(PNR_OK));
    attest(pbp->core.user_id_len, equals(4));

    attest(pubnub_get(pbp),
           streqs("{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\","
                  "\"Payload\":{\"channels\":[discovery,nat_geo,nature]}}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));

    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/subscribe/sub-measurements/,/0/"
                             "1516149789251234583?pnsdk=unit-test-0.1&channel-"
                             "group=[air-temperature,humidity,wind-speed-and-"
                             "direction,pressure]&uuid=bird&auth=weather-"
                             "conditions");
    incoming("HTTP/1.1 200\r\n"
             "Content-Encoding: gzip\r\n"
             "Content-Length: 203\r\n"
             "\r\n",
             &body_block2);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(
               pbp, NULL, "[air-temperature,humidity,wind-speed-and-direction,pressure]"),
           equals(PNR_OK));

    attest(pubnub_get_channel(pbp), streqs("ch-ws1"));
    attest(
        pubnub_get(pbp),
        streqs(
            "{\"uuid1\":{\"dir\":w,\"speed\":\"2mph\",\"blows\":\"3mph\"}}"));
    attest(
        pubnub_get(pbp),
        streqs(
            "{\"uuid2\":{\"dir\":nw,\"speed\":\"5mph\",\"blows\":\"7mph\"}}"));
    attest(pubnub_get(pbp), streqs("{\"Sebu, Philippines\":{\"dir\":nne,\"speed\":\"4mph\",\"blows\":\"4mph\"}}"));
    attest(pubnub_get(pbp), streqs("{\"Sri Jayawardenepura Kotte, Sri Lanka\":{\"dir\":ne,\"speed\":\"7mph\",\"blows\":\"7mph\"}}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
}

Ensure(single_context_pubnub, global_here_now_gzip_response)
{
    uint8_t gzip_body[] = {
        0x1f, 0x8b, 0x08, 0x00, 0x26, 0x60, 0xe4, 0x5a, 0x02, 0x03, 0x5d, 0xcf,
        0x4b, 0x0a, 0x83, 0x30, 0x10, 0x06, 0xe0, 0xab, 0xc8, 0xac, 0x67, 0xa1,
        0x89, 0x52, 0xc8, 0x15, 0xba, 0x68, 0xf7, 0x45, 0x4a, 0x88, 0xa1, 0x0a,
        0x31, 0x91, 0x3c, 0x5a, 0x44, 0xbc, 0x5a, 0xcf, 0xd0, 0x23, 0x35, 0x06,
        0x04, 0xe3, 0x66, 0x92, 0x0c, 0xdf, 0x3f, 0x4c, 0x16, 0x70, 0x9e, 0xfb,
        0xe0, 0x80, 0x91, 0xb2, 0x44, 0x18, 0xa5, 0x73, 0xfc, 0x25, 0x81, 0xc1,
        0xed, 0x0a, 0x08, 0x4e, 0xda, 0xf7, 0x20, 0xb6, 0xe7, 0xdd, 0x4a, 0x27,
        0x75, 0xbc, 0x22, 0x4c, 0x7c, 0x56, 0x86, 0x77, 0xc0, 0x16, 0xd1, 0x73,
        0xad, 0xa5, 0x72, 0x6c, 0x01, 0xd1, 0x57, 0xb1, 0x01, 0x21, 0x0c, 0x5d,
        0x9c, 0xf5, 0xd8, 0xce, 0x0a, 0xb7, 0x4a, 0x52, 0xa5, 0x2d, 0x82, 0x11,
        0x22, 0x4c, 0x5c, 0x8b, 0x19, 0x18, 0x5d, 0x31, 0x26, 0xc8, 0x39, 0x41,
        0x93, 0xad, 0x73, 0x4b, 0x92, 0xa5, 0x67, 0x7b, 0xc9, 0x55, 0x95, 0x54,
        0x7d, 0x54, 0x39, 0x28, 0x13, 0x68, 0x8e, 0xc0, 0xdb, 0x41, 0xcc, 0x42,
        0x49, 0x2c, 0x46, 0xe3, 0x7b, 0x69, 0xb1, 0x70, 0x9f, 0xdf, 0xd7, 0x76,
        0x2d, 0x16, 0xf9, 0xb2, 0x31, 0xea, 0x8d, 0xe7, 0xea, 0xb9, 0x7f, 0x18,
        0x58, 0xb3, 0xb7, 0x8e, 0x3b, 0x90, 0x75, 0xfd, 0x03, 0x8d, 0x7b, 0x90,
        0x91, 0x50, 0x01, 0x00, 0x00
    };
    struct uint8_block body_block = { 197, gzip_body };

    uint8_t gzip_chunk1[] = {
        0x1f, 0x8b, 0x08, 0x00, 0x67, 0x51, 0xe4, 0x5a, 0x02, 0x03, 0xb5, 0x5a,
        0x69, 0x73, 0x1b, 0x47, 0x92, 0xfd, 0x2b, 0x13, 0xfa, 0xbc, 0x85, 0xa8,
        0xfb, 0xd8, 0x6f, 0x38, 0x6d, 0x79, 0x64, 0x51, 0x23, 0x79, 0x67, 0x6c,
        0x6f, 0x4c, 0x20, 0xea, 0x94, 0x20, 0x93, 0x00, 0x87, 0x00, 0xa5, 0xd1,
        0x1c, 0xff, 0x7d, 0x5f, 0x56, 0x77, 0x43, 0xa4, 0x65, 0x07, 0x38, 0x1b,
        0xb6, 0x29, 0xd2, 0xec, 0xee, 0xea, 0xaa, 0xac, 0xcc, 0x97, 0x2f, 0x5f,
        0x16, 0xf8, 0xcf, 0x67, 0xc7, 0x53, 0x3c, 0xdd, 0x1f, 0x9f, 0xfd, 0xf7,
        0x1f, 0x24, 0xe7, 0xff, 0xf5, 0x87, 0x67, 0x37, 0xf5, 0x78, 0x8c, 0x6f,
        0x2b, 0xae, 0x9f, 0x5d, 0xfd, 0xf1, 0x19, 0x6e, 0xdc, 0xc6, 0x4f, 0xd7,
        0x87, 0x58, 0x70, 0xe3, 0x9f, 0xcf, 0xf2, 0xbb, 0xb8, 0xdf, 0xd7, 0xeb,
        0x63, 0xbf, 0xb8, 0xdb, 0xdd, 0xc6, 0x3d, 0x7b, 0x17, 0xaf, 0xaf, 0x0f,
        0xfb, 0x7a, 0x62, 0xc7, 0xba, 0x3f, 0x1e, 0xee, 0x86, 0x47, 0x87, 0x9c,
        0xef, 0xf1, 0x30, 0x7f, 0xc2, 0x95, 0xc0, 0x1c, 0xf7, 0xf7, 0xbb, 0x42,
        0x4f, 0xfe, 0xf7, 0x99, 0x77, 0xb2, 0x96, 0x66, 0x38, 0x4b, 0x59, 0x56,
        0xa6, 0x93, 0x8f, 0x2c, 0xb6, 0x28, 0x58, 0x48, 0x26, 0x28, 0xe5, 0x22,
        0x7e, 0xfa, 0x67, 0x7f, 0xfd, 0x37, 0xde, 0x79, 0x75, 0x77, 0xf8, 0xfb,
        0xa7, 0xed, 0xf2, 0x5d, 0xcd, 0x3f, 0x6d, 0xaf, 0xf6, 0xd7, 0xbb, 0x7d,
        0xdd, 0xaa, 0x0b, 0x93, 0x4b, 0x37, 0x73, 0x7a, 0x26, 0x14, 0x9f, 0x09,
        0xee, 0x86, 0x59, 0x62, 0xb9, 0xd9, 0xed, 0x6f, 0x0f, 0x77, 0xa7, 0x78,
        0xfd, 0xc5, 0xcb, 0xf2, 0xd1, 0xcb, 0x45, 0xf2, 0x26, 0x5d, 0x12, 0xac,
        0x5a, 0xdb, 0x98, 0x96, 0xda, 0xb1, 0x18, 0x92, 0x66, 0xd2, 0x6a, 0xad,
        0x64, 0xf5, 0x36, 0xda, 0x44, 0xfe, 0x10, 0xbc, 0x5a, 0x5f, 0xa4, 0x66,
        0x5c, 0x6a, 0xc5, 0xb4, 0xd7, 0x99, 0xf9, 0x60, 0x39, 0x93, 0x4e, 0x9b,
        0x9c, 0xaa, 0xa8, 0x3a, 0xa7, 0x61, 0xed, 0x37, 0xef, 0x76, 0xf5, 0xba,
        0x6c, 0xbf, 0xdc, 0x88, 0xfc, 0x72, 0x1f, 0x86, 0x3f, 0x32, 0x46, 0x48,
        0xec, 0x83, 0x9b, 0x99, 0x90, 0x62, 0x26, 0xac, 0xef, 0xeb, 0x0a, 0x3e,
        0x33, 0x7a, 0x26, 0x95, 0xc3, 0x1d, 0x4d, 0x77, 0x74, 0x98, 0x09, 0x8d,
        0xab, 0xa0, 0x66, 0x41, 0x0d, 0x43, 0x14, 0x9e, 0x89, 0x99, 0xc1, 0x8b,
        0x9c, 0xf7, 0x21, 0x76, 0xa6, 0xec, 0xcc, 0x9a, 0x99, 0x14, 0xfd, 0x1a,
        0x0f, 0x25, 0xdc, 0xe3, 0xe1, 0x24, 0x3d, 0xbc, 0x21, 0x67, 0x92, 0xe3,
        0x2d, 0xed, 0x67, 0xce, 0xf4, 0x3b, 0x92, 0xcf, 0x24, 0x26, 0xe6, 0xf8,
        0x16, 0x81, 0xee, 0x60, 0x02, 0x49, 0xdf, 0xb0, 0x44, 0xda, 0x3e, 0x24,
        0xc0, 0xc1, 0xd2, 0x93, 0x19, 0x33, 0xe1, 0x25, 0xdd, 0x82, 0xe7, 0x25,
        0x99, 0x42, 0xae, 0x0f, 0x83, 0xb9, 0x9e, 0x7e, 0x15, 0x33, 0x8f, 0xbb,
        0xa2, 0xaf, 0x45, 0x86, 0x7b, 0x3b, 0xd3, 0x72, 0x66, 0xc7, 0xb5, 0x03,
        0xec, 0x9e, 0xd1, 0xe2, 0xe3, 0xb5, 0x9d, 0x05, 0x9a, 0x15, 0xff, 0xb3,
        0xe7, 0x85, 0xf1, 0x98, 0x0b, 0x9a, 0xa9, 0x2f, 0x03, 0x53, 0xa5, 0xc4,
        0xad, 0x6e, 0x41, 0xdf, 0x51, 0xd0, 0x33, 0x13, 0x06, 0x5f, 0x75, 0x6b,
        0x8d, 0xef, 0xab, 0x88, 0xe0, 0x3e, 0xef, 0x11, 0x97, 0xb4, 0x47, 0x4e,
        0x8f, 0xfa, 0x4b, 0xc2, 0x38, 0xb2, 0x43, 0xd0, 0xb7, 0x17, 0xc3, 0xb6,
        0x25, 0x2c, 0x51, 0x01, 0xdb, 0x14, 0x67, 0xdf, 0xfa, 0x99, 0x87, 0xb1,
        0xc3, 0x73, 0x47, 0x3e, 0x94, 0x33, 0x47, 0x6b, 0x0d, 0x77, 0x38, 0xa6,
        0x1d, 0xde, 0xd0, 0xd3, 0x0c, 0xb4, 0x94, 0x94, 0x66, 0xe6, 0xec, 0x67,
        0xe7, 0xc2, 0x9d, 0x34, 0xd2, 0x8c, 0xae, 0x33, 0x04, 0x51, 0x49, 0xe1,
        0x55, 0x72, 0xb8, 0x83, 0x41, 0xe4, 0x4c, 0x2c, 0x20, 0xd4, 0xf4, 0x9a,
        0x20, 0x47, 0x90, 0xcd, 0x63, 0x50, 0x06, 0x57, 0x49, 0x03, 0x4f, 0xb8,
        0xbe, 0xba, 0xd7, 0x33, 0x4c, 0x25, 0x95, 0x9c, 0x6e, 0xd0, 0x68, 0xcc,
        0x40, 0x21, 0x38, 0xdb, 0x8f, 0x6b, 0x41, 0x51, 0xeb, 0x01, 0xf1, 0x98,
        0xd1, 0x90, 0x89, 0xe4, 0x88, 0xcf, 0x06, 0xe3, 0x2e, 0xa1, 0x66, 0x9c,
        0x44, 0xc0, 0x3c, 0xbc, 0xe5, 0x60, 0xb0, 0x0e, 0x23, 0x60, 0xba, 0x47,
        0x71, 0xd3, 0xfb, 0x73, 0x50, 0xf8, 0x4c, 0xe1, 0x86, 0x71, 0x93, 0x7b,
        0x1d, 0x85, 0xc4, 0x4e, 0x2b, 0x75, 0xc8, 0x62, 0x94, 0x99, 0x69, 0xf5,
        0xd9, 0x0f, 0xd8, 0x31, 0xb6, 0x2a, 0x2c, 0x9f, 0xd6, 0xa6, 0xaf, 0x01,
        0x3d, 0x67, 0x68, 0x13, 0xca, 0xba, 0xad, 0x61, 0xa6, 0x71, 0x8d, 0x5d,
        0x4b, 0x1e, 0x86, 0xf1, 0x74, 0xe9, 0x09, 0x2c, 0x2a, 0x4c, 0xf1, 0x10,
        0xb0, 0x4c, 0x62, 0x75, 0x3f, 0x6d, 0xd8, 0x70, 0xc2, 0xd2, 0xac, 0x87,
        0x03, 0x2f, 0x20, 0x0d, 0x80, 0xe4, 0x60, 0xce, 0xe3, 0x95, 0x9f, 0xc1,
        0x83, 0x23, 0x3e, 0x75, 0x87, 0x27, 0x6e, 0x49, 0x39, 0xec, 0x43, 0x8b,
        0x9e, 0x5c, 0x14, 0x2d, 0x39, 0x80, 0x5a, 0xd0, 0x7c, 0xb8, 0x81, 0x9f,
        0x83, 0x59, 0x04, 0x69, 0xd3, 0x51, 0x2e, 0x85, 0x99, 0xec, 0x22, 0xec,
        0x01, 0x82, 0x26, 0x9c, 0xf7, 0x4a, 0xef, 0x39, 0x6c, 0x69, 0x88, 0x84,
        0x1c, 0x3c, 0x0c, 0xd7, 0x7a, 0x73, 0xb6, 0x34, 0xcc, 0xec, 0x34, 0x40,
        0x08, 0x47, 0x91, 0xd3, 0xb4, 0xb7, 0x0e, 0x12, 0x00, 0xa4, 0x3b, 0x93,
        0x82, 0x2d, 0x1f, 0x22, 0x02, 0x61, 0xd1, 0x63, 0x72, 0xa9, 0x19, 0xe2,
        0x89, 0xbb, 0x72, 0xf0, 0xb0, 0xef, 0x3b, 0x03, 0x1a, 0xa4, 0x1a, 0xe7,
        0xc4, 0x66, 0xb4, 0xed, 0x11, 0x1f, 0x6c, 0xef, 0x71, 0x83, 0x95, 0x14,
        0x97, 0x1e, 0x58, 0x5a, 0x9e, 0x36, 0x46, 0xa6, 0x8e, 0xe9, 0x49, 0x86,
        0x4b, 0x5a, 0x36, 0xc8, 0x09, 0x52, 0x20, 0x52, 0xc0, 0x4e, 0x8d, 0x71,
        0xb1, 0x3d, 0x4e, 0x82, 0xcb, 0x59, 0xe0, 0xe3, 0xa4, 0x0e, 0x79, 0x66,
        0xd4, 0x04, 0x3b, 0xd1, 0x93, 0x93, 0xb0, 0x8d, 0x6f, 0x33, 0x86, 0x92,
        0xa6, 0xa5, 0x74, 0xb4, 0xf6, 0xf3, 0xac, 0x58, 0x9a, 0xf8, 0x60, 0x08,
        0x55, 0xb7, 0x44, 0xc1, 0xd3, 0x83, 0x43, 0x88, 0x81, 0xfa, 0x3f, 0x21,
        0x27, 0xff, 0x00, 0x29, 0x66, 0xf2, 0xf9, 0xe4, 0xd0, 0x10, 0xa6, 0xe4,
        0x95, 0xb4, 0xf9, 0x4e, 0x5c, 0x6a, 0x8c, 0x8a, 0xe9, 0x20, 0xed, 0x3e,
        0x1d, 0xe3, 0xdf, 0x1d, 0x12, 0x7a, 0xca, 0x4b, 0xdd, 0x6f, 0x21, 0xfa,
        0x82, 0x76, 0x03, 0xde, 0x72, 0xe1, 0x9c, 0x33, 0x81, 0x52, 0xdc, 0x8e,
        0x6c, 0xa3, 0x87, 0x58, 0xc2, 0xfd, 0x21, 0x9c, 0xf3, 0x43, 0x75, 0x30,
        0x8f, 0xa8, 0x21, 0x36, 0x13, 0x03, 0xf7, 0x99, 0x11, 0x34, 0x7e, 0xc8,
        0x3a, 0x37, 0xa1, 0xa6, 0x43, 0x4f, 0xf3, 0x8e, 0xdf, 0xf1, 0x56, 0x5f,
        0x49, 0x0f, 0xf3, 0x70, 0xfb, 0x90, 0x76, 0xb1, 0x94, 0x19, 0xe8, 0xc4,
        0x89, 0xce, 0x86, 0x0f, 0xb2, 0x88, 0xc8, 0x9d, 0x77, 0x5a, 0x1a, 0x81,
        0x42, 0x18, 0xb6, 0x8e, 0x40, 0x30, 0xbe, 0x24, 0xc9, 0x69, 0x08, 0x0a,
        0x71, 0xa8, 0x18, 0xa1, 0x42, 0x26, 0xfb, 0x19, 0x68, 0x72, 0xf0, 0x26,
        0x30, 0x6c, 0x01, 0x03, 0x39, 0xf3, 0xe7, 0x49, 0x81, 0x37, 0xa2, 0xb5,
        0x91, 0xa5, 0x45, 0x9f, 0x13, 0x8c, 0x64, 0x27, 0xd2, 0xa6, 0xd4, 0x75,
        0x7c, 0x0c, 0xd6, 0xe0, 0x7a, 0x4e, 0xac, 0x60, 0xd4, 0x79, 0x7b, 0x94,
        0x19, 0x44, 0xaf, 0x6e, 0xb8, 0x45, 0xf3, 0x83, 0xb9, 0x1f, 0x56, 0x02,
        0x7c, 0x03, 0xb4, 0xd6, 0x4c, 0x34, 0x2e, 0x00, 0x1c, 0x5a, 0x67, 0x48,
        0x35, 0x4a, 0x33, 0xac, 0xac, 0xcf, 0x66, 0xe3, 0x29, 0xae, 0x25, 0xcd,
        0x39, 0xbc, 0xd3, 0x59, 0x88, 0x6a, 0x89, 0x7b, 0x8c, 0x60, 0xaa, 0x41,
        0x66, 0x4c, 0x71, 0x4f, 0x79, 0x68, 0x27, 0xb0, 0x76, 0xd2, 0xd4, 0x53,
        0x1a, 0x9c, 0x6d, 0xa7, 0xbc, 0x81, 0xef, 0x07, 0x8a, 0x26, 0x23, 0x42,
        0x0f, 0x9f, 0x94, 0x61, 0x08, 0x5f, 0x20, 0x82, 0xf6, 0xe4, 0x85, 0x91,
        0x35, 0x2d, 0xe1, 0x8c, 0x5c, 0xe0, 0x1f, 0x30, 0x97, 0x26, 0x97, 0xf6,
        0x01, 0x94, 0xda, 0x94, 0x36, 0xee, 0x8c, 0x91, 0x21, 0x59, 0x25, 0x81,
        0x8f, 0x8f, 0x78, 0x14, 0x94, 0x37, 0xf8, 0xa7, 0x27, 0xff, 0x28, 0xf2,
        0x0f, 0xd8, 0xe3, 0x61, 0xa9, 0xec, 0xd5, 0x53, 0x3d, 0x74, 0x19, 0x76,
        0x34, 0x62, 0xa8, 0xf3, 0x0c, 0x85, 0xda, 0x4e, 0xf5, 0xa7, 0xa7, 0x41,
        0xe8, 0x5e, 0x9d, 0x32, 0x0b, 0x38, 0xc0, 0x73, 0x0b, 0xdf, 0x9f, 0xe1,
        0x01, 0xca, 0x90, 0x9c, 0x68, 0x21, 0x9c, 0xa3, 0x69, 0x7b, 0x4d, 0x1e,
        0xeb, 0xd1, 0xb4, 0x34, 0x6d, 0xbb, 0x4f, 0xdb, 0xab, 0x8a, 0xe9, 0x69,
        0xcd, 0xd5, 0x04, 0x08, 0x6d, 0x87, 0xe2, 0x77, 0x66, 0x4e, 0x92, 0x59,
        0xa4, 0x10, 0x86, 0x72, 0x8a, 0xec, 0xeb, 0xe0, 0x46, 0xdd, 0xf5, 0x7d,
        0x5a, 0x54, 0x1e, 0xbc, 0x82, 0x1f, 0xe1, 0x73, 0xa0, 0x68, 0x73, 0x63,
        0x5d, 0x24, 0xb6, 0xe2, 0x9d, 0xee, 0xc5, 0x90, 0xd2, 0x24, 0xdc, 0xec,
        0x90, 0xd1, 0x23, 0x90, 0xf1, 0xea, 0xac, 0x13, 0xf2, 0x23, 0xed, 0x43,
        0x7b, 0x1e, 0x00, 0x33, 0xd5, 0x37, 0x42, 0x92, 0x0f, 0x67, 0xb3, 0x46,
        0xb5, 0xa0, 0xc6, 0x92, 0x28, 0x3a, 0x31, 0x60, 0x3b, 0x43, 0x9d, 0x71,
        0x7e, 0x60, 0x1e, 0x2f, 0xa6, 0x32, 0x4f, 0x4c, 0x41, 0x8a, 0x89, 0x5c,
        0x3f, 0xc2, 0x79, 0x48, 0x44, 0x47, 0x15, 0xe8, 0xcc, 0x9b, 0xc4, 0x03,
        0xb4, 0x98, 0x1d, 0x15, 0x07, 0x55, 0x75, 0xc4, 0x6f, 0xd4, 0x17, 0x44,
        0x06, 0x30, 0x07, 0xaf, 0xc8, 0x70, 0xe6, 0x1a, 0x44, 0xb8, 0xfb, 0xf5,
        0x0c, 0x3c, 0x69, 0x86, 0x0a, 0x28, 0x07, 0xe3, 0x28, 0x27, 0x55, 0x07,
        0xa7, 0x35, 0x9f, 0x8b, 0x24, 0xae, 0xfc, 0xa4, 0xa0, 0xb0, 0x8e, 0x73,
        0x5d, 0x1b, 0x60, 0x9d, 0x2e, 0x28, 0x95, 0x36, 0xda, 0xd5, 0x58, 0x18,
        0xd7, 0x12, 0xd2, 0x33, 0x78, 0xc5, 0xa2, 0x2b, 0x82, 0x09, 0x91, 0x53,
        0x36, 0x46, 0xb7, 0x16, 0xf5, 0x05, 0x89, 0xfc, 0xa4, 0x29, 0xfa, 0x62,
        0x5f, 0xbf, 0xfc, 0xea, 0xc2, 0x5c, 0x99, 0x37, 0x57, 0x45, 0x4a, 0x4c,
        0x56, 0xd2, 0xf2, 0x00, 0x3b, 0x4b, 0x3c, 0x7a, 0x96, 0x5b, 0xf4, 0xd2,
        0x27, 0x95, 0x78, 0x1d, 0x95, 0xf0, 0xea, 0xc5, 0xa5, 0xb9, 0x62, 0x4c,
        0x21, 0x25, 0x58, 0xa3, 0x65, 0x0c, 0x4c, 0x2b, 0x65, 0x59, 0x34, 0x3a,
        0xb0, 0x8c, 0x84, 0x2c, 0xd6, 0x1a, 0x6f, 0xe4, 0x38, 0x57, 0x8b, 0xc7,
        0xbc, 0xdb, 0xbe, 0x85, 0x9a, 0x3e, 0x94, 0xfa, 0xe6, 0xfe, 0x98, 0xef,
        0x76, 0xa9, 0xde, 0x5d, 0x98, 0xfe, 0x76, 0xcf, 0x9a, 0xcf, 0x41, 0xeb,
        0x16, 0x58, 0xc9, 0xb6, 0x30, 0x9d, 0x6b, 0x63, 0xde, 0x99, 0xc2, 0x8c,
        0xf7, 0x4a, 0x4a, 0xeb, 0x5a, 0xcc, 0x6d, 0x58, 0xa1, 0x5a, 0xde, 0x8a,
        0x48, 0xa2, 0x05, 0xed, 0x83, 0x11, 0x2a, 0x72, 0xbc, 0x27, 0xf1, 0x6a,
        0xf5, 0x11, 0xa9, 0xe6, 0x25, 0xf6, 0x15, 0x31, 0xdd, 0xbf, 0x86, 0x96,
        0x67, 0x7b, 0x7b, 0xc0, 0x7c, 0x30, 0x56, 0xd8, 0x7f, 0xdd, 0x1f, 0xeb,
        0x1d, 0x73, 0xc6, 0x60, 0xd4, 0x17, 0x16, 0xd9, 0x9f, 0x5b, 0x14, 0x35,
        0x1c, 0xd8, 0x64, 0x60, 0xc9, 0xa1, 0x89, 0xd0, 0x09, 0xbf, 0x05, 0x9d,
        0x2c, 0xb3, 0xc9, 0x62, 0xcf, 0xa6, 0x88, 0x52, 0x3b, 0x9e, 0x30, 0xd4,
        0xdb, 0xd2, 0x44, 0xd2, 0x91, 0x29, 0x25, 0x39, 0x5c, 0x6d, 0x3c, 0x0b,
        0x42, 0x57, 0xa6, 0x62, 0x88, 0xdc, 0x48, 0xa7, 0xda, 0x80, 0x70, 0x0c,
        0xad, 0xd1, 0x21, 0xab, 0x6a, 0x61, 0xc5, 0x47, 0xf4, 0x31, 0x3a, 0x44,
        0xe6, 0xab, 0x10, 0xcc, 0x97, 0x62, 0x3d, 0x8f, 0x22, 0xf1, 0xc2, 0xc7,
        0xa1, 0x29, 0xe6, 0xd0, 0x5a, 0x56, 0x88, 0x98, 0xb0, 0x4c, 0x63, 0xdb,
        0x2c, 0x5a, 0x1e, 0x59, 0xad, 0x2a, 0xb6, 0x66, 0x9a, 0x44, 0x50, 0xc6,
        0xa1, 0x4e, 0x68, 0xe0, 0xa4, 0x2a, 0xa6, 0x0a, 0xa2, 0xa2, 0x45, 0xf1,
        0xcc, 0x27, 0x9e, 0x99, 0x95, 0x46, 0x36, 0x53, 0x43, 0x6c, 0xf9, 0x3c,
        0x14, 0xa6, 0x9a, 0xe8, 0x35, 0x2b, 0x25, 0xc0, 0x31, 0x0e, 0x2f, 0x79,
        0xd5, 0x04, 0x83, 0x53, 0x95, 0xd3, 0x3c, 0x97, 0x68, 0xc4, 0xe0, 0xe8,
        0xed, 0x76, 0x7f, 0x38, 0xed, 0xda, 0x2e, 0xc7, 0xd3, 0xee, 0xb0, 0x3f,
        0x6e, 0xb7, 0xda, 0x39, 0xe5, 0x2f, 0x84, 0x32, 0xf0, 0xda, 0x4a, 0x2c,
        0x95, 0x05, 0xa0, 0x15, 0xd3, 0x07, 0x20, 0xc5, 0x37, 0xcd, 0xaa, 0x93,
        0x5e, 0xcb, 0x56, 0x45, 0x8e, 0x23, 0x52, 0x6e, 0x3e, 0xdd, 0xc4, 0xdb,
        0x4b, 0x0d, 0xa9, 0x6e, 0x48, 0x84, 0xdc, 0x32, 0x93, 0xde, 0x22, 0x08,
        0x19, 0x33, 0xf9, 0x2c, 0x13, 0xcb, 0xd9, 0x66, 0xe8, 0x11, 0xa7, 0x93,
        0x1a, 0xad, 0x5d, 0x1e, 0x76, 0xfb, 0x97, 0xb0, 0x77, 0xbb, 0xd8, 0xed,
        0x31, 0x55, 0xbd, 0x0c, 0x39, 0xf4, 0xba, 0x50, 0xce, 0xc9, 0x31, 0x1b,
        0x32, 0x12, 0xa4, 0x08, 0xc5, 0x92, 0x16, 0x95, 0xa1, 0xee, 0x73, 0xe0,
        0x27, 0x3b, 0x20, 0x6a, 0x32, 0x75, 0x3b, 0xf6, 0xd6, 0x5f, 0xce, 0xaa,
        0x7f, 0x3e, 0xed, 0xda, 0x1b, 0xbd, 0x5c, 0x71, 0xc1, 0xd4, 0x1c, 0x90,
        0xd1, 0xcb, 0x45, 0x60, 0x7e, 0x8e, 0x86, 0x74, 0xb9, 0x59, 0x84, 0x8d,
        0x32, 0xf3, 0xf5, 0x4a, 0x4e, 0x11, 0xf6, 0xdc, 0x29, 0xbb, 0x96, 0x92,
        0x71, 0xb3, 0x42, 0x84, 0x37, 0x56, 0xb2, 0xb9, 0x9c, 0x6f, 0x98, 0xd9,
        0xac, 0xe4, 0x4a, 0x73, 0xbf, 0x31, 0x76, 0x3e, 0x0e, 0x5d, 0xad, 0xdd,
        0xc6, 0x8b, 0x4d, 0x60, 0xcb, 0x30, 0x37, 0xc0, 0xb3, 0x72, 0x2c, 0x58,
        0xb9, 0x62, 0x6e, 0x35, 0x0f, 0xc1, 0x1a, 0x67, 0xbc, 0x73, 0xe3, 0xd0,
        0xc0, 0x97, 0x81, 0xf3, 0xb9, 0xc2, 0x5c, 0x01, 0x68, 0xf4, 0x76, 0xc5,
        0x30, 0x46, 0xb2, 0xc5, 0xca, 0xad, 0xf8, 0x5a, 0x71, 0x2c, 0xb9, 0x9e,
        0x66, 0x05, 0xa1, 0x2b, 0xb5, 0x00, 0xa8, 0xcd, 0x82, 0x70, 0x83, 0xb5,
        0x31, 0x74, 0xc5, 0xd6, 0x4a, 0xdb, 0xb9, 0x5d, 0xf9, 0xb5, 0x38, 0x03,
        0x57, 0xaf, 0x97, 0x73, 0x13, 0xa4, 0x63, 0x6a, 0xed, 0x24, 0xb6, 0xa5,
        0x11, 0x09, 0x32, 0xc5, 0xad, 0x0d, 0x37, 0x76, 0x09, 0xe3, 0x86, 0xe2,
        0x88, 0xa1, 0x72, 0x1d, 0x8c, 0xd9, 0x2c, 0xe6, 0xb0, 0x64, 0x33, 0x07,
        0x5b, 0x08, 0x47, 0xdb, 0x12, 0x4c, 0x06, 0x2f, 0xdd, 0x42, 0xc9, 0xe5,
        0x62, 0x35, 0xcd, 0xaa, 0x82, 0x92, 0x26, 0x6c, 0x34, 0x13, 0xab, 0x95,
        0x66, 0x7a, 0xa5, 0x97, 0x6c, 0x6e, 0xe7, 0x73, 0xe6, 0xf5, 0x12, 0xf3,
        0xba, 0x95, 0x5c, 0xfb, 0xc9, 0xd6, 0xe2, 0x5d, 0xd1, 0x35, 0x15, 0x56,
        0x83, 0xc3, 0x50, 0xe5, 0x04, 0x38, 0x48, 0x02, 0xc2, 0x3e, 0x39, 0x9f,
        0x15, 0x32, 0xe0, 0x6c, 0x2b, 0xc4, 0x80, 0xb7, 0x1b, 0xb1, 0x60, 0x6a,
        0xa9, 0xc0, 0x59, 0xdc, 0x1a, 0x16, 0x5c, 0x08, 0x6c, 0xae, 0x36, 0x1b,
        0x29, 0x97, 0x6e, 0x81, 0x7d, 0x4d, 0x06, 0xd8, 0x35, 0xe8, 0x20, 0x2c,
        0xd8, 0x6a, 0xb3, 0x59, 0x21, 0x04, 0x72, 0x0d, 0x5b, 0xfd, 0x92, 0xad,
        0x37, 0x7e, 0xbd, 0x11, 0xcb, 0xb5, 0xd6, 0xcb, 0xe5, 0xe4, 0x57, 0xef,
        0x34, 0x32, 0x59, 0x33, 0x5e, 0x1c, 0x9c, 0x05, 0xd2, 0x63, 0x31, 0x35,
        0x04, 0x4f, 0x81, 0x84, 0x9c, 0x05, 0x62, 0xce, 0xf9, 0xb8, 0x94, 0xf0,
        0x94, 0x5f, 0xc1, 0x03, 0x2b, 0x8a, 0x56, 0x90, 0x96, 0x2d, 0xc8, 0x77,
        0xf3, 0xd5, 0x62, 0x81, 0x0e, 0x79, 0xb5, 0x90, 0x43, 0x0f, 0x8d, 0xa1,
        0x73, 0x25, 0xe0, 0x12, 0x61, 0x80, 0x1b, 0xc4, 0x41, 0xf3, 0x05, 0x02,
        0x85, 0x2a, 0xc7, 0xac, 0x58, 0x2c, 0xd7, 0x0b, 0x07, 0x8e, 0x83, 0xb3,
        0x3a, 0x0a, 0xdf, 0xdf, 0x6c, 0x63, 0x35, 0x4e, 0x58, 0xc5, 0x41, 0x2f,
        0x01, 0x3e, 0x30, 0xc9, 0xb3, 0x84, 0x6e, 0x85, 0x85, 0x64, 0x35, 0xe8,
        0x28, 0xbb, 0xec, 0xe3, 0xf6, 0xfe, 0xf6, 0x7a, 0xb7, 0xff, 0xe9, 0x02,
        0xfe, 0x6d, 0xf5, 0xda, 0x1b, 0x9f, 0x58, 0xe5, 0x2e, 0x32, 0x14, 0x96,
        0x02, 0x17, 0x79, 0xc7, 0x8c, 0x05, 0xb1, 0xc3, 0xb7, 0xde, 0xb4, 0xf1,
        0x8c, 0xe6, 0xf6, 0x6d, 0x89, 0x9f, 0x2e, 0xcc, 0xe6, 0x72, 0x53, 0x20,
        0xa4, 0xcc, 0x62, 0x4b, 0x70, 0x78, 0x54, 0x92, 0x45, 0x09, 0x3e, 0x6c,
        0xc6, 0xeb, 0x2c, 0x0a, 0x97, 0xb1, 0x85, 0x61, 0xb6, 0xe3, 0xee, 0xf8,
        0xe1, 0xf7, 0xad, 0x0f, 0xd1, 0xa5, 0x92, 0x9c, 0x88, 0x17, 0x0e, 0x94,
        0x9c, 0x8b, 0x32, 0x67, 0xd4, 0x56, 0xe7, 0x51, 0x6f, 0xb5, 0x08, 0x95,
        0x25, 0x74, 0xf0, 0x8c, 0x53, 0x45, 0x4b, 0x21, 0x9a, 0xc8, 0xfb, 0x81,
        0x92, 0xb5, 0x15, 0x24, 0x13, 0x2d, 0xcb, 0x56, 0x3b, 0x8c, 0xb3, 0x20,
        0x34, 0x62, 0x79, 0x67, 0xa3, 0xe4, 0x8d, 0x62, 0x3f, 0x6d, 0xed, 0xf6,
        0xfe, 0x2e, 0x7e, 0xd8, 0x95, 0x78, 0x61, 0x33, 0x60, 0x17, 0x9e, 0x6c,
        0x8d, 0xac, 0x98, 0x82, 0x18, 0x4a, 0x30, 0x8e, 0x2f, 0xd2, 0x20, 0x10,
        0x56, 0xe6, 0x5a, 0xc0, 0xee, 0xbc, 0x0e, 0x13, 0x2e, 0xe6, 0x3f, 0x5c,
        0x3a, 0xaf, 0x43, 0x59, 0x42, 0x0a, 0x45, 0x86, 0x6f, 0x4f, 0x7e, 0x07,
        0x99, 0xd7, 0x80, 0x6a, 0xe2, 0x92, 0xf6, 0x25, 0x29, 0x95, 0xca, 0xa8,
        0x17, 0xde, 0xac, 0x5f, 0xbe, 0x59, 0x7f, 0x7b, 0xc5, 0xae, 0x9e, 0xf3,
        0xcd, 0xf7, 0xe1, 0xa5, 0xba, 0xe0, 0x9d, 0x04, 0xc5, 0x0c, 0x1d, 0x12,
        0x19, 0x3a, 0x78, 0xd0, 0x38, 0x97, 0x02, 0x65, 0xd0, 0x35, 0x86, 0x42,
        0x67, 0x33, 0xd5, 0x3f, 0xf0, 0xef, 0x20, 0x73, 0x93, 0x51, 0xc5, 0x07,
        0xc6, 0x33, 0xb0, 0xd4, 0x45, 0x46, 0xb0, 0xc5, 0x62, 0x6f, 0x1e, 0x3c,
        0x2d, 0x6b, 0x46, 0x95, 0x1a, 0x03, 0x7f, 0xbd, 0xbb, 0xf9, 0x5d, 0xe3,
        0x0e, 0x86, 0x3e, 0x5d, 0x98, 0x0f, 0x82, 0x88, 0x03, 0x1e, 0x8d, 0x39,
        0x44, 0x9e, 0xe9, 0x8a, 0x3a, 0x1d, 0x85, 0x6f, 0x1d, 0xfe, 0x12, 0x06,
        0x83, 0x25, 0xc7, 0xc9, 0x7e, 0x38, 0xdc, 0x9f, 0xee, 0x53, 0xdd, 0x2e,
        0xbf, 0xdd, 0xfe, 0xf9, 0xcd, 0x2f, 0xa8, 0x36, 0xff, 0x58, 0xb6, 0xb9,
        0xf1, 0x74, 0xc3, 0x92, 0x9c, 0x65, 0x3f, 0x7c, 0xb5, 0x7e, 0xbd, 0x5c,
        0x5d, 0xaf, 0xee, 0xf8, 0xd4, 0xc8, 0x42, 0x58, 0xa2, 0xa9, 0x41, 0x74,
        0xde, 0xb9, 0xdd, 0x7e, 0x51, 0xde, 0x3d, 0xbf, 0x1a, 0x84, 0x33, 0x89,
        0x74, 0xa8, 0xe2, 0xae, 0x3b, 0x2d, 0xfb, 0xf4, 0x29, 0xee, 0x7f, 0x3c,
        0x85, 0x6f, 0xde, 0x2f, 0xba, 0x6c, 0xef, 0x9a, 0x94, 0x0e, 0x07, 0x81,
        0xb6, 0xef, 0xbf, 0xa9, 0xdf, 0x1d, 0x77, 0x6e, 0xf1, 0x4d, 0x19, 0xfb,
        0xa2, 0xa1, 0x63, 0xc6, 0x73, 0xcf, 0x9e, 0xdf, 0xef, 0xc3, 0xa7, 0x8f,
        0xf3, 0x20, 0xee, 0xfb, 0x72, 0x6a, 0x46, 0xed, 0x50, 0x57, 0xe7, 0x9c,
        0x9d, 0x3e, 0xe4, 0xba, 0xdb, 0xbd, 0x7f, 0xff, 0x82, 0x8f, 0xdd, 0x43,
        0xe8, 0x0d, 0x1a, 0x09, 0x66, 0xf6, 0xc7, 0xe5, 0x5b, 0xaa, 0xfc, 0xaf,
        0xfd, 0xa0, 0xe9, 0x49, 0x04, 0xf7, 0xb3, 0x59, 0xea, 0x5c, 0x58, 0x90,
        0xf3, 0xd5, 0xc7, 0xfc, 0xcd, 0x8b, 0xb0, 0x19, 0x7a, 0x82, 0xe1, 0x10,
        0xc5, 0x43, 0xf3, 0x32, 0xf1, 0x36, 0xbd, 0xbc, 0x7a, 0x21, 0xfe, 0xfc,
        0xea, 0xf6, 0x67, 0xb6, 0x18, 0xce, 0x6e, 0xd6, 0xf9, 0x90, 0x51, 0x44,
        0x5f, 0xed, 0xa7, 0x56, 0xa2, 0x77, 0x75, 0x86, 0x3a, 0x47, 0xcb, 0xc2,
        0x8f, 0xed, 0x5d, 0x7c, 0xbd, 0x7a, 0xf5, 0x95, 0x1e, 0x35, 0x36, 0xc9,
        0x77, 0xf4, 0xaf, 0xc8, 0xa9, 0xc3, 0x9f, 0xec, 0xdf, 0xfe, 0x72, 0x2d,
        0x9f, 0xeb, 0xbb, 0xb3, 0x63, 0xa8, 0xb3, 0xa3, 0xe6, 0x57, 0xb0, 0x97,
        0x57, 0xee, 0xeb, 0x1f, 0x6f, 0xe2, 0xfd, 0xfb, 0xd3, 0xf4, 0x90, 0x24,
        0x3e, 0x1d, 0xd3, 0x80, 0x58, 0xf9, 0x3f, 0xf4, 0x8b, 0xdb, 0x7f, 0xfc,
        0x4f, 0x79, 0xf9, 0xfc, 0x97, 0x96, 0xfc, 0xf1, 0xe3, 0xed, 0xfc, 0xfd,
        0xd5, 0xf3, 0xaf, 0x17, 0x7f, 0x1b, 0x1d, 0x40, 0xfd, 0x86, 0x34, 0xe8,
        0x39, 0x14, 0xab, 0xf6, 0xd5, 0x21, 0xbf, 0xff, 0x29, 0x8f, 0x9d, 0xe1,
        0xe3, 0x60, 0x3c, 0x2f, 0x6f, 0x77, 0xb7, 0x57, 0xdf, 0xbf, 0xfe, 0xf8,
        0xfd, 0x74, 0xfc, 0x44, 0x0d, 0x0b, 0x1c, 0xe0, 0xd9, 0x69, 0xf3, 0x63,
        0x8b, 0xe1, 0xbb, 0x1f, 0xf6, 0x7f, 0x39, 0x13, 0x73, 0x91, 0x99, 0xc7,
        0x58, 0xa8, 0x24, 0x15, 0x20, 0xab, 0x15, 0xc8, 0x46, 0x2b, 0x34, 0xcb,
        0xbe, 0x6a, 0x70, 0x0b, 0xb8, 0x46, 0xe6, 0xa7, 0x11, 0x73, 0xac, 0x4e,
        0xd7, 0xe2, 0x32, 0xa4, 0x5c, 0x45, 0x46, 0x29, 0xd4, 0xd9, 0xe0, 0x90,
        0x51, 0xc9, 0x35, 0x93, 0x52, 0x71, 0x1c, 0xd5, 0x6f, 0x58, 0xf6, 0x5d,
        0xbd, 0xbe, 0x3e, 0x6c, 0x3f, 0x1e, 0xee, 0xae, 0xcb, 0x85, 0x6c, 0x26,
        0xb1, 0xe3, 0x79, 0x40, 0x1d, 0xf2, 0x0c, 0x15, 0x04, 0x34, 0x56, 0x5a,
        0x64, 0xa0, 0x49, 0xb0, 0x4e, 0x0a, 0xda, 0x18, 0xf4, 0x43, 0xae, 0x0d,
        0xd0, 0x92, 0x48, 0x7b, 0x09, 0xf3, 0x33, 0x37, 0x1a, 0xfa, 0x11, 0x22,
        0x0e, 0x3a, 0xc3, 0x43, 0xa1, 0x40, 0xdf, 0x72, 0x9f, 0x8a, 0xb4, 0xa3,
        0xe0, 0x4a, 0x35, 0xfe, 0x74, 0xd8, 0x6f, 0x4f, 0xf5, 0x78, 0x29, 0xed,
        0x56, 0xf3, 0x97, 0xcf, 0xd7, 0x2f, 0x5e, 0xac, 0xcf, 0x79, 0x4a, 0x4a,
        0x6a, 0xab, 0x0d, 0xdf, 0xea, 0xad, 0xb7, 0x31, 0xa0, 0x73, 0x8b, 0xd5,
        0x1a, 0xd8, 0x60, 0x8d, 0x88, 0xf0, 0x98, 0xb1, 0x5e, 0x19, 0x28, 0x3b,
        0x7b, 0x89, 0x4b, 0x51, 0xf3, 0x94, 0x96, 0xa0, 0x1f, 0x21, 0x41, 0x4e,
        0xda, 0xa6, 0xcc, 0x52, 0x0e, 0x8e, 0xe5, 0x82, 0xaf, 0x94, 0xb4, 0xcc,
        0x51, 0x0d, 0xab, 0x5e, 0xbd, 0xfe, 0xd3, 0xa5, 0x1a, 0x16, 0x0c, 0xfa,
        0xbd, 0x9c, 0x98, 0xab, 0x55, 0x10, 0x39, 0xa0, 0x9a, 0xc9, 0xec, 0x98,
        0x03, 0xf9, 0x49, 0x14, 0x5e, 0xef, 0xe5, 0x48, 0xf4, 0xdf, 0xbd, 0xb8,
        0xc4, 0xcb, 0xc9, 0xa3, 0x41, 0x90, 0x51, 0x31, 0xa3, 0x38, 0x82, 0xe8,
        0xc1, 0x5a, 0x90, 0x44, 0x05, 0xe8, 0x00, 0x81, 0x79, 0x55, 0x01, 0x8b,
        0xa9, 0xba, 0xde, 0xa7, 0xfd, 0x7d, 0x62, 0xef, 0x4e, 0x37, 0xd7, 0x86,
        0x3d
    };
    struct uint8_block chunk_block1 = { 3085, gzip_chunk1 };

    uint8_t gzip_chunk2[] = {
        0xd4, 0xdb, 0xac, 0xd4, 0x9b, 0xc3, 0x25, 0xb1, 0xad, 0xb3, 0x55, 0x06,
        0x54, 0xe6, 0x92, 0x81, 0x82, 0xcd, 0xd0, 0x26, 0x29, 0x89, 0xcc, 0x04,
        0x00, 0x94, 0x90, 0xda, 0x51, 0xb9, 0x32, 0xf6, 0xa6, 0x11, 0x9d, 0x9a,
        0xc5, 0x96, 0x84, 0x47, 0x29, 0xd3, 0x15, 0x64, 0x1d, 0xa0, 0x21, 0xf0,
        0x1b, 0xba, 0x80, 0x14, 0x6a, 0xa5, 0xb6, 0xe3, 0x42, 0x6f, 0xfa, 0x94,
        0x29, 0xfa, 0x62, 0xdf, 0x2e, 0xdf, 0x5c, 0x8a, 0x9b, 0x2d, 0xc5, 0x54,
        0xc8, 0x5d, 0xdb, 0x38, 0x24, 0x2a, 0xaa, 0x09, 0x4b, 0x15, 0x9d, 0x0b,
        0x74, 0x44, 0xd6, 0xb1, 0xa1, 0x04, 0x97, 0x31, 0x6e, 0xf3, 0xd7, 0x97,
        0x7a, 0x53, 0xd4, 0x3b, 0xd4, 0xc0, 0x0c, 0xc5, 0x61, 0x8b, 0x1a, 0x90,
        0x1d, 0x79, 0x40, 0xf0, 0xe0, 0xe6, 0xd2, 0x0a, 0x5c, 0x21, 0xc6, 0x7a,
        0x3a, 0x56, 0x9f, 0xed, 0xe9, 0xe3, 0xee, 0x74, 0x3a, 0xfe, 0x96, 0xb5,
        0xe7, 0x57, 0x3f, 0x4b, 0xba, 0x90, 0x98, 0x74, 0x2a, 0xe2, 0x75, 0xe7,
        0xa3, 0xf1, 0x40, 0xb6, 0x9f, 0x9c, 0x58, 0x3a, 0xf8, 0xf6, 0xa3, 0x07,
        0x04, 0xa7, 0xca, 0xf6, 0x14, 0x38, 0xd8, 0xa6, 0xb5, 0xae, 0x40, 0xb0,
        0x42, 0x28, 0xa0, 0x0e, 0xa1, 0xc7, 0x42, 0x05, 0x93, 0x18, 0x0e, 0x95,
        0xe0, 0x05, 0x12, 0xc2, 0xfe, 0xd2, 0x67, 0x5f, 0x5f, 0xd5, 0xd3, 0x76,
        0xbe, 0x8f, 0xd7, 0x9f, 0x4e, 0xbb, 0xfc, 0xa5, 0x57, 0xd4, 0xe3, 0x7e,
        0x6c, 0x3c, 0xd8, 0xb1, 0xfd, 0x94, 0x97, 0x09, 0x8e, 0xff, 0x94, 0x06,
        0x4d, 0x4b, 0x6e, 0xdd, 0x70, 0xae, 0xf5, 0x70, 0x07, 0xc3, 0x00, 0xe4,
        0x4f, 0x30, 0x0a, 0xbd, 0xb1, 0xd2, 0x0f, 0x0f, 0x87, 0x1e, 0xce, 0x21,
        0xd1, 0xca, 0xa1, 0x6d, 0x46, 0x01, 0xb6, 0x8f, 0x59, 0x42, 0xe0, 0xab,
        0x88, 0xea, 0x92, 0xcb, 0x09, 0x3f, 0x82, 0x72, 0xb2, 0xe5, 0x14, 0x23,
        0x1a, 0x14, 0x5c, 0xba, 0xc0, 0xc1, 0xb5, 0x97, 0xdc, 0x92, 0x0b, 0x3a,
        0x52, 0x14, 0x3c, 0x44, 0x13, 0x61, 0x04, 0x41, 0xb0, 0xd0, 0x84, 0x67,
        0x50, 0x78, 0x36, 0x62, 0xc9, 0xec, 0x8b, 0xff, 0xcf, 0x14, 0x76, 0xd9,
        0xff, 0x22, 0x91, 0xcb, 0x9f, 0x11, 0xf9, 0xe5, 0x99, 0xba, 0xc0, 0x7c,
        0xb2, 0x12, 0x7f, 0x8b, 0x40, 0x8d, 0x1f, 0x00, 0x6f, 0x8f, 0xf1, 0x43,
        0xbd, 0x10, 0x2b, 0x20, 0xd8, 0xa0, 0xbf, 0xab, 0xde, 0x63, 0x5a, 0xd2,
        0x25, 0x3a, 0x01, 0xbc, 0x49, 0xa0, 0xd9, 0x97, 0xda, 0x8b, 0x24, 0x43,
        0x88, 0x16, 0x82, 0x67, 0x3c, 0x6d, 0x40, 0x73, 0x6e, 0x24, 0x7a, 0xd1,
        0x90, 0x49, 0xc2, 0xa3, 0xd0, 0xa0, 0x40, 0x10, 0xf1, 0xdb, 0x9c, 0x81,
        0xf7, 0x66, 0x78, 0x9b, 0xda, 0xab, 0x26, 0x9a, 0x76, 0xbc, 0xe0, 0x59,
        0xc8, 0xd8, 0x99, 0x27, 0xc1, 0x69, 0x51, 0x32, 0x5b, 0xb2, 0x2d, 0x73,
        0x07, 0x1c, 0x88, 0xc9, 0xe2, 0xeb, 0x43, 0xba, 0xf8, 0xd1, 0xee, 0x93,
        0x88, 0xe5, 0xa9, 0xe7, 0x6b, 0x43, 0x0b, 0xf0, 0xb1, 0x1e, 0x0f, 0x37,
        0x75, 0xf9, 0x2b, 0x3d, 0xbb, 0xfa, 0xa2, 0x67, 0x07, 0x89, 0x7a, 0x60,
        0x2a, 0xb3, 0x12, 0x3c, 0x65, 0x7a, 0xae, 0x70, 0x03, 0x8a, 0x60, 0x45,
        0x57, 0xed, 0x3d, 0x14, 0xbb, 0x2d, 0x53, 0x6b, 0x07, 0x45, 0x5d, 0x25,
        0xb2, 0x8c, 0x29, 0xa8, 0x62, 0x48, 0xee, 0x02, 0x6a, 0xcf, 0xa6, 0x9f,
        0xf5, 0xc8, 0x24, 0x82, 0x12, 0x31, 0x4c, 0x8d, 0xb8, 0x00, 0xc2, 0xb2,
        0xe1, 0x91, 0x79, 0x53, 0xe8, 0xf0, 0x02, 0x12, 0x3d, 0x28, 0xe3, 0x98,
        0x8d, 0xf4, 0x89, 0x75, 0x74, 0xd9, 0xb8, 0xa9, 0x0d, 0x45, 0x17, 0x20,
        0x32, 0x3a, 0x0d, 0xa6, 0x5c, 0x44, 0xea, 0x36, 0x70, 0xa1, 0xe7, 0xd5,
        0x31, 0x89, 0x56, 0xb3, 0x39, 0xc0, 0x46, 0xeb, 0x38, 0x1d, 0xe0, 0xe0,
        0x45, 0x9f, 0xd0, 0xf8, 0x06, 0x81, 0xb5, 0x75, 0x4c, 0x34, 0xbf, 0x4d,
        0x68, 0xbd, 0x6a, 0x43, 0xb6, 0x23, 0x57, 0xea, 0x34, 0xab, 0x10, 0xd1,
        0xb6, 0x86, 0x66, 0x35, 0xf2, 0xca, 0xa9, 0x11, 0x47, 0xf8, 0x7d, 0xaa,
        0x4c, 0xa6, 0x28, 0x42, 0xe0, 0x06, 0x99, 0x94, 0xa6, 0xf6, 0xde, 0x28,
        0x99, 0x0d, 0xa0, 0xa1, 0xb2, 0xc6, 0xd0, 0x00, 0x37, 0x47, 0x8d, 0xc0,
        0x5a, 0x0e, 0x46, 0x36, 0x60, 0x90, 0x70, 0x3e, 0xed, 0xca, 0xa6, 0xd8,
        0x16, 0xa3, 0x63, 0x1e, 0x65, 0x11, 0xe1, 0x07, 0xb2, 0x3c, 0x76, 0x8f,
        0x1f, 0x55, 0x02, 0xaf, 0x74, 0x30, 0x63, 0xa6, 0xee, 0x1a, 0xe1, 0x6a,
        0xb9, 0x24, 0x66, 0x2d, 0xa7, 0xbe, 0xd0, 0x73, 0x20, 0x05, 0x6e, 0xe3,
        0xe8, 0x0a, 0x5d, 0xa9, 0x16, 0x89, 0xdf, 0xce, 0x27, 0x01, 0x29, 0x55,
        0x67, 0x50, 0x12, 0x2c, 0x41, 0x15, 0x69, 0xce, 0x7c, 0x70, 0x16, 0xa5,
        0x2d, 0xcb, 0xc4, 0x41, 0x82, 0xd9, 0x4f, 0xdb, 0xaa, 0x36, 0xe6, 0xcc,
        0xbd, 0x65, 0x12, 0xf7, 0x89, 0xe7, 0x24, 0xf3, 0xbe, 0x7a, 0x86, 0x36,
        0x5a, 0x54, 0x5b, 0x8b, 0xe6, 0x22, 0x9d, 0x8f, 0xdb, 0x2c, 0x74, 0xbd,
        0x93, 0x30, 0x9a, 0xfa, 0x93, 0x1c, 0x1a, 0x15, 0x1a, 0x04, 0xaf, 0xa6,
        0xa4, 0xa4, 0x43, 0xd7, 0x62, 0xd5, 0x79, 0x5b, 0x9e, 0x5b, 0x0f, 0x3f,
        0x05, 0x90, 0x0b, 0x42, 0x50, 0xd1, 0x97, 0x69, 0x90, 0x69, 0x0e, 0x95,
        0x67, 0xb4, 0x53, 0xc1, 0x37, 0x31, 0x39, 0x2b, 0x35, 0xe8, 0x3c, 0x84,
        0x47, 0xc1, 0xe7, 0x4c, 0x03, 0x97, 0xd0, 0xcb, 0xd8, 0x60, 0xaa, 0xde,
        0xe6, 0x88, 0x86, 0x52, 0x27, 0x3d, 0x0e, 0x8d, 0x5c, 0xc7, 0xe0, 0x21,
        0x1a, 0x15, 0x77, 0xf4, 0xf7, 0x08, 0xb0, 0x3a, 0x04, 0x74, 0xc6, 0x3e,
        0xe9, 0x12, 0x73, 0x6c, 0xaa, 0xc8, 0x69, 0x28, 0x50, 0x8c, 0xac, 0x4a,
        0x81, 0x59, 0xa5, 0xe8, 0xc8, 0xd2, 0x91, 0x4b, 0xa9, 0xa1, 0x82, 0xc2,
        0x29, 0x05, 0xd1, 0x72, 0xee, 0x7c, 0xc2, 0x12, 0x45, 0x4b, 0x0d, 0x4a,
        0xb3, 0x68, 0x87, 0xa1, 0x41, 0x01, 0xaf, 0x5a, 0x2b, 0x86, 0xb2, 0x9f,
        0x5b, 0xd0, 0x16, 0x02, 0x63, 0x1a, 0x9a, 0xb2, 0x12, 0x92, 0xe8, 0x0e,
        0xba, 0x11, 0x2d, 0xa2, 0x22, 0x0f, 0xa0, 0x3b, 0x64, 0xf0, 0x9f, 0x53,
        0x39, 0x35, 0x6b, 0xc5, 0x94, 0xd7, 0x5c, 0x00, 0x1c, 0xd8, 0x1a, 0xb6,
        0x45, 0x07, 0x47, 0x2a, 0x51, 0x08, 0xa0, 0x5f, 0x6c, 0xe4, 0xce, 0x42,
        0x58, 0x14, 0x5e, 0xa6, 0x53, 0x0b, 0xed, 0xb2, 0x8a, 0x60, 0x70, 0xd4,
        0x16, 0xea, 0x9e, 0x2d, 0xe2, 0xe6, 0x05, 0xe2, 0x90, 0xe1, 0xef, 0x2a,
        0xab, 0x2a, 0xe8, 0x9f, 0xa7, 0x63, 0x93, 0x92, 0x25, 0x3a, 0xd3, 0xc2,
        0x60, 0x05, 0xb2, 0xa0, 0x62, 0x47, 0x48, 0x0b, 0x08, 0x35, 0xa1, 0x21,
        0xda, 0x40, 0x7d, 0x3a, 0x4c, 0x70, 0x69, 0xc5, 0xea, 0x14, 0x29, 0xef,
        0x04, 0x1d, 0x63, 0x42, 0x98, 0x23, 0x0b, 0x02, 0x66, 0xf5, 0x20, 0x69,
        0x8e, 0xbc, 0x03, 0x98, 0xa6, 0x2c, 0x70, 0xa6, 0xc9, 0xe2, 0xc1, 0x3c,
        0x06, 0x7a, 0x0f, 0xf3, 0x03, 0x59, 0xcd, 0x58, 0x06, 0x58, 0x01, 0x58,
        0x52, 0x26, 0xcd, 0xcf, 0xb3, 0x7a, 0xfa, 0x6b, 0x16, 0x18, 0x80, 0xa6,
        0x14, 0xb3, 0x72, 0x47, 0x78, 0x45, 0x4f, 0x95, 0x7d, 0x8e, 0xc8, 0x39,
        0x5e, 0xc2, 0xd9, 0x00, 0x64, 0x48, 0x04, 0xb1, 0x61, 0x47, 0xb1, 0x21,
        0xb0, 0xc8, 0x11, 0x64, 0x59, 0xa2, 0x97, 0x6c, 0x41, 0xeb, 0xef, 0xa3,
        0x3c, 0xf3, 0x80, 0x93, 0x4a, 0xe5, 0xd8, 0x23, 0x69, 0xe9, 0x74, 0xd6,
        0x0a, 0x02, 0x61, 0x65, 0x19, 0x4a, 0xb1, 0xa9, 0xe4, 0x4a, 0x4a, 0x93,
        0x07, 0xb4, 0x6f, 0xc0, 0x4f, 0x24, 0x95, 0xaa, 0x2b, 0xa9, 0x54, 0x49,
        0x69, 0xd8, 0xcf, 0xd1, 0x95, 0x29, 0xce, 0x72, 0xdb, 0x26, 0xca, 0x30,
        0xb9, 0xd5, 0x10, 0xb3, 0x61, 0x3d, 0x64, 0xa0, 0x34, 0xc8, 0x1a, 0xd4,
        0x0b, 0x66, 0x43, 0x45, 0x99, 0x85, 0x02, 0x4f, 0x65, 0x0a, 0x6c, 0x74,
        0x86, 0x57, 0x63, 0xe9, 0xd8, 0x82, 0xc8, 0xd2, 0x70, 0x22, 0x6c, 0x10,
        0x11, 0x80, 0x1b, 0x2d, 0x76, 0x09, 0x29, 0x3b, 0x19, 0xd0, 0x2c, 0xd2,
        0xd7, 0x70, 0x0f, 0xb9, 0x45, 0xe7, 0x61, 0x02, 0xb9, 0x92, 0x88, 0x88,
        0x8c, 0x53, 0x2d, 0x23, 0x08, 0x5e, 0xa6, 0x7a, 0xf6, 0x6b, 0x90, 0x88,
        0x39, 0xc8, 0x98, 0xce, 0xb3, 0x81, 0x57, 0x90, 0x07, 0xc0, 0xc4, 0x38,
        0xf0, 0x16, 0xac, 0xca, 0x90, 0xe8, 0xe7, 0x32, 0xa0, 0x8b, 0xe4, 0x09,
        0x65, 0xcd, 0xa4, 0x06, 0xb8, 0x24, 0xb0, 0x11, 0xc0, 0x06, 0xbc, 0x36,
        0x6c, 0x51, 0x14, 0x11, 0xaa, 0x9e, 0xe0, 0xa2, 0xa0, 0x83, 0x9d, 0x10,
        0x8e, 0x35, 0x89, 0x04, 0xd0, 0x06, 0xf2, 0x2c, 0xc9, 0x08, 0x37, 0x38,
        0xeb, 0x2c, 0x79, 0x2a, 0xba, 0x29, 0xb7, 0x82, 0x6d, 0x56, 0x55, 0xb2,
        0xd5, 0x85, 0x44, 0x94, 0x91, 0x08, 0x59, 0xe0, 0x25, 0x9b, 0x40, 0x4f,
        0x5c, 0x3b, 0x35, 0x9d, 0x28, 0x7c, 0x79, 0x3e, 0x8d, 0x74, 0xb9, 0xa8,
        0x62, 0xab, 0x01, 0x3f, 0x04, 0x58, 0xc9, 0x15, 0xb1, 0x67, 0x44, 0xd8,
        0x04, 0x32, 0x07, 0xac, 0x01, 0x78, 0x27, 0x67, 0xb2, 0xfb, 0x2c, 0x06,
        0x12, 0x7a, 0x42, 0x93, 0x43, 0x64, 0xcd, 0x45, 0x40, 0x37, 0x20, 0x21,
        0x88, 0xe2, 0x59, 0xb6, 0xd6, 0x69, 0xb0, 0x95, 0x80, 0x04, 0xf9, 0x35,
        0x31, 0xf0, 0xff, 0xed, 0xea, 0x16, 0x6f, 0x2e, 0x09, 0x5e, 0x6e, 0x60,
        0x0d, 0x92, 0x8a, 0x90, 0xe7, 0xe9, 0x73, 0x81, 0x04, 0x67, 0x16, 0x89,
        0xf2, 0x8b, 0xc2, 0x28, 0x53, 0x6b, 0x69, 0x92, 0xa5, 0xb7, 0xc7, 0x9b,
        0xc3, 0xed, 0xf1, 0x09, 0x07, 0x23, 0x29, 0x3a, 0x1e, 0x89, 0x96, 0x9c,
        0x48, 0x94, 0xce, 0x28, 0x44, 0x5e, 0x20, 0xf1, 0x44, 0x8a, 0xca, 0x9b,
        0x28, 0x92, 0x31, 0x63, 0x45, 0x2f, 0xf5, 0xc3, 0x4c, 0x44, 0x34, 0x56,
        0x1c, 0x10, 0x16, 0x88, 0x87, 0x16, 0x28, 0x9c, 0xbe, 0xe8, 0xec, 0x8a,
        0x37, 0x60, 0x8f, 0x1c, 0xcd, 0x85, 0xc5, 0xa0, 0x49, 0x8c, 0xd5, 0x68,
        0xeb, 0x61, 0xab, 0x21, 0x9a, 0x01, 0xc6, 0x51, 0x6d, 0x59, 0x43, 0x95,
        0x44, 0x63, 0x14, 0xd1, 0x47, 0x97, 0xc7, 0x87, 0x56, 0xdf, 0x3a, 0xb5,
        0xe1, 0x8b, 0xc5, 0xa5, 0xd8, 0x3e, 0xfd, 0x30, 0xea, 0xc1, 0x87, 0x05,
        0xa7, 0xd3, 0x5d, 0xfd, 0xfb, 0x6f, 0xf9, 0x61, 0x01, 0xd2, 0xdf, 0x45,
        0xd4, 0x3d, 0xec, 0xa4, 0xf0, 0xea, 0xb9, 0xf2, 0x1e, 0xe8, 0xf7, 0x97,
        0x54, 0x2a, 0x34, 0x84, 0x35, 0x30, 0x9c, 0x3e, 0xc2, 0xa1, 0xe3, 0x6d,
        0x8e, 0x5e, 0x4e, 0x5a, 0xf4, 0x32, 0x0e, 0x22, 0x48, 0x9a, 0x54, 0xed,
        0xd4, 0x12, 0x3d, 0x15, 0x98, 0xbf, 0xf1, 0x71, 0xc3, 0xfd, 0xbe, 0xd4,
        0x86, 0x96, 0xa6, 0xfc, 0xae, 0xa7, 0x78, 0x4f
    };
    struct uint8_block chunk_block2 = { 1484, gzip_chunk2 };

    uint8_t gzip_chunk3[] = {
        0x3d, 0x4b, 0x79, 0x92, 0x04, 0x7f, 0xca, 0x4c, 0x44, 0x3b, 0x4f, 0x74,
        0x02, 0xd9, 0x77, 0x3a, 0x9c, 0xe2, 0xf5, 0xf6, 0xc1, 0x5f, 0x5a, 0x1a,
        0x75, 0xbe, 0xfb, 0xc8, 0x10, 0x2f, 0xfb, 0xa9, 0x67, 0xbd, 0xfb, 0xb0,
        0xeb, 0x1f, 0x46, 0x3d, 0x7b, 0x75, 0x57, 0x8f, 0x95, 0x3e, 0x98, 0xfa,
        0xf7, 0xff, 0x01, 0xa9, 0xdf, 0x53, 0x30, 0xd6, 0x29, 0x00, 0x00
    };
    struct uint8_block chunk_block3 = { 59, gzip_chunk3 };

    pubnub_init(pbp, "demo", "demo");
    pubnub_set_user_id(pbp, "test_id");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url("/v2/presence/sub-key/demo?pnsdk=unit-test-0.1&uuid=test_id");
    incoming("HTTP/1.1 200\r\n"
             "Content-Length: 197\r\n"
             "Content-Encoding: gzip\r\n"
             "\r\n",
             &body_block);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_global_here_now(pbp), equals(PNR_OK));

    attest(pubnub_get(pbp),
           streqs("{\"status\":200,\"message\":\"OK\",\"service\":\"Presence\","
                  "\"payload\":{channels:{\"ch1\":{\"uuids\":[uuid1,uuid2,"
                  "uuid3],\"occupancy\":3},\"ch2\":{\"uuids\":[uuid3,uuid4],"
                  "\"occupancy\":2},\"ch3\":{\"uuids\":[uuid7],\"occupancy\":1}"
                  ",\"ch4\":{\"uuids\":[],\"occupancy\":0},\"ch5\":{\"uuids\":["
                  "tricycle, mother, swоrd], "
                  "\"occupancy\":3}},\"total_channels\":5,\"total_occupancy\":"
                  "12}}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));

    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/v2/presence/sub-key/demo?pnsdk=unit-test-0.1&uuid=test_id");
    incoming("HTTP/1.1 200\r\n"
             "Content-Encoding: gzip\r\n"
             "Transfer-Encoding: chunked\r\n"
             "\r\n"
             "c0d\r\n",
             &chunk_block1);
    incoming("\r\n5cc\r\n", &chunk_block2);
    incoming("\r\n3b\r\n", &chunk_block3);
    incoming("\r\n0\r\n", NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_global_here_now(pbp), equals(PNR_STARTED));
    attest(pubnub_global_here_now(pbp), equals(PNR_IN_PROGRESS));

    /* 'push' until finished */
    attest(pbnc_fsm(pbp), equals(0));
    attest(pbnc_fsm(pbp), equals(0));
    attest(pbnc_fsm(pbp), equals(0));
    attest(pbp->core.last_result, equals(PNR_OK));

    attest(
        pubnub_get(pbp),
        streqs(
            "{\"status\": 200, \"message\": \"OK\", \"payload\": "
            "{\"channels\": {\"ripan-hallonet-sensors\": {\"occupancy\": 1, "
            "\"uuids\": [\"872edf50-bc2e-4b8a-afa1-9b59337a5938\"]}, "
            "\"Proxy_Check_Online_3\": {\"occupancy\": 1, \"uuids\": "
            "[\"27.74.130.107\"]}, \"adminportal\": {\"occupancy\": 2, "
            "\"uuids\": [\"d20f27b1-e66f-4247-a9b4-264432e86a6b\", "
            "\"10e68d24-0243-484c-8960-2745cbe1e4cb\"]}, "
            "\"Shield_Proxy_Check_Online2\": {\"occupancy\": 150, \"uuids\": "
            "[\"124.105.121.168\", \"110.54.237.164\", \"49.147.193.93\", "
            "\"113.161.55.100\", \"46.36.65.210\", \"61.230.8.134\", "
            "\"112.203.148.75\", \"120.29.109.119\", \"36.236.21.126\", "
            "\"190.128.164.182\", \"27.247.130.198\", \"180.191.87.114\", "
            "\"37.186.42.64\", \"119.93.3.144\", \"116.98.166.96\", "
            "\"36.233.101.191\", \"203.223.190.120\", \"94.59.105.19\", "
            "\"58.186.197.134\", \"116.103.108.180\", \"157.42.142.181\", "
            "\"122.3.39.211\", \"49.148.82.61\", \"175.212.79.101\", "
            "\"106.1.39.214\", \"122.116.225.76\", \"112.200.206.156\", "
            "\"195.74.224.132\", \"192.228.175.13\", \"112.198.103.175\", "
            "\"119.93.251.171\", \"84.95.232.171\", \"103.5.1.130\", "
            "\"49.145.111.128\", \"88.152.11.108\", \"122.118.113.171\", "
            "\"115.75.176.149\", \"61.58.185.188\", \"36.230.35.157\", "
            "\"116.73.196.128\", \"1.168.235.43\", \"112.204.192.160\", "
            "\"122.2.2.82\", \"110.54.236.21\", \"89.44.219.209\", "
            "\"124.218.3.139\", \"175.158.216.8\", \"49.150.144.4\", "
            "\"24.55.29.95\", \"175.138.51.98\", \"14.191.38.227\", "
            "\"141.237.206.22\", \"210.14.20.141\", \"80.195.130.215\", "
            "\"124.105.59.59\", \"112.210.170.50\", \"42.113.115.85\", "
            "\"49.159.60.50\", \"117.5.143.136\", \"74.96.121.172\", "
            "\"112.198.76.48\", \"183.88.2.123\", \"188.51.32.230\", "
            "\"111.246.152.41\", \"36.239.5.235\", \"60.50.141.170\", "
            "\"37.210.221.192\", \"103.107.253.21\", \"60.54.102.90\", "
            "\"36.77.153.130\", \"123.19.132.135\", \"89.210.97.166\", "
            "\"103.100.137.14\", \"2.50.134.250\", \"20.20.20.12\", "
            "\"117.2.85.215\", \"42.113.199.181\", \"2.51.120.235\", "
            "\"125.230.143.195\", \"111.249.142.245\", \"91.154.147.79\", "
            "\"49.149.79.61\", \"204.210.112.99\", \"115.73.204.227\", "
            "\"42.61.247.152\", \"218.111.17.141\", \"175.140.218.141\", "
            "\"49.144.204.206\", \"120.29.112.54\", \"171.98.135.43\", "
            "\"113.160.148.136\", \"14.167.123.54\", \"220.136.166.148\", "
            "\"115.78.94.12\", \"80.69.52.8\", \"113.174.79.114\", "
            "\"31.167.28.6\", \"37.104.170.14\", \"113.190.58.53\", "
            "\"175.180.108.173\", \"1.52.33.98\", \"180.190.183.65\", "
            "\"203.177.67.222\", \"14.231.46.148\", \"77.31.208.165\", "
            "\"58.187.117.235\", \"60.50.64.155\", \"178.59.63.21\", "
            "\"119.94.246.152\", \"113.188.244.76\", \"77.69.247.229\", "
            "\"219.74.81.16\", \"88.69.184.178\", \"122.2.248.94\", "
            "\"83.110.227.99\", \"112.198.243.105\", \"121.54.54.43\", "
            "\"1.53.33.124\", \"180.191.130.13\", \"180.190.78.41\", "
            "\"124.107.126.101\", \"42.119.183.166\", \"123.26.62.36\", "
            "\"14.176.202.239\", \"37.106.87.156\", \"180.191.119.71\", "
            "\"95.235.77.103\", \"31.146.212.78\", \"14.174.109.180\", "
            "\"51.218.186.86\", \"45.46.5.49\", \"60.50.30.176\", "
            "\"188.50.236.115\", \"27.76.199.154\", \"2.49.41.238\", "
            "\"110.54.242.122\", \"49.145.103.189\", \"14.173.101.131\", "
            "\"101.51.135.82\", \"78.100.181.211\", \"125.161.130.53\", "
            "\"120.29.79.44\", \"36.234.215.106\", \"94.200.33.34\", "
            "\"14.229.179.29\", \"111.253.91.112\", \"113.254.192.231\", "
            "\"78.93.246.65\", \"112.206.68.198\", \"94.77.228.34\"]}, "
            "\"34547ead-0423-4983-a7d1-11cbc554ffa4\": {\"occupancy\": 1, "
            "\"uuids\": [\"34547ead-0423-4983-a7d1-11cbc554ffa4\"]}, \"HNG\": "
            "{\"occupancy\": 1, \"uuids\": "
            "[\"c0f7e1bb-2e2e-4105-b0a8-cfa828b3b0eb\"]}, \"DLG\": "
            "{\"occupancy\": 1, \"uuids\": "
            "[\"aab9bb83-42a9-4336-a549-c219d665852b\"]}, "
            "\"fasci_geckodeSuscriber\": {\"occupancy\": 1, \"uuids\": "
            "[\"pn-f8c944f9-dc6d-4cef-875d-58832267facf\"]}, "
            "\"e60fd1b1f9489513a044f24f9e8a166820ebaf8c|sensor_pod-454916|user-"
            "755668\": {\"occupancy\": 6, \"uuids\": "
            "[\"pn-a40f7f29-b74c-4b29-94b6-6b69d65d1de9\", "
            "\"pn-86df1b4a-3320-4158-914e-3a9a05273f82\", "
            "\"pn-ea7115ed-d8af-449a-8e11-8dd680a1b0d0\", "
            "\"pn-bac9ffc3-cf16-460f-a60a-ee3aff5f2ab9\", "
            "\"pn-714a7de3-3d49-41d8-8b0c-6252f5e9afc9\", "
            "\"pn-7df15a84-dd9d-47e3-83f1-0fd3740cda51\"]}, "
            "\"__notifications__47738\": {\"occupancy\": 1, \"uuids\": "
            "[\"90efdade-954f-4796-a8f4-e72842fe1cab\"]}, \"mymaps\": "
            "{\"occupancy\": 1, \"uuids\": "
            "[\"4f423cfc-286c-4cf4-8c2b-cc6c25074b31\"]}, "
            "\"CoinNoti_Binance\": {\"occupancy\": 1, \"uuids\": "
            "[\"pn-72e185b7-69ce-4d13-b41e-2040f8cc79e8\"]}, \"my_channel\": "
            "{\"occupancy\": 14, \"uuids\": "
            "[\"pn-E854CD01-3A29-4CB9-8A24-CFB9F35AED20\", "
            "\"pn-80736E22-05D6-4F62-A2AF-5FD2D408F56A\", "
            "\"pn-DE7F81F9-C9A5-4537-962D-7DA996575877\", "
            "\"pn-90C900A3-A290-486D-A992-BD7D0E3036EE\", "
            "\"pn-D15433B6-65B9-41AF-A99D-E346A6D8E182\", "
            "\"pn-4ECA5927-3E72-4C44-89A5-7E5056CF8113\", "
            "\"pn-2E955FBA-90FA-4317-A2A1-29827B32CBD2\", "
            "\"pn-393259F4-1DD4-4D4C-A6AA-84CE507D2E8E\", "
            "\"pn-d87d4ebd-e974-4371-a52d-48b78c314a82\", "
            "\"pn-17886F1B-3C33-4065-9799-A3FF22C7BE18\", "
            "\"pn-36E5669B-DFFD-4F2E-A28C-EF8EF1CE44CC\", "
            "\"pn-98743f84-0d79-4585-abf2-0313a768ccb9\", "
            "\"pn-C29278DA-9DA5-4926-B927-ADBB223DB296\", "
            "\"pn-A31B3215-E8B6-40B2-B120-61BCEB7489D2\"]}, "
            "\"jm_ae571630-9194-45b8-b159-9b64de9c7c8a_uplink\": "
            "{\"occupancy\": 1, \"uuids\": "
            "[\"6e84858b-e07a-4ffd-9787-56a5437185f7\"]}, \"pgday\": "
            "{\"occupancy\": 1, \"uuids\": "
            "[\"7cf35f2c-afb3-4a32-a24a-f584c1d02af9\"]}, "
            "\"sisvi_geckodeSuscriber\": {\"occupancy\": 1, \"uuids\": "
            "[\"pn-f8c944f9-dc6d-4cef-875d-58832267facf\"]}, \"a7bdb71a\": "
            "{\"occupancy\": 2, \"uuids\": "
            "[\"77a2ccd1-78ad-419e-b251-019d6b9a5a0b\", "
            "\"66e4b3a6-c647-4166-a320-76a20f0313f9\"]}, \"puravida\": "
            "{\"occupancy\": 1, \"uuids\": "
            "[\"0400b6ea-d5d4-421e-8d25-e062cedee30e\"]}, \"BAY\": "
            "{\"occupancy\": 1, \"uuids\": "
            "[\"84b2393a-93a8-4a33-8e91-87b48db33bd4\"]}, "
            "\"SENSEMO-OI0FX9N3\": {\"occupancy\": 2, \"uuids\": "
            "[\"b227eada-216f-4021-b77f-6df6c69d6c6c\", "
            "\"42b53d89-0c8b-4105-96d6-d588c22ecf2a\"]}, "
            "\"slim_geckodeSuscriber\": {\"occupancy\": 1, \"uuids\": "
            "[\"pn-f8c944f9-dc6d-4cef-875d-58832267facf\"]}, \"chat\": "
            "{\"occupancy\": 1, \"uuids\": "
            "[\"f7e0a7bf-7ccd-4e58-a18f-4ffd288c992f\"]}, \"Youtube_CM_VS4\": "
            "{\"occupancy\": 18, \"uuids\": [\"37.150.146.135-YGERCDlDr0\", "
            "\"91.192.44.33-h7inBdhIO8\", \"109.173.92.236-yyanZt9JjB\", "
            "\"5.254.65.220-XJeTsi7BJd\", \"77.111.244.68-Iun9ywA91u\", "
            "\"93.84.141.230-tvceiijjL0\", \"95.97.235.215-KCg83f1R84\", "
            "\"213.230.102.11-92ADwcJL9F\", \"185.59.58.77-1gbNOL1VPp\", "
            "\"77.111.244.50-mEcoc41ePn\", \"188.113.156.146-9ZfhaRDPG4\", "
            "\"78.36.204.66-oQ6qWl2I4r\", \"109.169.174.71-NO7HZmaujt\", "
            "\"109.200.159.26-0z4LpzUdNI\", \"188.113.156.146-ZwpAjOIHBq\", "
            "\"95.135.255.83-e6Pocjkc99\", \"109.173.92.236-IdgipOXRwX\", "
            "\"141.101.8.78-tFZfa9TYnW\"]}, "
            "\"jm_d2c0aad1-a5dd-4fd9-b614-c8e44197a22c_uplink\": "
            "{\"occupancy\": 1, \"uuids\": "
            "[\"ae74ed7c-3deb-4344-97d6-b7f5bbd70974\"]}, \"hello_world\": "
            "{\"occupancy\": 2, \"uuids\": "
            "[\"pn-78094928-e577-4dfa-bdb5-eb94552537fd\", "
            "\"72ead2d9-c054-624f-9628-05b4a08bd261\"]}, \"beakon_test\": "
            "{\"occupancy\": 1, \"uuids\": [\"DANIELLE\"]}, "
            "\"channel_450_4_86a9106ae65537651a8e456835b316a\": "
            "{\"occupancy\": 1, \"uuids\": "
            "[\"de934289-127f-46bc-bc97-cdcdcbb42ca3\"]}, \"ORQ\": "
            "{\"occupancy\": 1, \"uuids\": "
            "[\"795246cb-7ee1-4e5c-a2c7-70212ae58829\"]}, \"TLY\": "
            "{\"occupancy\": 1, \"uuids\": "
            "[\"b865d2a3-530b-48ef-955d-a5cef83ec8e7\"]}, "
            "\"pubnub-html5-notification-demo\": {\"occupancy\": 1, \"uuids\": "
            "[\"94c635cd-7b5e-4c84-bb1c-1ed7b3f1a37d\"]}, "
            "\"3a21965c-18a6-4e8b-9919-4ed9db9ee0d0\": {\"occupancy\": 1, "
            "\"uuids\": [\"3a21965c-18a6-4e8b-9919-4ed9db9ee0d0\"]}, \"MCS\": "
            "{\"occupancy\": 1, \"uuids\": "
            "[\"d6dd5e62-6f00-46c6-be49-7cfc4af19dd3\"]}, \"ARG\": "
            "{\"occupancy\": 1, \"uuids\": "
            "[\"e91239c2-a6d3-4dfa-a09c-a3ecdfded71e\"]}, \"geckode_twitts\": "
            "{\"occupancy\": 1, \"uuids\": "
            "[\"pn-f8c944f9-dc6d-4cef-875d-58832267facf\"]}, "
            "\"Shield_Proxy_Check_Online\": {\"occupancy\": 2, \"uuids\": "
            "[\"14.184.255.135\", \"49.146.38.83\"]}, \"10chat-demo\": "
            "{\"occupancy\": 1, \"uuids\": "
            "[\"6f444ecb-30d0-452c-9e7c-50b2381cbb6b\"]}, "
            "\"Shield_Proxy_Get_Analytics\": {\"occupancy\": 3, \"uuids\": "
            "[\"49.145.162.113-1000034174206736\", "
            "\"49.146.38.83-1000021295333234\", "
            "\"49.145.162.113-1000020741589926\"]}, "
            "\"channel_1_1_d1e7b7cb7b79372fcbaa3467b77904fd\": {\"occupancy\": "
            "1, \"uuids\": [\"6cddad15-c94d-4bc9-9f18-5a06a899c8d8\"]}, "
            "\"jm_ae571630-9194-45b8-b159-9b64de9c7c8a_dnlink\": "
            "{\"occupancy\": 2, \"uuids\": "
            "[\"ae571630-9194-45b8-b159-9b64de9c7c8a\", "
            "\"6e84858b-e07a-4ffd-9787-56a5437185f7\"]}, \"get_message_save\": "
            "{\"occupancy\": 3, \"uuids\": "
            "[\"pn-5154e88a-4135-4b5d-b1f1-2481b299a62f\", "
            "\"pn-b2fe5222-9cb3-4d70-bd28-06cc883f50f2\", "
            "\"pn-f1f470d8-09c4-4891-8636-fb6fc0773617\"]}, \"global\": "
            "{\"occupancy\": 2, \"uuids\": "
            "[\"3a21965c-18a6-4e8b-9919-4ed9db9ee0d0\", "
            "\"34547ead-0423-4983-a7d1-11cbc554ffa4\"]}, \"awesomeChannel\": "
            "{\"occupancy\": 34, \"uuids\": "
            "[\"pn-35c8aa3c-d98d-4cce-9cd9-e877880406d9\", "
            "\"pn-bd4e24ec-3402-4adf-9c5e-3a92b1931a97\", "
            "\"pn-15a0c50a-85dc-4ca8-9357-6a32e8a7c578\", "
            "\"pn-21e1c6e4-37a0-4f49-80e7-2d79f715944a\", "
            "\"pn-7a7c8b2d-91df-4aba-856b-afefe7c1e7e8\", "
            "\"pn-11a6ff79-a0e0-431d-b8be-2ba19905fcbb\", "
            "\"pn-4532c5f1-3c40-49ad-a491-60fc4550b9e9\", "
            "\"pn-c5d6faa7-82ae-4835-83a9-83e23716c255\", "
            "\"pn-3eadfcdb-6603-4a80-8602-04c17de6002f\", "
            "\"pn-39bbe752-6635-472f-8976-7bc2b010cc88\", "
            "\"pn-e6acc086-2010-4522-88e8-3211e6ed401b\", "
            "\"pn-ea688c72-c58b-4c9f-be4c-3ebb3270c863\", "
            "\"pn-c58068e0-9a34-4fee-84cb-c9e0cb4898f1\", "
            "\"pn-4bfa5de7-3fef-4454-9203-be86ca78a4b4\", "
            "\"pn-a04a9878-3073-4886-994a-8b4dacaf3d24\", "
            "\"pn-cbc70db9-633d-4579-8305-9428dd05f773\", "
            "\"pn-2a1fbfdd-d47d-493e-9443-d7bcf946ec83\", "
            "\"pn-bc312f18-9748-439f-b7b4-08673cbf6612\", "
            "\"pn-018bebfa-3f90-43bf-890b-6a076bb1d0d6\", "
            "\"pn-47c3a162-50ad-4652-8122-ca68e2e3da5a\", "
            "\"pn-9dc24a3d-2f1c-4e05-a0cc-b1427f485495\", "
            "\"pn-fd64bad9-e149-4c99-8092-c89c804e2dfc\", "
            "\"pn-775f2d81-8597-4a30-8f56-17d7de22b405\", "
            "\"pn-f8bc2e3d-d589-4075-8320-c8cac6e0d995\", "
            "\"pn-31da0d8d-4aff-4990-91b9-406d4168a2d9\", "
            "\"pn-7233ca4a-8b66-4661-88ee-ce58f3b7dbba\", "
            "\"pn-48fb48a9-124e-46b2-b8b9-43335d7606f7\", "
            "\"pn-5cfe9ac5-2f18-4ff2-ab64-69e3414a0bd3\", "
            "\"pn-a750e56e-b219-4500-bd5e-feea64076cba\", "
            "\"pn-f6b9e508-6f33-41e8-b357-573fc27f82be\", "
            "\"pn-779290bc-1b4a-4889-ac83-0fbf963c7652\", "
            "\"pn-f4d20bb8-5bf0-4bdf-bc35-9f46b1d19e46\", "
            "\"pn-35307117-f286-459c-b2a2-b7676dbbaa71\", "
            "\"pn-96f63e08-679b-4a8b-8121-36b5f104732a\"]}, "
            "\"__notifications__48300\": {\"occupancy\": 1, \"uuids\": "
            "[\"3e5d4095-903f-4aa1-8143-ed4d0db75c78\"]}, "
            "\"jm_b1095c9a-f7ac-49bf-b193-c6674be717b7_dnlink\": "
            "{\"occupancy\": 1, \"uuids\": "
            "[\"ae74ed7c-3deb-4344-97d6-b7f5bbd70974\"]}, \"BSG\": "
            "{\"occupancy\": 1, \"uuids\": "
            "[\"057acdc2-8b68-460b-b2d2-fb0422bffbcf\"]}, \"psmopschat\": "
            "{\"occupancy\": 1, \"uuids\": "
            "[\"ba70a94a-71bd-46e4-8192-1ba385a1b557\"]}, "
            "\"dev.1a42c0b8b110441aa38d4c7d85ebfca5\": {\"occupancy\": 1, "
            "\"uuids\": [\"15456471-2bf5-43b2-a9cd-f5dcc8eaaadd\"]}, "
            "\"SENSEMO-M73F0BB0\": {\"occupancy\": 1, \"uuids\": "
            "[\"42b53d89-0c8b-4105-96d6-d588c22ecf2a\"]}, "
            "\"CoinNoti_Bittrex\": {\"occupancy\": 1, \"uuids\": "
            "[\"pn-72e185b7-69ce-4d13-b41e-2040f8cc79e8\"]}, "
            "\"58f7aead8ead0e803885308d\": {\"occupancy\": 1, \"uuids\": "
            "[\"c5e650c8-3d44-4304-b26a-a79db25be6d3\"]}, "
            "\"jm_b1095c9a-f7ac-49bf-b193-c6674be717b7_uplink\": "
            "{\"occupancy\": 1, \"uuids\": "
            "[\"ae74ed7c-3deb-4344-97d6-b7f5bbd70974\"]}, "
            "\"undefined_geckodeSuscriber\": {\"occupancy\": 1, \"uuids\": "
            "[\"pn-f8c944f9-dc6d-4cef-875d-58832267facf\"]}, "
            "\"jm_d2c0aad1-a5dd-4fd9-b614-c8e44197a22c_dnlink\": "
            "{\"occupancy\": 2, \"uuids\": "
            "[\"d2c0aad1-a5dd-4fd9-b614-c8e44197a22c\", "
            "\"ae74ed7c-3deb-4344-97d6-b7f5bbd70974\"]}}, \"total_channels\": "
            "53, \"total_occupancy\": 282}, \"service\": \"Presence\"}"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));
}


Ensure(single_context_pubnub, gzip_bad_compression_format)
{
    uint8_t gzip_body[] = { 0x1f, 0x8b, 0x08, 0x00, 0x61, 0xb3, 0xe4, 0x5a,
                            0x02, 0x03, 0x8b, 0x8e, 0x2e, 0xc8, 0xcc, 0x4b,
                            0xd5, 0x49, 0xae, 0x2c, 0x28, 0x4a, 0x2d, 0x2e,
                            0xd6, 0x29, 0x48, 0xcc, 0xc9, 0x8d, 0xd5, 0x51,
                            0x32, 0x34, 0x35, 0x34, 0x33, 0x30, 0x34, 0xb1,
                            0x34, 0xb7, 0xb0, 0x34, 0x32, 0x35, 0x36, 0x32,
                            0x36, 0x31, 0x37, 0x54, 0x8a, 0x05, 0x00, 0x1b,
                            0x37, 0xa9, 0x34, 0x2b, 0x00, 0x00, 0x00 };
    struct uint8_block body_block = { 63, gzip_body };

    uint8_t gzip_body_garbage[] = {
        0x1f, 0x8b, 0x08, 0x00, 0x61, 0xb3, 0xe4, 0x5a, 0x02, 0x03, 0x89,
        0x5e, 0x1e, 0x18, 0x1c, 0x1b, 0x15, 0x19, 0x2e, 0x0c, 0x88, 0xfa,
        0xfd, 0xee, 0xd6, 0xd9, 0x98, 0x3c, 0x29, 0xfd, 0xd5, 0x51, 0x32,
        0x34, 0x35, 0x34, 0x33, 0x30, 0x34, 0xb1, 0x34, 0xb7, 0xb0, 0x34,
        0x32, 0x35, 0x36, 0x32, 0x36, 0x31, 0x37, 0x54, 0x8a, 0x05, 0x00,
        0x1b, 0x37, 0xa9, 0x34, 0x2b, 0x00, 0x00, 0x00
    };
    struct uint8_block body_garbage_block = { 63, gzip_body_garbage };

    uint8_t            extra_byte       = 0x01;
    struct uint8_block extra_byte_block = { 1, &extra_byte };

    pubnub_init(pbp, "looking-glass", "looking-glass");
    pubnub_set_user_id(pbp, "test_id");

    expect_have_dns_for_pubnub_origin();
    expect_outgoing_with_url(
        "/subscribe/looking-glass/island/0/0?pnsdk=unit-test-0.1&uuid=test_id");
    incoming("HTTP/1.1 200\r\nContent-Length: "
             "26\r\n\r\n[[],\"1516014978925123457\"]",
             NULL);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(pbp, "island", NULL), equals(PNR_OK));

    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));

    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/subscribe/looking-glass/island/0/"
                             "1516014978925123457?pnsdk=unit-test-0.1&uuid=test_id");
    /* Changing 'gzip' format byte into something else */
    gzip_body[1]++;
    incoming("HTTP/1.1 200\r\n"
             "Content-Length: 63\r\n"
             "Content-Encoding: gzip\r\n"
             "\r\n",
             &body_block);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(pbp, "island", NULL),
           equals(PNR_BAD_COMPRESSION_FORMAT));
    gzip_body[1] = 0x8b;

    /* Changing 'deflate' algoritam byte into something else */
    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/subscribe/looking-glass/island/0/"
                             "1516014978925123457?pnsdk=unit-test-0.1&uuid=test_id");
    gzip_body[2]--;
    body_block.size = sizeof gzip_body;
    incoming("HTTP/1.1 200\r\n"
             "Content-Length: 63\r\n"
             "Content-Encoding: gzip\r\n"
             "\r\n",
             &body_block);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(pbp, "island", NULL),
           equals(PNR_BAD_COMPRESSION_FORMAT));
    gzip_body[2]++;

    /* Changing 'flags' byte to defer 0(expected by 'pubnub gzip' format:
       no filename, nor extras in the compressed block)
     */
    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/subscribe/looking-glass/island/0/"
                             "1516014978925123457?pnsdk=unit-test-0.1&uuid=test_id");
    gzip_body[3]++;
    body_block.size = sizeof gzip_body;
    incoming("HTTP/1.1 200\r\n"
             "Content-Length: 63\r\n"
             "Content-Encoding: gzip\r\n"
             "\r\n",
             &body_block);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(pbp, "island", NULL),
           equals(PNR_BAD_COMPRESSION_FORMAT));
    gzip_body[3]--;

    /* Receiving shorter message than it should be */
    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/subscribe/looking-glass/island/0/"
                             "1516014978925123457?pnsdk=unit-test-0.1&uuid=test_id");
    body_block.size = sizeof gzip_body - 1;
    incoming("HTTP/1.1 200\r\n"
             "Content-Length: 62\r\n"
             "Content-Encoding: gzip\r\n"
             "\r\n",
             &body_block);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(pbp, "island", NULL),
           equals(PNR_BAD_COMPRESSION_FORMAT));

    /* Receiving longer message than it should be */
    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/subscribe/looking-glass/island/0/"
                             "1516014978925123457?pnsdk=unit-test-0.1&uuid=test_id");
    body_block.size = sizeof gzip_body;
    incoming("HTTP/1.1 200\r\n"
             "Content-Length: 64\r\n"
             "Content-Encoding: gzip\r\n"
             "\r\n",
             &body_block);
    incoming(NULL, &extra_byte_block);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(pbp, "island", NULL), equals(PNR_STARTED));
    attest(pbnc_fsm(pbp), equals(0));
    attest(pbp->core.last_result, equals(PNR_BAD_COMPRESSION_FORMAT));

    /* Changing 'isize'(size of decompressed block) value byte into smaller
     * value */
    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/subscribe/looking-glass/island/0/"
                             "1516014978925123457?pnsdk=unit-test-0.1&uuid=test_id");
    gzip_body[sizeof gzip_body - 4]--;
    body_block.size = sizeof gzip_body;
    incoming("HTTP/1.1 200\r\n"
             "Content-Length: 63\r\n"
             "Content-Encoding: gzip\r\n"
             "\r\n",
             &body_block);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(pbp, "island", NULL),
           equals(PNR_BAD_COMPRESSION_FORMAT));
    gzip_body[sizeof gzip_body - 4]++;

    /* Changing 'isize'(size of decompressed block) value byte into greater
     * value */
    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/subscribe/looking-glass/island/0/"
                             "1516014978925123457?pnsdk=unit-test-0.1&uuid=test_id");
    gzip_body[sizeof gzip_body - 4]++;
    body_block.size = sizeof gzip_body;
    incoming("HTTP/1.1 200\r\n"
             "Content-Length: 63\r\n"
             "Content-Encoding: gzip\r\n"
             "\r\n",
             &body_block);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(pbp, "island", NULL),
           equals(PNR_BAD_COMPRESSION_FORMAT));
    gzip_body[sizeof gzip_body - 4]--;

    /* Contaminated content */
    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/subscribe/looking-glass/island/0/"
                             "1516014978925123457?pnsdk=unit-test-0.1&uuid=test_id");
    gzip_body[30]--;
    gzip_body[32]--;
    gzip_body[34]--;
    gzip_body[38]++;
    body_block.size = sizeof gzip_body;
    incoming("HTTP/1.1 200\r\n"
             "Content-Length: 63\r\n"
             "Content-Encoding: gzip\r\n"
             "\r\n",
             &body_block);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    /* Decompression went through, but result is also contaminated */
    attest(pubnub_subscribe(pbp, "island", NULL), equals(PNR_FORMAT_ERROR));
    gzip_body[30]++;
    gzip_body[32]++;
    gzip_body[34]++;
    gzip_body[38]--;

    /* Returning to all correct(initial) parameters in the compressed block */
    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/subscribe/looking-glass/island/0/"
                             "1516014978925123457?pnsdk=unit-test-0.1&uuid=test_id");
    body_block.size = sizeof gzip_body;
    incoming("HTTP/1.1 200\r\n"
             "Content-Length: 63\r\n"
             "Content-Encoding: gzip\r\n"
             "\r\n",
             &body_block);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(pbp, "island", NULL), equals(PNR_OK));
    attest(pubnub_last_time_token(pbp), streqs("1516014978925323471"));

    attest(pubnub_get(pbp), streqs("pine"));
    attest(pubnub_get(pbp), streqs("cypress"));
    attest(pubnub_get(pbp), streqs("palm"));
    attest(pubnub_get(pbp), equals(NULL));
    attest(pubnub_get_channel(pbp), streqs(NULL));
    attest(pubnub_last_http_code(pbp), equals(200));

    /* Receiving gzip garbage block */
    expect(pbntf_enqueue_for_processing, when(pb, equals(pbp)), returns(0));
    expect(pbntf_got_socket, when(pb, equals(pbp)), returns(0));
    expect_outgoing_with_url("/subscribe/looking-glass/island/0/"
                             "1516014978925323471?pnsdk=unit-test-0.1&uuid=test_id");
    body_block.size = sizeof gzip_body;
    incoming("HTTP/1.1 200\r\n"
             "Content-Length: 63\r\n"
             "Content-Encoding: gzip\r\n"
             "\r\n",
             &body_garbage_block);
    expect(pbntf_lost_socket, when(pb, equals(pbp)));
    expect(pbntf_trans_outcome, when(pb, equals(pbp)));
    attest(pubnub_subscribe(pbp, "island", NULL),
           equals(PNR_BAD_COMPRESSION_FORMAT));
}

Ensure(single_context_pubnub, wont_fail_on_freed) {
    // Do nothing, just let alloc and free happen
}

/* Verify ASSERT gets fired */
Ensure(single_context_pubnub, illegal_context_fires_assert)
{
#if PUBNUB_USE_ADVANCED_HISTORY
    pubnub_chamebl_t o_msg[1];
    size_t io_count;
    struct pubnub_chan_msg_count chan_msg_counters[1];
    int    count;
#endif
    pubnub_assert_set_handler((pubnub_assert_handler_t)test_assert_handler);

    expect_assert_in(pubnub_init(NULL, "k", "u"), "pubnub_pubsubapi.c");
    expect_assert_in(pubnub_publish(NULL, "x", "0"), "pubnub_pubsubapi.c");
    expect_assert_in(pubnub_last_publish_result(NULL), "pubnub_pubsubapi.c");
    expect_assert_in(pubnub_history(NULL, "ch", 22, true), "pubnub_coreapi.c");
    expect_assert_in(pubnub_last_time_token(NULL), "pubnub_pubsubapi.c");
    expect_assert_in(pubnub_subscribe(NULL, "x", NULL), "pubnub_pubsubapi.c");
    expect_assert_in(pubnub_leave(NULL, "x", NULL), "pubnub_coreapi.c");
    expect_assert_in(pubnub_cancel(NULL), "pubnub_pubsubapi.c");
    expect_assert_in(pubnub_set_user_id(NULL, ""), "pubnub_pubsubapi.c");
    PUBNUB_DISABLE_WARNING_PUSH
    PUBNUB_DISABLE_DEPRECATED
    expect_assert_in(pubnub_set_uuid(NULL, ""), "pubnub_pubsubapi.c");
    PUBNUB_DISABLE_WARNING_POP
    expect_assert_in(pubnub_user_id_get(NULL), "pubnub_pubsubapi.c");
    expect_assert_in(pubnub_set_auth(NULL, ""), "pubnub_pubsubapi.c");
    expect_assert_in(pubnub_auth_get(NULL), "pubnub_pubsubapi.c");
    expect_assert_in(pubnub_last_http_code(NULL), "pubnub_pubsubapi.c");
    expect_assert_in(pubnub_get(NULL), "pubnub_pubsubapi.c");
    expect_assert_in(pubnub_get_channel(NULL), "pubnub_pubsubapi.c");
    expect_assert_in(pubnub_set_state(NULL, "y", "group", NULL, "{}"),
                     "pubnub_coreapi.c");
    expect_assert_in(pubnub_state_get(NULL, "y", "group", NULL), "pubnub_coreapi.c");
    expect_assert_in(pubnub_here_now(NULL, "ch", "ch_group"), "pubnub_coreapi.c");
    expect_assert_in(pubnub_global_here_now(NULL), "pubnub_coreapi.c");
    expect_assert_in(pubnub_heartbeat(NULL, "25", "[37,0Rh-]"), "pubnub_coreapi.c");
    expect_assert_in(pubnub_where_now(NULL, "uuid"), "pubnub_coreapi.c");
    expect_assert_in(pubnub_where_now(pbp, NULL), "pubnub_ccore.c");
    expect_assert_in(pubnub_add_channel_to_group(NULL, "ch", "group"),
                     "pubnub_coreapi.c");
    expect_assert_in(pubnub_add_channel_to_group(pbp, NULL, "group"),
                     "pubnub_ccore.c");
    expect_assert_in(pubnub_add_channel_to_group(pbp, "ch", NULL), "pubnub_ccore.c");
    expect_assert_in(pubnub_remove_channel_from_group(NULL, "ch", "group"),
                     "pubnub_coreapi.c");
    expect_assert_in(pubnub_remove_channel_from_group(pbp, NULL, "group"),
                     "pubnub_ccore.c");
    expect_assert_in(pubnub_remove_channel_from_group(pbp, "ch", NULL),
                     "pubnub_ccore.c");
    expect_assert_in(pubnub_remove_channel_group(NULL, "group"), "pubnub_coreapi.c");
    expect_assert_in(pubnub_remove_channel_group(pbp, NULL), "pubnub_ccore.c");
    expect_assert_in(pubnub_list_channel_group(NULL, "group"), "pubnub_coreapi.c");
    expect_assert_in(pubnub_list_channel_group(pbp, NULL), "pubnub_ccore.c");
    expect_assert_in(pubnub_subscribe(NULL, "%22cheesy%22", "[milk, meat]"),
                     "pubnub_pubsubapi.c");
    expect_assert_in(pubnub_origin_set(NULL, "origin_server"), "pubnub_pubsubapi.c");
    expect_assert_in(pubnub_get_origin(NULL), "pubnub_pubsubapi.c");
    expect_assert_in(pubnub_free((pubnub_t*)((char*)pbp + 10000)),
                     "pubnub_alloc_static.c");
#if PUBNUB_USE_ACTIONS_API
    expect_assert_in(pubnub_parse_token(pbp, NULL), "pubnub_grant_token_api.c");
#endif
#if PUBNUB_USE_ADVANCED_HISTORY
    expect_assert_in(pubnub_get_error_message(NULL, o_msg), "pubnub_advanced_history.c");
    expect_assert_in(pubnub_get_chan_msg_counts_size(NULL), "pubnub_advanced_history.c");
    expect_assert_in(pubnub_message_counts(NULL, "ch", "12345"), "pubnub_advanced_history.c");
    expect_assert_in(pubnub_message_counts(pbp, NULL, "12345"), "pubnub_advanced_history.c");
    expect_assert_in(pubnub_message_counts(pbp, "ch", NULL), "pubnub_advanced_history.c");
    expect_assert_in(pubnub_get_chan_msg_counts(NULL,
                                                &io_count,
                                                chan_msg_counters),
                     "pubnub_advanced_history.c");
    expect_assert_in(pubnub_get_chan_msg_counts(pbp,
                                                NULL,
                                                chan_msg_counters),
                     "pubnub_advanced_history.c");
    expect_assert_in(pubnub_get_chan_msg_counts(pbp,
                                                &io_count,
                                                NULL),
                     "pubnub_advanced_history.c");
    expect_assert_in(pubnub_get_message_counts(NULL, "ch", &count), "pubnub_advanced_history.c");
    expect_assert_in(pubnub_get_message_counts(pbp, NULL, &count), "pubnub_advanced_history.c");
    expect_assert_in(pubnub_get_message_counts(pbp, "ch", NULL), "pubnub_advanced_history.c");
#endif /* -- ADVANCED HISTORY message_counts -- */
}

#if 0
int main(int argc, char *argv[])
{
    TestSuite *suite = create_test_suite();
    add_test_with_context(suite, single_context_pubnub, time_in_progress);
    run_test_suite(suite, create_text_reporter());
}
#endif
