/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_memory_block.h"
#include <stdlib.h>
#if defined PUBNUB_USE_SUBSCRIBE_V2

#include "cgreen/assertions.h"
#include "cgreen/cgreen.h"
#include "cgreen/mocks.h"
#include "cgreen/constraint_syntax_helpers.h"

#include "pubnub_alloc.h"
#include "pubnub_grant_token_api.h"
#include "pubnub_pubsubapi.h"
#include "pubnub_internal.h"
#include "pubnub_internal_common.h"
#include "pubnub_assert.h"
#include "pubnub_log.h"
#include "pubnub_subscribe_v2_message.h"
#include "pubnub_subscribe_v2.h"
#include "test/pubnub_test_mocks.h"

static pubnub_t* pbp;

Describe(subscribe_v2);

BeforeEach(subscribe_v2) {
    pubnub_setup_mocks(&pbp);
    printf("B address=%p\n", pbp);

    pubnub_origin_set(pbp, NULL);
	pubnub_init(pbp, "pub_key", "sub_key");
    pubnub_set_user_id(pbp, "test_id");
}

AfterEach(subscribe_v2) {
    pubnub_cleanup_mocks(pbp);
}

void assert_char_mem_block(struct pubnub_char_mem_block block, char const* expected) {
    char* str = malloc(block.size + 1);
    memcpy((void*)str, block.ptr, block.size);

    str[block.size] = '\0';

    assert_that(str, is_equal_to_string(expected));

    free((void*)str);
}

Ensure(subscribe_v2, should_parse_response_correctly) {
    assert_that(pubnub_last_time_token(pbp), is_equal_to_string("0"));

    assert_that(pubnub_auth_get(pbp), is_equal_to_string(NULL));
    expect_have_dns_for_pubnub_origin_on_ctx(pbp);
    expect_outgoing_with_url_on_ctx(pbp,
        "/v2/subscribe/sub_key/my-channel/0?pnsdk=unit-test-0.1&tt=0&tr=0&uuid=test_id&heartbeat=270");
    incoming("HTTP/1.1 200\r\nContent-Length: "
             "44\r\n\r\n{\"t\":{\"t\":\"15628652479932717\",\"r\":4},\"m\":[]}",
             NULL);

    expect(pbntf_lost_socket, when(pb, is_equal_to(pbp)));
    expect(pbntf_trans_outcome, when(pb, is_equal_to(pbp)));

    assert_that(pubnub_subscribe_v2(pbp, "my-channel", pubnub_subscribe_v2_defopts()), 
        is_equal_to(PNR_OK));
    
    assert_that(pubnub_last_http_code(pbp), is_equal_to(200));
    assert_that(pubnub_last_time_token(pbp), is_equal_to_string("15628652479932717"));

    expect(pbntf_enqueue_for_processing, when(pb, is_equal_to(pbp)), will_return(0));
    expect(pbntf_got_socket, when(pb, is_equal_to(pbp)), will_return(0));
 
    expect_outgoing_with_url_on_ctx(pbp,
        "/v2/subscribe/sub_key/my-channel/0?pnsdk=unit-test-0.1&tt=15628652479932717&tr=4&uuid=test_id&heartbeat=270");
    incoming("HTTP/1.1 220\r\nContent-Length: "
             "183\r\n\r\n{\"t\":{\"t\":\"15628652479932717\",\"r\":4},\"m\":[{\"a\":\"1\",\"f\":514,\"i\":\"publisher_id\",\"s\":1,\"p\":{\"t\":\"15628652479933927\",\"r\":4},\"k\":\"demo\",\"c\":\"my-channel\",\"d\":\"mymessage\",\"b\":\"my-channel\"}]}",
             NULL);

    expect(pbntf_lost_socket, when(pb, is_equal_to(pbp)));
    expect(pbntf_trans_outcome, when(pb, is_equal_to(pbp)));
 
    assert_that(pubnub_subscribe_v2(pbp, "my-channel", pubnub_subscribe_v2_defopts()),
        is_equal_to(PNR_OK));

    struct pubnub_v2_message msg = pubnub_get_v2(pbp);

    assert_char_mem_block(msg.tt, "15628652479933927");
    assert_char_mem_block(msg.channel, "my-channel");
    assert_char_mem_block(msg.payload, "\"mymessage\"");
    assert_char_mem_block(msg.publisher, "publisher_id");

    assert_that(msg.region, is_equal_to(4));
    assert_that(msg.flags, is_equal_to(514));
    assert_that(msg.message_type, is_equal_to(pbsbPublished));
}

#if 0
int main(int argc, char *argv[]) {
    TestSuite *suite = create_test_suite();
    add_test_with_context(suite, token_parsing);
    run_test_suite(suite, create_text_reporter());
}
#endif // 0
#endif // defined PUBNUB_USE_SUBSCRIBE_V2

