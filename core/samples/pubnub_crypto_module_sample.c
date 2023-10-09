/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pbcc_crypto.h"
#include "pubnub_api_types.h"
#include "pubnub_memory_block.h"
#include "pubnub_sync.h"

#include "core/pubnub_helper.h"
#include "core/pubnub_timers.h"

#include "core/pubnub_pubsubapi.h"
#include "core/pubnub_coreapi_ex.h"
#include "core/pubnub_crypto.h"

#include <stdint.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

static void generate_user_id(pubnub_t *pbp);
static void sync_sample_free(pubnub_t* p);
static void print_encrypted_message(const char* display, pubnub_bymebl_t encrypted_message);

int main()
{
    char const *msg = "\"Hello world from Pubnub C core API!\"";
    uint8_t const *cipher_key = (uint8_t*)"enigma";

    printf("message to be encrypted: %s\n\n", msg);

    pubnub_bymebl_t block;
    block.ptr = (uint8_t*)msg;
    block.size = strlen(msg);

    struct pubnub_crypto_provider_t *legacy_crypto_module = pubnub_crypto_legacy_module_init(cipher_key);
    pubnub_bymebl_t legacy_encrypt_result = legacy_crypto_module->encrypt(legacy_crypto_module, block);

    if (NULL == legacy_encrypt_result.ptr) {
        printf("encrypt with legacy AES-CBC failed\n");
        return -1;
    }

    print_encrypted_message("encrypt with legacy AES-CBC result", legacy_encrypt_result);

    struct pubnub_crypto_provider_t *crypto_module = pubnub_crypto_aes_cbc_module_init(cipher_key);
    pubnub_bymebl_t encrypt_result = crypto_module->encrypt(crypto_module, block);
    
    if (NULL == encrypt_result.ptr) {
        printf("encrypt with enhanced AES-CBC failed\n");
        return -1;
    }

    print_encrypted_message("encrypt with enhanced AES-CBC result", encrypt_result);

    pubnub_bymebl_t legacy_decrypt_result = crypto_module->decrypt(crypto_module, legacy_encrypt_result);

    if (NULL == legacy_decrypt_result.ptr) {
        printf("decrypt with legacy AES-CBC failed\n");
        return -1;
    }

    printf("decrypt with legacy AES-CBC result: %s\n", legacy_decrypt_result.ptr);

    pubnub_bymebl_t decrypt_result = legacy_crypto_module->decrypt(legacy_crypto_module, encrypt_result);

    if (NULL == decrypt_result.ptr) {
        printf("decrypt with enhanced AES-CBC failed\n");
        return -1;
    }

    printf("decrypt with enhanced AES-CBC result: %s\n", decrypt_result.ptr);
    
    /* When `PUBNUB_CRYPTO_API` flag is defined, Pubnub Crypto API is
       automatically derivered into Pubnub C-core API. This means that 
       `publish`, `subscribe` and `history` functions will automatically
       encrypt and decrypt messages.
    */
#ifdef PUBNUB_CRYPTO_API
    pubnub_t *pbp = pubnub_alloc();

    if (NULL == pbp) {
        printf("Failed to allocate Pubnub context!\n");
        return -1;
    }

    char* my_env_publish_key = getenv("PUBNUB_PUBLISH_KEY");
    if (NULL == my_env_publish_key) { my_env_publish_key = "demo"; }
    char* my_env_subscribe_key = getenv("PUBNUB_SUBSCRIBE_KEY");
    if (NULL == my_env_subscribe_key) { my_env_subscribe_key = "demo"; }
    pubnub_init(pbp, "demo", "demo");

    /* Leave this commented out to use the default - which is
       blocking I/O on most platforms. Uncomment to use non-
       blocking I/O.
    */
    pubnub_set_non_blocking_io(pbp);
    generate_user_id(pbp);

    pubnub_set_crypto_module(pbp, pubnub_crypto_aes_cbc_module_init(cipher_key));

    enum pubnub_res result;

    printf("Subscribing to PubNub infrastructure...\n");

    result = pubnub_subscribe(pbp, "hello_world", NULL);
    if (PNR_STARTED == result) {
        result = pubnub_await(pbp);
    }

    if (PNR_OK != result) {
        printf("Subscribing failed with code: %d('%s')\n",
               pubnub_last_result(pbp),
               pubnub_res_2_string(pubnub_last_result(pbp)));
        sync_sample_free(pbp);
        return -1;
    }

    printf("Publishing a message...\n");

    result = pubnub_publish(pbp, "hello_world", msg);
    if (PNR_STARTED == result) {
        result = pubnub_await(pbp);
    }

    if (PNR_OK != result) {
        printf("Publishing failed with code: %d('%s')\n",
               pubnub_last_result(pbp),
               pubnub_res_2_string(pubnub_last_result(pbp)));
        sync_sample_free(pbp);
        return -1;
    }

    printf("Fetching messages...\n");

    result = pubnub_subscribe(pbp, "hello_world", NULL);
    if (PNR_STARTED == result) {
        result = pubnub_await(pbp);
    }

    if (PNR_OK != result) {
        printf("Subscribing failed with code: %d('%s')\n",
               pubnub_last_result(pbp),
               pubnub_res_2_string(pubnub_last_result(pbp)));
        sync_sample_free(pbp);
        return -1;
    }

    printf("Here are the messages:\n");
    for (const char* msg = pubnub_get(pbp); msg != NULL; msg = pubnub_get(pbp)) {
        printf("> %s\n", msg);
    }

    puts("Pubnub crypto module demo over.");

#endif

    return 0;
}

static void generate_user_id(pubnub_t *pbp)
{
    char const *user_id_default = "zeka-peka-iz-jendeka";
    struct Pubnub_UUID uuid;
    static struct Pubnub_UUID_String str_uuid;

    if (0 != pubnub_generate_uuid_v4_random(&uuid)) {
        pubnub_set_user_id(pbp, user_id_default);
    }
    else {
        str_uuid = pubnub_uuid_to_string(&uuid);
        pubnub_set_user_id(pbp, str_uuid.uuid);
        printf("Generated UUID: %s\n", str_uuid.uuid);
    }
}


static void sync_sample_free(pubnub_t* p)
{
    if (PN_CANCEL_STARTED == pubnub_cancel(p)) {
        enum pubnub_res pnru = pubnub_await(p);
        if (pnru != PNR_OK) {
            printf("Awaiting cancel failed: %d('%s')\n",
                   pnru,
                   pubnub_res_2_string(pnru));
        }
    }
    if (pubnub_free(p) != 0) {
        printf("Failed to free the Pubnub context\n");
    }
}

static void print_encrypted_message(const char* display, pubnub_bymebl_t encrypted_message)
{
    printf("%s: ", display);

    for (int i = 0; i < encrypted_message.size; i++) {
        printf("%c", encrypted_message.ptr[i]);
    }
    printf("\n");

    for (int i = 0; i < encrypted_message.size; i++) {
        printf("%d, ", encrypted_message.ptr[i]);
    }
    printf("\n\n");
}


