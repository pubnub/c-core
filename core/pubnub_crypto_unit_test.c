/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if defined PUBNUB_CRYPTO_API

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

Describe(crypto);

BeforeEach(crypto) {
    pubnub_setup_mocks(&pbp);
    printf("B address=%p\n", pbp);

    pubnub_origin_set(pbp, NULL);
	pubnub_init(pbp, "pub_key", "sub_key");
    pubnub_set_user_id(pbp, "test_id");
}

AfterEach(crypto) {
    pubnub_cleanup_mocks(pbp);
}

#if 0
int main(int argc, char *argv[]) {
    TestSuite *suite = create_test_suite();
    add_test_with_context(suite, token_parsing);
    run_test_suite(suite, create_text_reporter());
}
#endif // 0
#endif // PUBNUB_CRYPTO_API
