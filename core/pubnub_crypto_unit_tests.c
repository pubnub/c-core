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
#include "pubnub_coreapi.h"
#include "pubnub_crypto.h"
#include "pbcc_crypto.h"
#include <stdint.h>
#include <stdlib.h>

static pubnub_t* pbp;

#define METADATA "meta"
#define METADATA_SIZE 4

#define VERSION 0x01
#define VERSION_SIZE 1

#define X "xxxx"
#define X_SIZE 4
int x_encrypt(pubnub_cryptor_t const*, struct pubnub_encrypted_data*, pubnub_bymebl_t);
int x_decrypt(pubnub_cryptor_t const*, pubnub_bymebl_t*, struct pubnub_encrypted_data);
static pubnub_cryptor_t x_cryptor = {
    .identifier = X,
    .encrypt = x_encrypt,
    .decrypt = x_decrypt
};

#define Y "yyyy"
#define Y_SIZE 4
int y_encrypt(pubnub_cryptor_t const*, struct pubnub_encrypted_data*, pubnub_bymebl_t);   
int y_decrypt(pubnub_cryptor_t const*, pubnub_bymebl_t*, struct pubnub_encrypted_data);
static pubnub_cryptor_t y_cryptor = {
    .identifier = Y,
    .encrypt = y_encrypt,
    .decrypt = y_decrypt
};

#define HEADER_SIZE 10

Describe(crypto_api);
BeforeEach(crypto_api) {
    pubnub_setup_mocks(&pbp);
    pubnub_origin_set(pbp, NULL);
	pubnub_init(pbp, "pub_key", "sub_key");
    pubnub_set_user_id(pbp, "test_id");
}
AfterEach(crypto_api) {
    pubnub_cleanup_mocks(pbp);
}


Ensure(crypto_api, module_should_properly_select_algorythm_for_encryption) {
    pubnub_crypto_provider_t *sut = pubnub_crypto_module_init(&x_cryptor, &y_cryptor, 1);

    pubnub_bymebl_t data = {.ptr = (uint8_t*)"test", .size = 4};

    pubnub_bymebl_t result = sut ->encrypt(sut, data);

    size_t expected_size = HEADER_SIZE + METADATA_SIZE + X_SIZE;
    assert_that(result.ptr, is_equal_to_contents_of("PNED\x01xxxx\x04metaxxxx", expected_size));
    assert_that(result.size, is_equal_to(expected_size));
}

Ensure(crypto_api, module_should_properly_select_algorythm_for_decryption) {
    pubnub_crypto_provider_t *sut = pubnub_crypto_module_init(&x_cryptor, &y_cryptor, 1);

    pubnub_bymebl_t data_x = {.ptr = (uint8_t*)"PNED\x01xxxx\x04metaxxxx", .size = 18};
    pubnub_bymebl_t data_y = {.ptr = (uint8_t*)"PNED\x01yyyy\x04metaxxxx", .size = 18};

    pubnub_bymebl_t result_x = sut ->decrypt(sut, data_x);
    pubnub_bymebl_t result_y = sut ->decrypt(sut, data_y);

    assert_that(result_x.ptr, is_equal_to_string(X));
    assert_that(result_x.size, is_equal_to(X_SIZE));

    assert_that(result_y.ptr, is_equal_to_string(Y));
    assert_that(result_y.size, is_equal_to(Y_SIZE));
}

Ensure(crypto_api, client_should_use_cryptors_for_publish) {
    pubnub_set_crypto_module(pbp, pubnub_crypto_module_init(&x_cryptor, &y_cryptor, 1));

    expect_have_dns_for_pubnub_origin_on_ctx(pbp);

    expect_outgoing_with_url_no_params_on_ctx(pbp,
        "/publish/pub_key/sub_key/0/jarak/0/%22UE5FRAF4eHh4BG1ldGF4eHh4%22");
    incoming("HTTP/1.1 200\r\nContent-Length: "
             "30\r\n\r\n[1,\"Sent\",\"14178940800777403\"]",
             NULL);
    expect(pbntf_lost_socket, when(pb, is_equal_to(pbp)));
    expect(pbntf_trans_outcome, when(pb, is_equal_to(pbp)));
    assert_that(pubnub_last_publish_result(pbp), is_equal_to_string(""));
    assert_that(pubnub_publish(pbp, "jarak", "\"TEST_VALUE\""), is_equal_to(PNR_OK));
    assert_that(pubnub_last_publish_result(pbp), is_equal_to_string("\"Sent\""));
    assert_that(pubnub_last_http_code(pbp), is_equal_to(200));
}

