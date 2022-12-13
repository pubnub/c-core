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

    pubnub_origin_set(pbp, NULL);
	pubnub_init(pbp, "pub_key", "sub_key");
    pubnub_set_user_id(pbp, "test_id");
}

AfterEach(token_parsing) {
    pubnub_cleanup_mocks(pbp);
}

typedef struct TestCase{
    char* raw_token;
    char* expected_parsed_token;
} TestCase;

static TestCase parsing_test_cases[] = {
    {
        .raw_token = "p0F2AkF0GmOX59NDdHRsGDxDcmVzpURjaGFuomRteWNoGB9raGVsbG9fd29ybGQDQ2dycKJkbXljZxgfbWNoYW5uZWwtZ3JvdXAHQ3NwY6FlbXlzcGMYH0N1c3KhZm15dXNlchgfRHV1aWSgQ3BhdKVEY2hhbqBDZ3JwoENzcGOhYl4kAUN1c3KhYl4kAUR1dWlkoERtZXRhoENzaWdYIJAZo8ma70ti67jei4f1ytynBcNPGm2lMS_tNuEChca8", 
        .expected_parsed_token = "{\"v\":2, \"t\":1670899667, \"ttl\":60, \"res\":{\"chan\":{\"mych\":31, \"hello_world\":3}, \"grp\":{\"mycg\":31, \"channel-group\":7}, \"spc\":{\"myspc\":31}, \"usr\":{\"myuser\":31}, \"uuid\":{}}, \"pat\":{\"chan\":{}, \"grp\":{}, \"spc\":{\"^$\":1}, \"usr\":{\"^$\":1}, \"uuid\":{}}, \"meta\":{}, \"sig\":\"kBmjyZrvS2LruN6Lh/XK3KcFw08abaUxL+024QKFxrw=\"}"
    },
    {
        .raw_token = "p0F2AkF0GmOX8A9DdHRsGDxDcmVzpURjaGFuomtoZWxsb193b3JsZANkbXljaBgfQ2dycKJtY2hhbm5lbC1ncm91cANkbXljZxgfQ3NwY6FlbXlzcGMYH0N1c3KhZml0c19tZRgcRHV1aWSgQ3BhdKVEY2hhbqBDZ3JwoENzcGOhYl4kAUN1c3KhYl4kAUR1dWlkoERtZXRhoENzaWdYILD4I0SjS4tdpUiOj987_LnuFRQCjbI2UtmlSdvUiWxQ",
        .expected_parsed_token = "{\"v\":2, \"t\":1670901775, \"ttl\":60, \"res\":{\"chan\":{\"hello_world\":3, \"mych\":31}, \"grp\":{\"channel-group\":3, \"mycg\":31}, \"spc\":{\"myspc\":31}, \"usr\":{\"its_me\":28}, \"uuid\":{}}, \"pat\":{\"chan\":{}, \"grp\":{}, \"spc\":{\"^$\":1}, \"usr\":{\"^$\":1}, \"uuid\":{}}, \"meta\":{}, \"sig\":\"sPgjRKNLi12lSI6P3zv8ue4VFAKNsjZS2aVJ29SJbFA=\"}"
    }
};

Ensure(token_parsing, should_properly_parse_tokens) {
    size_t test_cases_count = sizeof(parsing_test_cases)/sizeof(parsing_test_cases[0]);

    for (int test_case = 0; test_case < test_cases_count; test_case++) {
    	char* raw_token = parsing_test_cases[test_case].raw_token;
    
    	char* parsed_token = pubnub_parse_token(pbp, raw_token);
    
    	char* expected_parsed_token = parsing_test_cases[test_case].expected_parsed_token;
    
        assert_that(*parsed_token, is_equal_to(*expected_parsed_token));
        free(parsed_token);
    }
}

static char* crashing_test_cases[] = {
    "dummy data",
    "abcdefg",
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789",
    "",
    "1234567890"
};

Ensure(token_parsing, should_not_crashing_for_not_valid_values) {
    size_t test_cases_count = sizeof(crashing_test_cases)/sizeof(crashing_test_cases[0]);

    for (int test_case = 0; test_case < test_cases_count; test_case++) {
        char* parsed_token = pubnub_parse_token(pbp, crashing_test_cases[test_case]);

        assert_that(parsed_token, is_null);

        free(parsed_token);
    }
}

#if 0
int main(int argc, char *argv[]) {
    TestSuite *suite = create_test_suite();
    add_test_with_context(suite, token_parsing);
    run_test_suite(suite, create_text_reporter());
}
#endif // 0
#endif // PUBNUB_USE_GRANT_TOKEN_API
