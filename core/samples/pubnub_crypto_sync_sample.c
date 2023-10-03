/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pbcc_crypto.h"
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


/** This is a sample for Pubnub Crypto API. It is only usable if the
    platform has support for cryptography - like the OpenSSL
    "library/platform".
 */


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

static void print_encrypted_message(const char* display, pubnub_bymebl_t *encrypted_message)
{
    printf("%s: ", display);

    for (int i = 0; i < encrypted_message->size; i++) {
        printf("%c", encrypted_message->ptr[i]);
    }
    printf("\n");

    for (int i = 0; i < encrypted_message->size; i++) {
        printf("%d, ", encrypted_message->ptr[i]);
    }
    printf("\n\n");
}

int main()
{
    char const *msg = "Hello world";
    char const *cipher_key = "enigma";

    printf("message to be encrypted: %s\n\n", msg);

    pubnub_bymebl_t block;
    block.ptr = (uint8_t*)msg;
    block.size = strlen(msg);

    struct pubnub_crypto_provider_t *legacy_crypto_module = pubnub_crypto_legacy_module_init((uint8_t*) cipher_key);
    pubnub_bymebl_t *legacy_encrypt_result = legacy_crypto_module->encrypt(legacy_crypto_module, block);

    print_encrypted_message("encrypt with legacy AES-CBC result", legacy_encrypt_result);

    struct pubnub_crypto_provider_t *crypto_module = pubnub_crypto_aes_cbc_module_init((uint8_t*)cipher_key);
    pubnub_bymebl_t *encrypt_result = crypto_module->encrypt(crypto_module, block);

    print_encrypted_message("encrypt with enhanced AES-CBC result", encrypt_result);

    pubnub_bymebl_t *legacy_decrypt_result = crypto_module->decrypt(crypto_module, *legacy_encrypt_result);
    printf("decrypt with legacy AES-CBC result: %s\n", legacy_decrypt_result->ptr);

    pubnub_bymebl_t *decrypt_result = legacy_crypto_module->decrypt(legacy_crypto_module, *encrypt_result);
    printf("decrypt with enhanced AES-CBC result: %s\n", decrypt_result->ptr);
    
    return 0;

    pubnub_t *pbp = pubnub_alloc();

    if (NULL == pbp) {
        printf("Failed to allocate Pubnub context!\n");
        return -1;
    }
    pubnub_init(pbp, "demo", "demo");

    pubnub_set_transaction_timeout(pbp, PUBNUB_DEFAULT_NON_SUBSCRIBE_TIMEOUT);

    /* Leave this commented out to use the default - which is
       blocking I/O on most platforms. Uncomment to use non-
       blocking I/O.
    */
    pubnub_set_non_blocking_io(pbp);

    generate_user_id(pbp);

//    pubnub_set_auth(pbp, "danaske");

    /* We're done, but, if keep-alive is on, we can't free,
       we need to cancel first...
     */
    sync_sample_free(pbp);

    puts("Pubnub crypto sync demo over.");

    return 0;
}
