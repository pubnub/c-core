/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_sync.h"

#include "pubnub_helper.h"
#include "pubnub_timers.h"

#include "pubnub_coreapi_ex.h"
#include "pubnub_crypto.h"

#include <stdio.h>
#include <time.h>


/** This is a sample for Pubnub Crypto API. It is only usable if the
    platform has support for cryptography - like the OpenSSL
    "library/platform".
 */


static void generate_uuid(pubnub_t *pbp)
{
    char const *uuid_default = "zeka-peka-iz-jendeka";
    struct Pubnub_UUID uuid;
    static struct Pubnub_UUID_String str_uuid;

    if (0 != pubnub_generate_uuid_v4_random(&uuid)) {
        pubnub_set_uuid(pbp, uuid_default);
    }
    else {
        str_uuid = pubnub_uuid_to_string(&uuid);
        pubnub_set_uuid(pbp, str_uuid.uuid);
        printf("Generated UUID: %s\n", str_uuid.uuid);
    }
}


int main()
{
    clock_t clk;
    enum pubnub_res res;
    char const *chan = "hello_world";
    char const *cipher_key = "4443443";
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

    generate_uuid(pbp);

    pubnub_set_auth(pbp, "danaske");


    puts("Subscribing...");
    clk = clock();
    res = pubnub_subscribe(pbp, chan, NULL);
    if (res != PNR_STARTED) {
        printf("pubnub_subscribe() returned unexpected: %d\n", res);
        pubnub_free(pbp);
        return -1;
    }
    res = pubnub_await(pbp);
    clk = clock() - clk;
    printf("Subscribe-connect took %d clicks (%f seconds).\n", (int)clk, ((float)clk)/CLOCKS_PER_SEC);
    if (res == PNR_STARTED) {
        printf("pubnub_await() returned unexpected: PNR_STARTED(%d)\n", res);
        pubnub_free(pbp);
        return -1;
    }

    if (PNR_OK == res) {
        puts("Subscribed!");
    }
    else {
        printf("Subscribing failed with code: %d\n", res);
    }

    puts("Publishing...");
    clk = clock();
    res = pubnub_publish_encrypted(pbp, chan, "\"Hello world from crypto sync!\"", cipher_key);
    if (res != PNR_STARTED) {
        printf("pubnub_publish() returned unexpected: %d\n", res);
        pubnub_free(pbp);
        return -1;
    }
    res = pubnub_await(pbp);
    clk = clock() - clk;
    printf("Publish took %d clicks (%f seconds).\n", (int)clk, ((float)clk)/CLOCKS_PER_SEC);
    if (res == PNR_STARTED) {
        printf("pubnub_await() returned unexpected: PNR_STARTED(%d)\n", res);
        pubnub_free(pbp);
        return -1;
    }
    if (PNR_OK == res) {
        printf("Published! Response from Pubnub: %s\n", pubnub_last_publish_result(pbp));
    }
    else if (PNR_PUBLISH_FAILED == res) {
        printf("Published failed on Pubnub, description: %s\n", pubnub_last_publish_result(pbp));
    }
    else {
        printf("Publishing failed with code: %d\n", res);
    }

    puts("Second subscribe");

    res = pubnub_subscribe(pbp, chan, NULL);
    if (res != PNR_STARTED) {
        printf("pubnub_subscribe() returned unexpected: %d\n", res);
        pubnub_free(pbp);
        return -1;
    }
    res = pubnub_await(pbp);
    if (res == PNR_STARTED) {
        printf("pubnub_await() returned unexpected: PNR_STARTED(%d)\n", res);
        pubnub_free(pbp);
        return -1;
    }

    if (PNR_OK == res) {
        puts("Subscribed! Got messages:");
        for (;;) {
            pubnub_bymebl_t mebl = pubnub_get_decrypted_alloc(pbp, cipher_key);
            if (NULL == mebl.ptr) {
                break;
            }
            puts((char*)mebl.ptr);
            free(mebl.ptr);
        }
    }
    else {
        printf("Subscribing failed with code %d: %s\n", res, pubnub_res_2_string(res));
    }
    return -1;
	
    /* We're done */
    if (pubnub_free(pbp) != 0) {
        printf("Failed to free the Pubnub context\n");
    }

    puts("Pubnub crypto sync demo over.");

    return 0;
}
