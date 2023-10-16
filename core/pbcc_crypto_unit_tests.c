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

#define AMOUNT_OF_TEST_CASES 100
static uint8_t* test_cases[AMOUNT_OF_TEST_CASES];
static uint8_t* cipher_key;

Describe(crypto);

uint8_t *generate_random_bytes(size_t max_amount) {
    size_t n = rand() % max_amount;

    uint8_t *bytes = (uint8_t *)malloc(n);
    assert_that(bytes, is_not_equal_to(NULL));

    for (size_t i = 0; i < n; i++) {
        bytes[i] = rand() % 256;
    }

    return bytes;
}

void generate_random_cipher_key() {
    cipher_key = generate_random_bytes(32);
}

void prepare_test_cases() {
    for (size_t i = 0; i < AMOUNT_OF_TEST_CASES; i++) {
        test_cases[i] = generate_random_bytes(1000);
    }
}

void cleanup_test_cases() {
    for (size_t i = 0; i < AMOUNT_OF_TEST_CASES; i++) {
        free(test_cases[i]);
    }
}

BeforeEach(crypto) {
    prepare_test_cases();
    generate_random_cipher_key();
}

AfterEach(crypto) {
    cleanup_test_cases();
}

void assert_that_cryptor_works_as_expected(pubnub_cryptor_t *sut) {
    for (size_t i = 0; i < AMOUNT_OF_TEST_CASES; i++) {
        pubnub_bymebl_t to_encrypt;
        to_encrypt.ptr = test_cases[i];
        to_encrypt.size = strlen((char*)test_cases[i]);

        struct pubnub_encrypted_data encrypted;
        encrypted.data.ptr = NULL;
        encrypted.data.size = 0;
        encrypted.metadata.ptr = NULL;
        encrypted.metadata.size = 0;

        int encryption_result = sut->encrypt(sut, &encrypted, to_encrypt);
        assert_that(encryption_result, is_equal_to(0));

        pubnub_bymebl_t decrypted;
        decrypted.ptr = NULL;
        decrypted.size = 0;

        int decryption_result = sut->decrypt(sut, &decrypted, encrypted);
        assert_that(decryption_result, is_equal_to(0));
 
        assert_that(decrypted.ptr, is_equal_to_string((char*)test_cases[i]));

        free(encrypted.data.ptr);
        free(encrypted.metadata.ptr);
        free(decrypted.ptr);
    }
}

Ensure(crypto, should_properly_aes_encrypt_and_decrypt_data) {
    pubnub_cryptor_t *sut = pbcc_aes_cbc_init(cipher_key);
    assert_that(sut->identifier, is_equal_to_string("ACRH"));
    assert_that_cryptor_works_as_expected(sut);

    free(sut);
}

Ensure(crypto, should_properly_legacy_encrypt_and_decrypt_data) {
    pubnub_cryptor_t *sut = pbcc_legacy_crypto_init(cipher_key);

    assert_that(sut, is_not_equal_to(NULL));
    char* expected_identifier[4] = {0};
    assert_that(sut->identifier, is_equal_to_contents_of(expected_identifier, 4));
    assert_that_cryptor_works_as_expected(sut);

    free(sut);
}


#if 0
int main(int argc, char *argv[]) {
    TestSuite *suite = create_test_suite();
    add_test_with_context(suite, crypto);
    run_test_suite(suite, create_text_reporter());
}
#endif // 0
//#endif // PUBNUB_CRYPTO_API