Ensure(crypto_api, client_should_use_cryptors_for_subscribe) {
    pubnub_set_crypto_module(pbp, pubnub_crypto_module_init(&x_cryptor, &y_cryptor, 1));

    assert_that(pubnub_last_time_token(pbp), is_equal_to_string("0"));
    expect_have_dns_for_pubnub_origin_on_ctx(pbp);
    expect_outgoing_with_url_no_params_on_ctx(pbp,
        "/subscribe/sub_key/health/0/0");
    incoming("HTTP/1.1 200\r\nContent-Length: "
             "26\r\n\r\n[[],\"1516014978925123457\"]",
             NULL);
    expect(pbntf_lost_socket, when(pb, is_equal_to(pbp)));
    expect(pbntf_trans_outcome, when(pb, is_equal_to(pbp)));
    assert_that(pubnub_subscribe(pbp, "health", NULL), is_equal_to(PNR_OK));

    assert_that(pubnub_get(pbp), is_equal_to(NULL));
    assert_that(pubnub_last_http_code(pbp), is_equal_to(200));
    assert_that(pubnub_last_time_token(pbp), is_equal_to_string("1516014978925123457"));
    /* Not publish operation */
    assert_that(pubnub_last_publish_result(pbp), is_equal_to_string(""));

    expect(pbntf_enqueue_for_processing, when(pb, is_equal_to(pbp)), will_return(0));
    expect(pbntf_got_socket, when(pb, is_equal_to(pbp)), will_return(0));
    expect_outgoing_with_url_no_params_on_ctx(pbp, "/subscribe/sub_key/health/0/"
                             "1516014978925123457");
    incoming("HTTP/1.1 200\r\nContent-Length: "
             "52\r\n\r\n[[\"UE5FRAF4eHh4BG1ldGF4eHh4\"],"
             "\"1516714978925123457\"]",
             NULL);
    expect(pbntf_lost_socket, when(pb, is_equal_to(pbp)));
    expect(pbntf_trans_outcome, when(pb, is_equal_to(pbp)));
    assert_that(pubnub_subscribe(pbp, "health", NULL), is_equal_to(PNR_OK));
    assert_that(pubnub_last_time_token(pbp), is_equal_to_string("1516714978925123457"));

    assert_that(pubnub_get(pbp), is_equal_to_string(X));
    assert_that(pubnub_get(pbp), is_equal_to(NULL));
    assert_that(pubnub_get_channel(pbp), is_equal_to_string(NULL));
    assert_that(pubnub_last_http_code(pbp), is_equal_to(200));
}

Ensure(crypto_api, client_should_use_cryptors_for_history) {
    pubnub_set_crypto_module(pbp, pubnub_crypto_module_init(&x_cryptor, &y_cryptor, 1));

    expect_have_dns_for_pubnub_origin_on_ctx(pbp);

    expect_outgoing_with_url_no_params_on_ctx(pbp, "/v2/history/sub-key/sub_key/channel/"
                             "ch");
    incoming("HTTP/1.1 200\r\nContent-Length: "
             "28\r\n\r\n[\"UE5FRAF4eHh4BG1ldGF4eHh4\"]",
             NULL);
    expect(pbntf_lost_socket, when(pb, is_equal_to(pbp)));
    expect(pbntf_trans_outcome, when(pb, is_equal_to(pbp)));
    assert_that(pubnub_history(pbp, "ch", 22, false), is_equal_to(PNR_OK));

    assert_that(pubnub_get(pbp), is_equal_to_string(X));
    assert_that(pubnub_get(pbp), is_equal_to(NULL));
    assert_that(pubnub_last_http_code(pbp), is_equal_to(200));
}

int x_encrypt(pubnub_cryptor_t const* _c, struct pubnub_encrypted_data *result, pubnub_bymebl_t _d) {
    result->data.ptr = (uint8_t*)X;
    result->data.size = X_SIZE;
    result->metadata.ptr = (uint8_t*)METADATA;
    result->metadata.size = METADATA_SIZE;
    return 0;
}

int x_decrypt(pubnub_cryptor_t const* _c, pubnub_bymebl_t *result, struct pubnub_encrypted_data _d) {
    result->ptr = (uint8_t*)X;
    result->size = X_SIZE;
    return 0;
}

int y_encrypt(pubnub_cryptor_t const* _c, struct pubnub_encrypted_data *result, pubnub_bymebl_t _d) {
    result->data.ptr = (uint8_t*)Y;
    result->data.size = Y_SIZE;
    result->metadata.ptr = (uint8_t*)METADATA;
    result->metadata.size = METADATA_SIZE;
    return 0;
}

int y_decrypt(pubnub_cryptor_t const* _c, pubnub_bymebl_t *result, struct pubnub_encrypted_data _d) {
    result->ptr = (uint8_t*)Y;
    result->size = Y_SIZE;
    return 0;
}

#if 0
int main(int argc, char *argv[]) {
    TestSuite *suite = create_test_suite();
    add_test_with_context(suite, crypto);
    run_test_suite(suite, create_text_reporter());
}
#endif // 0
//#endif // PUBNUB_CRYPTO_API

