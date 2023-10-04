/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
//#if defined PUBNUB_CRYPTO_API

#include "cgreen/assertions.h"
#include "cgreen/cgreen.h"
#include "cgreen/constraint_syntax_helpers.h"
#include "cgreen/mocks.h"
#include "pubnub_memory_block.h"
#include "test/pubnub_test_mocks.h"

#include "pubnub_alloc.h"
#include "pubnub_crypto.h"
#include "pubnub_pubsubapi.h"
#include "pubnub_internal.h"
#include "pubnub_internal_common.h"
#include "pubnub_assert.h"
#include "pubnub_log.h"
#include "pubnub_crypto.h"
#include "pbcc_crypto.h"
#include <stdint.h>
#include <stdlib.h>

Describe(crypto_module);
BeforeEach(crypto_module) {}
AfterEach(crypto_module) {}

Ensure(crypto_module, should_properly_select_algorythm_for_encryption) {
}

Ensure(crypto_module, should_properly_select_algorythm_for_decryption) {
}

Ensure(crypto_module, should_be_used_in_case_of_publish_call) {
}

Ensure(crypto_module, should_be_used_in_case_of_subscribe_call) {
}

Ensure(crypto_module, should_be_used_in_case_of_history_call) {
}




#if 0
int main(int argc, char *argv[]) {
    TestSuite *suite = create_test_suite();
    add_test_with_context(suite, crypto);
    run_test_suite(suite, create_text_reporter());
}
#endif // 0
//#endif // PUBNUB_CRYPTO_API

