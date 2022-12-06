/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if defined PUBNUB_USE_GRANT_TOKEN_API

#include "cgreen/cgreen.h"
#include "cgreen/mocks.h"
#include "test/pubnub_test_mocks.h"

#include "pubnub_alloc.h"
#include "pubnub_grant_token_api.h"
#include "pubnub_pubsubapi.h"
#include "pubnub_internal.h"
#include "pubnub_internal_common.h"
#include "pubnub_assert.h"
#include "pubnub_log.h"

static pubnub_t* pbp;

Describe(token_parsing);

BeforeEach(token_parsing) {
    pubnub_setup_mocks(&pbp);
    printf("B address=%p\n", pbp);

    //pubnub_origin_set(pbp, NULL);
	//pubnub_init(pbp, "pub_key", "sub_key");
    //pubnub_set_user_id(pbp, "test_id");
}

AfterEach(token_parsing) {
    pubnub_cleanup_mocks(pbp);
}

// TODO: create valid test cases for this test
Ensure(token_parsing, should_properly_parse_tokens) {
	char* raw_token = "TODO";

	//char* parsed_token = pubnub_parse_token(pbp, raw_token);

	char* expected_parsed_token = "TODO";

	//assert_that(parsed_token, is_equal_to(expected_parsed_token));
}

Ensure(token_parsing, should_not_crashing_for_not_valid_values) {
    assert_that(pubnub_parse_token(pbp, "dummy data"), is_null);
}

#if 0
int main(int argc, char *argv[]) {
    TestSuite *suite = create_test_suite();
    add_test_with_context(suite, token_parsing);
    run_test_suite(suite, create_text_reporter());
}
#endif // 0
#endif // PUBNUB_USE_GRANT_TOKEN_API
