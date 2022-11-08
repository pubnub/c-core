/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_sync.h"

#include "core/pubnub_coreapi_ex.h"
#include "core/pubnub_helper.h"
#include "core/pubnub_timers.h"

#include <stdio.h>
#include <string.h>
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


static struct pubnub_publish_options publish_opts_method(enum pubnub_method method)
{
    struct pubnub_publish_options opts = pubnub_publish_defopts();
    opts.method = method;
    return opts;
}


/** Allocates memory for message to be published and initiates 'publish via post' transaction.
    This memory should be freed after the publish 'via post' transaction is finished, not before.
    (In callback environment we can free message memory from callback.)
    Publish via post could not work if this function passed pointer to lacal char array insead
    of pointer to dynamically allocated memory.
 */
static int alloc_and_start_publish_via_post(pubnub_t*        pb,
                                            char const*      channel,
                                            char**           allocated,
                                            enum pubnub_res* publish_res)
{
    char time_string[100];
    char* mem;
    time_t rawtime;
    struct tm* timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    /* Making json object */
    sprintf(time_string,
            "\"Local date and time(published via post)-%s",
            asctime(timeinfo)); 
    /* Replacing 'new line' with quotes */
    time_string[strlen(time_string) - 1] = '\"';
    mem = malloc(strlen(time_string) + 1);
    if (NULL == mem) {
        return -1;
    }
    strcpy(mem, time_string);
    *publish_res = pubnub_publish_ex(pb,
                                     channel,
                                     mem,
                                     publish_opts_method(pubnubSendViaPOST));
    *allocated = mem;
    return 0;
}


int main()
{
    time_t          t0;
    enum pubnub_res res;
    char const*     chan = "hello_world";
    char const*     user_id = "demo";
    char*           allocated;
    char            message_via_post_for_gzip[] = "\"Hello world from publish via post(with GZIP) sample! - Hello world from publish via post(with GZIP) sample! - Hello world from publish via post(with GZIP) sample! - Hello world from publish via post(with GZIP) sample! - Hello world from publish via post(with GZIP) sample! - Hello world from publish via post(with GZIP) sample! - Hello world from publish via post(with GZIP) sample! - Hello world from publish via post(with GZIP) sample! - Hello world from publish via post(with GZIP) sample! - Hello world from publish via post(with GZIP) sample! - Hello world from publish via post(with GZIP) sample! - Hello world from publish via post(with GZIP) sample!\"";
    pubnub_t*       pbp  = pubnub_alloc();

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
    printf("%s\n%s\n%s\n",my_env_publish_key,my_env_subscribe_key,my_env_secret_key);

    pubnub_init(pbp, my_env_publish_key, my_env_subscribe_key);
    pubnub_set_user_id(pbp, user_id);

    pubnub_set_transaction_timeout(pbp, PUBNUB_DEFAULT_NON_SUBSCRIBE_TIMEOUT);

    /* Leave this commented out to use the default - which is
       blocking I/O on most platforms. Uncomment to use non-
       blocking I/O.
    */
    pubnub_set_non_blocking_io(pbp);

    puts("Publishing(via post)...");
    time(&t0);
    if (-1 == alloc_and_start_publish_via_post(
                  pbp,
                  chan,
                  &allocated,
                  &res)) {
        puts("Failed to allocate memory for 'publish via post' message!");

        return -1;
    }
    if (PNR_STARTED == res) {
        res = pubnub_await(pbp);
    }
    free(allocated);
    printf("Publish via post lasted %lf seconds.\n", difftime(time(NULL), t0));
    if (PNR_OK == res) {
        printf("Published! Response from Pubnub: %s\n",
               pubnub_last_publish_result(pbp));
    }
    else if (PNR_PUBLISH_FAILED == res) {
        printf("Publishing failed on Pubnub, description: %s\n",
               pubnub_last_publish_result(pbp));
    }
    else {
        printf("Publishing failed with code: %d('%s')\n",
               res,
               pubnub_res_2_string(res));
    }

    puts("=========================================");
    /* Publishing via post with gzip */
    puts("Publishing(via post with gzip)...");
    time(&t0);
    /* Since the message is packed in pubnub context buffer before the publish transaction
       effectively starts we can do it directly(using string literal), or from a local char array.
     */
    res = pubnub_publish_ex(pbp,
                            chan,
                            message_via_post_for_gzip,
                            publish_opts_method(pubnubSendViaPOSTwithGZIP));
    if (PNR_STARTED == res) {
        res = pubnub_await(pbp);
    }
    printf("Publish 'via post with gzip' lasted %lf seconds.\n", difftime(time(NULL), t0));
    if (PNR_OK == res) {
        printf("Published! Response from Pubnub: %s\n",
               pubnub_last_publish_result(pbp));
    }
    else if (PNR_PUBLISH_FAILED == res) {
        printf("Publishing failed on Pubnub, description: %s\n",
               pubnub_last_publish_result(pbp));
    }
    else {
        printf("Publishing failed with code: %d('%s')\n",
               res,
               pubnub_res_2_string(res));
    }

    sync_sample_free(pbp);

    puts("Pubnub publish 'via post' demo over.");

    return 0;
}
