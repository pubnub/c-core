/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_sync.h"

#include "core/pubnub_sync_subscribe_loop.h"
#include "core/pubnub_helper.h"

#include <stdio.h>
#include <time.h>


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
    pubnub_t *pbp = pubnub_alloc();
    struct pubnub_subloop_descriptor pbsld;
    time_t seconds_in_loop = 60;
    time_t start = time(NULL);

    if (NULL == pbp) {
        printf("Failed to allocate Pubnub context!\n");
        return -1;
    }
    pubnub_init(pbp, "demo", "demo");
    pubnub_set_user_id(pbp, "demo");

    /* Leave this commented out to use the default - which is
       blocking I/O on most platforms. Uncomment to use non-
       blocking I/O.
    */
    pubnub_set_non_blocking_io(pbp);

//! [Define subscribe loop]
    pbsld = pubnub_subloop_define(pbp, "hello_world");
//! [Define subscribe loop]

    printf("Entering subscribe loop for channel '%s'...\n", pbsld.channel);

//! [Subscribe loop]
    for (;;) {
        char const* msg;
        enum pubnub_res pbres = pubnub_subloop_fetch(&pbsld, &msg);
        if (PNR_OK != pbres) {
            printf("Exiting subscribe loop because of error: %d('%s')\n",
                   pbres,
                   pubnub_res_2_string(pbres));
            break;
        }
        if (NULL == msg) {
            puts("Everything's OK, yet no message received");
        }
        else {
            printf("Got message '%s'\n", msg);
        }
        if(difftime(time(NULL), start) > seconds_in_loop) {
            break;
        }
    }
//! [Subscribe loop]

    /* We're done, but, if keep-alive is on, we can't free,
       we need to cancel first...
     */
    sync_sample_free(pbp);

    puts("Pubnub sync subloop demo over.");

    return 0;
}
