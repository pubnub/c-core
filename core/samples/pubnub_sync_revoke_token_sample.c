/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_sync.h"

#include "core/pubnub_helper.h"
#include "core/pubnub_timers.h"
#include "core/pubnub_crypto.h"
#include "core/pubnub_grant_token_api.h"
#include "core/pubnub_revoke_token_api.h"
#include "core/pubnub_json_parse.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#include <stdio.h>
#include <time.h>

#ifdef _MSC_VER
#define strdup(p) _strdup(p)
#endif

static void generate_user_id(pubnub_t* pbp)
{
    char const*                      user_id_default = "zeka-peka-iz-jendeka";
    struct Pubnub_UUID               uuid;
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


static int do_time(pubnub_t* pbp)
{
    enum pubnub_res res;

    puts("-----------------------");
    puts("Getting time...");
    puts("-----------------------");
    res = pubnub_time(pbp);
    if (PNR_STARTED == res) {
        res = pubnub_await(pbp);
    }
    if (PNR_OK == res) {
        printf("Gotten time: %s; last time token=%s\n",
               pubnub_get(pbp),
               pubnub_last_time_token(pbp));
    }
    else {
        printf("Getting time failed with code: %d('%s')\n",
               res,
               pubnub_res_2_string(res));
    }

    return 0;
}

int main()
{
    time_t             t0;
    enum pubnub_res    res;
    struct Pubnub_UUID uuid;
    char const*        chan = pubnub_uuid_to_string(&uuid).uuid;
    pubnub_t*          gtp  = pubnub_alloc();
    pubnub_t*          pbp  = pubnub_alloc();

    if (NULL == pbp || NULL == gtp) {
        printf("Failed to allocate Pubnub context!\n");
        return -1;
    }

    char* my_env_publish_key = getenv("PUBNUB_PUBLISH_KEY");
    char* my_env_subscribe_key = getenv("PUBNUB_SUBSCRIBE_KEY");
    char* my_env_secret_key = getenv("PUBNUB_SECRET_KEY");

    if (NULL == my_env_publish_key) { my_env_publish_key = "demo"; }
    if (NULL == my_env_subscribe_key) { my_env_subscribe_key = "demo"; }
    if (NULL == my_env_secret_key) { my_env_secret_key = "demo"; }

    printf("%s\n%s\n%s\n",my_env_publish_key,my_env_subscribe_key,my_env_secret_key);

    pubnub_init(gtp, my_env_publish_key, my_env_subscribe_key);
    pubnub_set_secret_key(gtp, my_env_secret_key);

    pubnub_set_transaction_timeout(gtp, PUBNUB_DEFAULT_SUBSCRIBE_TIMEOUT);

    /* Leave this commented out to use the default - which is
       blocking I/O on most platforms. Uncomment to use non-
       blocking I/O.
    */
    pubnub_set_non_blocking_io(gtp);

    generate_user_id(gtp);

    puts("Grant Token...");
    time(&t0);
    struct pam_permission h_perm = {.read=true, .write=true };
    int perm_hello_world = pubnub_get_grant_bit_mask_value(h_perm);
    struct pam_permission cg_perm = {.read=true, .write=true, .manage=true};
    int perm_channel_group = pubnub_get_grant_bit_mask_value(cg_perm);
    int ttl_minutes = 60;
    char perm_obj[2000];
    char* authorized_user_id = "my_authorized_uuid";
    sprintf(perm_obj,"{\"ttl\":%d, \"uuid\":\"%s\", \"permissions\":{\"resources\":{\"channels\":{ \"mych\":31, \"%s\":%d }, \"groups\":{ \"mycg\":31, \"channel-group\":%d }, \"users\":{ \"myuser\":31 }, \"spaces\":{ \"myspc\":31 }}, \"patterns\":{\"channels\":{ }, \"groups\":{ }, \"users\":{ \"^$\":1 }, \"spaces\":{ \"^$\":1 }},\"meta\":{ }}}", ttl_minutes, authorized_user_id, chan, perm_hello_world, perm_channel_group);
    res = pubnub_grant_token(gtp, perm_obj);
    char* grant_token = NULL;
    if (PNR_STARTED == res) {
        res = pubnub_await(gtp);
        if (PNR_OK == res) {
            //const char* grant_token_resp_json = pubnub_get(gtp);
            //printf("pubnub_grant_token() JSON Response from Pubnub: %s\n", grant_token_resp_json);
            // OR below code
            pubnub_chamebl_t grant_token_resp;
            grant_token_resp = pubnub_get_grant_token(gtp);
            grant_token = strdup(grant_token_resp.ptr);
            printf("pubnub_grant_token() Response from Pubnub: %s\n", grant_token);

            char* cbor_data = pubnub_parse_token(gtp, grant_token);
            printf("pubnub_parse_token() = %s\n", cbor_data);
            free(cbor_data);
        }
        else{
            printf("pubnub_grant_token() failed with code: %d('%s')\n",
                res,
                pubnub_res_2_string(res));
        }
    }
    else{
        printf("pubnub_grant_token() failed with code: %d('%s')\n",
               res,
               pubnub_res_2_string(res));
    }

    if (grant_token == NULL) {
        return 1;
    }

    pubnub_init(pbp, my_env_publish_key, my_env_subscribe_key);
    pubnub_set_auth_token(pbp, grant_token);
    pubnub_set_transaction_timeout(gtp, PUBNUB_DEFAULT_SUBSCRIBE_TIMEOUT);


    /* Leave this commented out to use the default - which is
       blocking I/O on most platforms. Uncomment to use non-
       blocking I/O.
    */
    pubnub_set_non_blocking_io(pbp);

    generate_user_id(pbp);

    puts("Publishing...");
    time(&t0);
    res = pubnub_publish(pbp, chan, "\"Hello world from sync!\"");
    if (PNR_STARTED == res) {
        res = pubnub_await(pbp);
    }
    printf("Publish lasted %lf seconds.\n", difftime(time(NULL), t0));
    if (PNR_OK == res) {
        printf("Published! Response from Pubnub: %s\n",
               pubnub_last_publish_result(pbp));
    }
    else if (PNR_PUBLISH_FAILED == res) {
        printf("Published failed on Pubnub, description: %s\n",
               pubnub_last_publish_result(pbp));
    }
    else {
        printf("Publishing failed with code: %d('%s')\n",
               res,
               pubnub_res_2_string(res));
    }

    puts("Revoking token...");
    time(&t0);
    res = pubnub_revoke_token(gtp, grant_token);
    if (PNR_STARTED == res) {
        res = pubnub_await(gtp);
    }
    printf("Token revoke lasted %lf seconds.\n", difftime(time(NULL), t0));
    if (PNR_OK == res) {
        printf("Access token permissions revoked! Response from Pubnub: %s\n",
               pubnub_get_revoke_token_response(gtp).ptr);
    }
    else if (PNR_PUBLISH_FAILED == res) {
        printf("pubnub_revoke_token() failed with code: %d('%s')\n",
               res,
               pubnub_res_2_string(res));
    }
    else {
        printf("pubnub_revoke_token() failed with code: %d('%s')\n",
               res,
               pubnub_res_2_string(res));
    }

    time(&t0);
    #ifdef _WIN32
    Sleep(70000); //70 seconds
    #else
    sleep(70); //70 seconds
    #endif
    printf("Slept for %lf seconds.\n", difftime(time(NULL), t0));
    puts("Publishing after revoke...");
    time(&t0);
    res = pubnub_publish(pbp, chan, "\"Hello world from sync!\"");
    if (PNR_STARTED == res) {
        res = pubnub_await(pbp);
    }
    printf("Publish lasted %lf seconds.\n", difftime(time(NULL), t0));
    if (PNR_OK == res) {
        printf("Unexpected publish success (request should fail)! Response from Pubnub: %s\n",
               pubnub_last_publish_result(pbp));
    }
    else if (PNR_PUBLISH_FAILED == res) {
        printf("Expected publish failure on Pubnub as expected, description: %s\n",
               pubnub_last_publish_result(pbp));
    }
    else {
        printf("Expected publish failure with code: %d('%s')\n",
               res,
               pubnub_res_2_string(res));
    }


    /* We're done, but, keep-alive might be on, so we need to cancel,
     * then free */
    free(grant_token);
    sync_sample_free(pbp);
    sync_sample_free(gtp);

    puts("Pubnub sync revoke token demo over.");

    return 0;
}
