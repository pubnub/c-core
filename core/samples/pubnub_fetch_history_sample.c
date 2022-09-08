/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#define PUBNUB_USE_FETCH_HISTORY 1
#include "pubnub_sync.h"
#if PUBNUB_USE_FETCH_HISTORY
#include "core/pubnub_fetch_history.c"
#endif
#include "core/pubnub_helper.h"
#include "core/pubnub_timers.h"

#include <stdio.h>
#include <time.h>

/* If you don't like these channel names you can change them */
static char* m_channel1 = "my_channel1";
static char* m_channel2 = "my_channel2";



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


static void publish_on_channels(pubnub_t* pbp)
{
    //Publish for channel = m_channel1
    puts("-----------------------");
    puts("Publishing for variable m_channel1...");
    puts("-----------------------");
    for (int i = 0; i < 10; i++) {
        enum pubnub_res res;
        char msg[100];
        sprintf(msg,"\"Hello world from fetch history sample %d !\"", i);
        res = pubnub_publish(pbp,
                                m_channel1,
                                msg);
        if (res == PNR_STARTED) {
            puts("Await publish");
            res = pubnub_await(pbp);
        }
        if (PNR_OK == res) {
            printf("Published! Response from Pubnub: %s\n",
                    pubnub_last_publish_result(pbp));
        }
        else if (PNR_PUBLISH_FAILED == res) {
            printf("Published failed on Pubnub, description: %s\n",
                    pubnub_last_publish_result(pbp));
        }
        else {
            printf("Publishing failed with code: %d('%s')\n", res, pubnub_res_2_string(res));
        }
    }

    //Publish for channel = m_channel2
    puts("-----------------------");
    puts("Publishing for variable m_channel2...");
    puts("-----------------------");
    for (int i = 0; i < 10; i++) {
        enum pubnub_res res;
        char msg[100];
        sprintf(msg,"\"Hello world from fetch history sample %d !\"", i);
        res = pubnub_publish(pbp,
                                m_channel2,
                                msg);
        if (res == PNR_STARTED) {
            puts("Await publish");
            res = pubnub_await(pbp);
        }
        if (PNR_OK == res) {
            printf("Published! Response from Pubnub: %s\n",
                    pubnub_last_publish_result(pbp));
        }
        else if (PNR_PUBLISH_FAILED == res) {
            printf("Published failed on Pubnub, description: %s\n",
                    pubnub_last_publish_result(pbp));
        }
        else {
            printf("Publishing failed with code: %d('%s')\n", res, pubnub_res_2_string(res));
        }
    }

}

int main(int argc, char* argv[])
{
    enum pubnub_res res;
    char            string_channels[500];
    pubnub_t*       pbp   = pubnub_alloc();
    
    if (NULL == pbp) {
        printf("Failed to allocate Pubnub context!\n");
        return -1;
    }

    char* my_env_publish_key = getenv("PUBNUB_PUBLISH_KEY");
    char* my_env_subscribe_key = getenv("PUBNUB_SUBSCRIBE_KEY");
    char* my_env_secret_key = getenv("PUBNUB_SECRET_KEY");

    if (NULL == my_env_publish_key) { my_env_publish_key = "demo"; }
    if (NULL == my_env_subscribe_key) { my_env_subscribe_key = "demo"; }
    if (NULL == my_env_secret_key) { my_env_secret_key = "demo"; }

    
    pubnub_init(pbp, my_env_publish_key, my_env_subscribe_key);
    generate_user_id(pbp);

    publish_on_channels(pbp);
    snprintf(string_channels,
             sizeof string_channels,
             "%s,%s",
             m_channel1,
             m_channel2);    

    struct pubnub_fetch_history_options opt = pubnub_fetch_history_defopts();
    opt.include_message_type = true;
    opt.include_meta = true;
    res = pubnub_fetch_history(pbp, string_channels, opt);
    if (res == PNR_STARTED) {
            puts("Await Fetch History");
            res = pubnub_await(pbp);
    }
    if (PNR_OK == res) {
            printf("Got response for Fetch History! Response from Pubnub: %s\n",
                    pubnub_get_fetch_history(pbp).ptr);
    }
    else{
        printf("pubnub_fetch_history() failed with code: %d('%s')\n",
               res,
               pubnub_res_2_string(res));
    }
    sync_sample_free(pbp);

    puts("Pubnub fetch history demo over.");

    return 0;
}
