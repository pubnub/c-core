/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_sync.h"

#include "core/pubnub_helper.h"
#include "core/pubnub_timers.h"

#include "core/pubnub_pubsubapi.h"
#include "core/pubnub_coreapi_ex.h"
#include "core/pubnub_crypto.h"

#include <stdio.h>
#include <time.h>


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


int main()
{
    time_t t0;
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

    generate_user_id(pbp);

    pubnub_set_auth(pbp, "danaske");


    puts("Subscribing...");
    time(&t0);
    res = pubnub_subscribe(pbp, chan, NULL);
    if (res == PNR_STARTED) {
        res = pubnub_await(pbp);
    }
    printf("Subscribe/connect lasted %lf seconds.\n", difftime(time(NULL), t0));
    if (PNR_OK == res) {
        puts("Subscribed!");
    }
    else {
        printf("Subscribing failed with code: %d('%s')\n",
               res,
               pubnub_res_2_string(res));
    }

    puts("Publishing...");
    time(&t0);
    res = pubnub_publish_encrypted(pbp, chan, "\"Hello world from crypto sync!\"", cipher_key);
    if (PNR_STARTED == res) {
        res = pubnub_await(pbp);
    }
    printf("Publish lasted %lf seconds.\n", difftime(time(NULL), t0));
    if (PNR_OK == res) {
        printf("Published! Response from Pubnub: %s\n", pubnub_last_publish_result(pbp));
    }
    else if (PNR_PUBLISH_FAILED == res) {
        printf("Published failed on Pubnub, description: %s\n", pubnub_last_publish_result(pbp));
    }
    else {
        printf("Publishing failed with code: %d('%s')\n",
               res,
               pubnub_res_2_string(res));
    }

    puts("Second subscribe");

    res = pubnub_subscribe(pbp, chan, NULL);
    if (res == PNR_STARTED) {
        res = pubnub_await(pbp);
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
        printf("Subscribing failed with code %d('%s')\n", res, pubnub_res_2_string(res));
    }
	
    /* We're done, but, if keep-alive is on, we can't free,
       we need to cancel first...
     */
    sync_sample_free(pbp);

    puts("Pubnub crypto sync demo over.");

    return 0;
}
