/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_sync.h"

#include "pubnub_sync_subscribe_loop.h"
#include "pubnub_helper.h"

#include <stdio.h>


int main()
{
    pubnub_t *pbp = pubnub_alloc();
    struct pubnub_subloop_descriptor pbsld;

    if (NULL == pbp) {
        printf("Failed to allocate Pubnub context!\n");
        return -1;
    }
    pubnub_init(pbp, "demo", "demo");

//! [Define subscribe loop]
    pbsld = pubnub_subloop_define(pbp, "hello_world");
//! [Define subscribe loop]

    printf("Entering subscribe loop for channel '%s'...\n", pbsld.channel);

//! [Subscribe loop]
    for (;;) {
        char const* msg;
        enum pubnub_res pbres = pubnub_subloop_fetch(&pbsld, &msg);
        if (PNR_OK != pbres) {
            printf("Exiting subscribe loop because of error: %d\n", pbres);
            break;
        }
        if (NULL == msg) {
            puts("Everything's OK, yet no message received");
        }
        else {
            printf("Got message '%s'\n", msg);
        }
    }
//! [Subscribe loop]

    /* We're done */
    if (pubnub_free(pbp) != 0) {
        printf("Failed to free the Pubnub context\n");
    }

    puts("Pubnub sync subloop demo over.");

    return 0;
}
