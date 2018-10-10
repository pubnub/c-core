/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_sync.h"

#include "core/pubnub_coreapi_ex.h"
#include "core/pubnub_subscribe_v2.h"
#include "core/pubnub_json_parse.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>


static enum pubnub_res publish(pubnub_t* pbp, char const* chan, char const* meta)
{
    enum pubnub_res               res;
    struct pubnub_publish_options pubopts;

    puts("------------------");
    puts("Publish w/metadata");
    puts("------------------");
    pubopts      = pubnub_publish_defopts();
    pubopts.meta = meta;
    res = pubnub_publish_ex(pbp, chan, "\"Hello world from sync w/meta\"", pubopts);
    if (PNR_STARTED == res) {
        res = pubnub_await(pbp);
    }
    return res;
}


static int printout_subscribe_v2_outcome(pubnub_t* pbp, enum pubnub_res res)
{
    struct pubnub_v2_message msg;
    if (PNR_OK != res) {
        printf("Subscribe failed, result=%d\n", res);
        return -1;
    }
    for (msg = pubnub_get_v2(pbp); msg.payload.size > 0; msg = pubnub_get_v2(pbp)) {
        puts("Received message:");
        printf("  Channel    = '%.*s'\n", (int)msg.channel.size, msg.channel.ptr);
        printf("  Timetoken  = '%.*s'\n", (int)msg.tt.size, msg.tt.ptr);
        printf("  Metadata   = '%.*s'\n", (int)msg.metadata.size, msg.metadata.ptr);
        printf("  Payload    = '%.*s'\n", (int)msg.payload.size, msg.payload.ptr);
    }
    return 0;
}


static int doit(pubnub_t* pbp)
{
    enum pubnub_res                    res;
    char const*                        chan = "hello_meta";
    struct pubnub_subscribe_v2_options subopts;

    pubnub_init(pbp, "demo", "demo");
    srand((unsigned)time(NULL));

    puts("----------------------------");
    puts("First Subscribe ('connect').");
    puts("----------------------------");
    res = pubnub_subscribe(pbp, chan, NULL);
    if (PNR_STARTED == res) {
        res = pubnub_await(pbp);
    }
    if (PNR_OK != res) {
        printf("First subscribe failed, result=%d\n", res);
        return -1;
    }

    res = publish(pbp, chan, "{\"pub\": \"nub\"}");
    if (PNR_OK != res) {
        printf("publish failed, result=%d\n", res);
        return -1;
    }

    puts("------------------------------------");
    puts("Subscribe w/filter that's satisfied.");
    puts("------------------------------------");
    subopts             = pubnub_subscribe_v2_defopts();
    subopts.filter_expr = "pub == 'nub'";
    res                 = pubnub_subscribe_v2(pbp, chan, subopts);
    if (PNR_STARTED == res) {
        res = pubnub_await(pbp);
    }
    if (0 != printout_subscribe_v2_outcome(pbp, res)) {
        return -1;
    }

    res = publish(pbp, chan, "{\"pub\": \"sub\"}");
    if (PNR_OK != res) {
        printf("publish failed, result=%d\n", res);
        return -1;
    }

    puts("----------------------------------------");
    puts("Subscribe w/filter that's NOT satisfied.");
    puts("----------------------------------------");
    subopts             = pubnub_subscribe_v2_defopts();
    subopts.filter_expr = "pub == 3";
    res                 = pubnub_subscribe_v2(pbp, chan, subopts);
    if (PNR_STARTED == res) {
        res = pubnub_await(pbp);
    }
    if (0 != printout_subscribe_v2_outcome(pbp, res)) {
        return -1;
    }

    return 0;
}


int main()
{
    int       rslt;
    pubnub_t* pbp = pubnub_alloc();
    if (NULL == pbp) {
        printf("Failed to allocate Pubnub context!\n");
        return -1;
    }

    rslt = doit(pbp);

    if (pubnub_free(pbp) != 0) {
        printf("Failed to free the Pubnub context!\n");
    }

    puts("Pubnub metadata sync demo over.");

    return rslt;
}
