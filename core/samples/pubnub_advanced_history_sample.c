/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_sync.h"

#include "core/pubnub_advanced_history.c"
#include "core/pubnub_helper.h"
#include "core/pubnub_timers.h"

#include <stdio.h>
#include <time.h>

/* If you don't like these channel names you can change them,
   but they have to remain the same in number */
static char* m_channel[] = {"pool_player",
                            "lucky_hand",
                            "wild_card",
                            "fast_draw",
                            "long_shot"};
static int m_msg_sent[sizeof m_channel/sizeof m_channel[0]]
                     [sizeof m_channel/sizeof m_channel[0]];
/* List of timetoken indexes for which pubnub_message_count() query is requested
   corresponding to the channel list declared above.
   You can change these values as long as they are in the range of channel
   indexes.(Number of offered timetokens in this example is the same as
   the number of channels.- Use numbers from 1 to number_of_channels)
 */
static int m_timetoken_index[sizeof m_channel/sizeof m_channel[0]] = {2, 1, 4, 3, 2};
static char m_timetokens[sizeof m_channel/sizeof m_channel[0] + 1][30];


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


static void wait_useconds(unsigned long time_in_microseconds)
{
    double d_time = (double)time_in_microseconds / (1000000.0 / (double)CLOCKS_PER_SEC);
    time_in_microseconds = (unsigned long)d_time;

    clock_t  start = clock();
    unsigned long time_passed_in_microseconds;
    do {
        time_passed_in_microseconds = clock() - start;
    } while (time_passed_in_microseconds < time_in_microseconds);
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


static int get_timetoken(pubnub_t* pbp, char* timetoken)
{
    enum pubnub_res res;

    puts("-----------------------");
    puts("Getting time...");
    puts("-----------------------");
    res = pubnub_time(pbp);
    if (res == PNR_STARTED) {
        res = pubnub_await(pbp);
    }
    if (PNR_OK == res) {
        strcpy(timetoken, pubnub_get(pbp));
        printf("Gotten time: '%s'\n", timetoken);
    }
    else {
        printf("Getting time failed with code %d('%s')\n",
               res,
               pubnub_res_2_string(res));
        return -1;
    }

    return 0;
}


static void print_msg_counts_table(int n)
{
    int i;
    
    puts("--------------------------------------------message counts table-----------------------------------------");
    printf("               \\channels:  %s  |  %s   |   %s   |   %s   |   %s   |\n",
           m_channel[0],
           m_channel[1],
           m_channel[2],
           m_channel[3],
           m_channel[4]);
    for (i = 0 ; i < n; i++) {
        printf("tt[%d]'%s':       %d       |       %d       |       %d       |       %d       |       %d       |\n",
               i + 1,
               m_timetokens[i],
               m_msg_sent[i][0],
               m_msg_sent[i][1],
               m_msg_sent[i][2],
               m_msg_sent[i][3],
               m_msg_sent[i][4]);        
    }
}


static void publish_on_channels(pubnub_t* pbp)
{
    int n = sizeof m_channel/sizeof m_channel[0];
    int i;
    
    for (i = 0; i < n; i++) {
        int j;        
        while (get_timetoken(pbp, m_timetokens[i]) != 0) {
            /* wait in microseconds */
            wait_useconds(1000);
        }
        for (j = i; j < n; j++) {
            enum pubnub_res res;
            puts("-----------------------");
            puts("Publishing...");
            puts("-----------------------");
            res = pubnub_publish(pbp,
                                 m_channel[j],
                                 "\"Hello world from message_counts sample!\"");
            if (res == PNR_STARTED) {
                puts("Await publish");
                res = pubnub_await(pbp);
            }
            if (PNR_OK == res) {
                printf("Published! Response from Pubnub: %s\n",
                       pubnub_last_publish_result(pbp));
                m_msg_sent[i][j] = 1;
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
    print_msg_counts_table(n);
}

/* calculates internal message counts used to compare against information
   obtained from the transaction response for single timetoken
 */
static void calculate_internal_msg_counts_for_a_single_timetoken(int* internal_msg_counts,
                                                                 int n,
                                                                 int index_timetoken)
{
    int j;
    /* Internal message count used to compare against information obtained from response */
    for (j = index_timetoken - 1; j < n; j++) {
        int k;
        for (k = index_timetoken - 1; k <= j; k++) {
            internal_msg_counts[j] += m_msg_sent[k][j];
        }
    }
}

/* calculates internal message counts used to compare against information
   obtained from the transaction response for the timetoken list
 */
static void calculate_internal_msg_counts_for_timetoken_list(int* internal_msg_counts, int  n)
{
    int j;
    for (j = 0; j < n; j++) {
        int k;
        for (k = m_timetoken_index[j] - 1; k <= j; k++) {
            internal_msg_counts[j] += m_msg_sent[k][j];
        }
    }
}


static void print_message_counts(pubnub_t* pbp,
                                 int index_timetoken,
                                 int* msg_counts,
                                 int* internal_msg_counts)
{
    int n = sizeof m_channel/sizeof m_channel[0];
    int j;
    if (index_timetoken > 0) {
        printf("tt[%d]='%s':", index_timetoken, m_timetokens[index_timetoken - 1]);
    }
    else {
        printf("tt[%d]='%s'|tt[%d]='%s'|tt[%d]='%s'|tt[%d]='%s'|tt[%d]='%s'|\n",
               m_timetoken_index[0],
               m_timetokens[m_timetoken_index[0] - 1],
               m_timetoken_index[1],
               m_timetokens[m_timetoken_index[1] - 1],
               m_timetoken_index[2],
               m_timetokens[m_timetoken_index[2] - 1],
               m_timetoken_index[3],
               m_timetokens[m_timetoken_index[3] - 1],
               m_timetoken_index[4],
               m_timetokens[m_timetoken_index[4] - 1]);
    }
    for (j = 0 ; j < n; j++) {
        if ((msg_counts[j] > 0) && (msg_counts[j] != internal_msg_counts[j])) {
            printf("Message counter mismatch! - "
                   "msg_counts[%d]=%d, "
                   "internal_msg_counts[%d]=%d |",
                   j,
                   msg_counts[j],
                   j,
                   internal_msg_counts[j]);
        }
        else {
            printf("%s   %d   %s|",
                   (index_timetoken > 0) ? "" : "         ",
                   msg_counts[j],
                   (index_timetoken > 0) ? "" : "         ");
        }
    }
    putchar('\n');
}


static void await_msg_counts_and_print_result(pubnub_t*       pbp,
                                              char*           string_channels,
                                              enum pubnub_res res,
                                              time_t          t0,
                                              int             index_timetoken,
                                              int*            internal_msg_counts)
{
    int msg_counts[sizeof m_channel/sizeof m_channel[0]];

    if (res == PNR_STARTED) {
        res = pubnub_await(pbp);
    }
    printf("Getting message counts lasted %lf seconds.\n", difftime(time(NULL), t0));
    if (PNR_OK == res) {
        if (pubnub_get_chan_msg_counts_size(pbp) == sizeof m_channel/sizeof m_channel[0]) {
            puts("-----------------------------------Got message counts for all channels!----------------------------------");
        }
        pubnub_get_message_counts(pbp, string_channels, msg_counts);
        print_message_counts(pbp,
                             index_timetoken,
                             msg_counts,
                             internal_msg_counts);
    }
    else {
        printf("Getting message counts failed with code: %d('%s')\n",
               res,
               pubnub_res_2_string(res));
    }
}


static void get_message_counts_for_a_single_timetoken(pubnub_t* pbp,
                                                      char* string_channels,
                                                      int index_timetoken)
{
    time_t t0;
    int    internal_msg_counts[sizeof m_channel/sizeof m_channel[0]] = {0};

    calculate_internal_msg_counts_for_a_single_timetoken(internal_msg_counts,
                                                         sizeof m_channel/sizeof m_channel[0],
                                                         index_timetoken);
    puts("------------------------------------------------");
    puts("Getting message counts for a single timetoken...");
    puts("------------------------------------------------");

    time(&t0);
    await_msg_counts_and_print_result(
        pbp,
        string_channels,
        pubnub_message_counts(pbp,
                              string_channels,
                              m_timetokens[index_timetoken - 1]),
        t0,
        index_timetoken,
        internal_msg_counts);
}


static void get_message_counts_for_the_list_of_timetokens(pubnub_t* pbp,
                                                          char* string_channels)
{
    time_t t0;
    char   string_timetokens[150];
    int    internal_msg_counts[sizeof m_channel/sizeof m_channel[0]] = {0};

    calculate_internal_msg_counts_for_timetoken_list(internal_msg_counts,
                                                     sizeof m_channel/sizeof m_channel[0]);
    snprintf(string_timetokens,
             sizeof string_timetokens,
             "%s,%s,%s,%s,%s",
             m_timetokens[m_timetoken_index[0] - 1],
             m_timetokens[m_timetoken_index[1] - 1],
             m_timetokens[m_timetoken_index[2] - 1],
             m_timetokens[m_timetoken_index[3] - 1],
             m_timetokens[m_timetoken_index[4] - 1]);
    puts("----------------------------------------------------");
    puts("Getting message counts for the list of timetokens...");
    puts("----------------------------------------------------");

    time(&t0);
    await_msg_counts_and_print_result(
        pbp,
        string_channels,
        pubnub_message_counts(pbp, string_channels, string_timetokens),
        t0,
        0,
        internal_msg_counts);
}


int main(int argc, char* argv[])
{
    char            string_channels[500];
    char const*     pubkey = (argc > 1) ? argv[1] : "demo";
    char const*     keysub = (argc > 2) ? argv[2] : "demo";
    char const*     origin = (argc > 3) ? argv[3] : "pubsub.pubnub.com";
    pubnub_t*       pbp   = pubnub_alloc();
    pubnub_t*       pbp_2 = pubnub_alloc();
    
    if ((NULL == pbp) || (NULL == pbp_2)) {
        printf("Failed to allocate Pubnub context!\n");
        return -1;
    }
    
    pubnub_init(pbp, pubkey, keysub);
    pubnub_init(pbp_2, pubkey, keysub);
    generate_user_id(pbp);
    generate_user_id(pbp_2);
    pubnub_origin_set(pbp, origin);
    pubnub_origin_set(pbp_2, origin);

    publish_on_channels(pbp);
    wait_useconds(1500000);
    /* getting last timetoken after all messages had been sent */
    get_timetoken(pbp, m_timetokens[5]);
    snprintf(string_channels,
             sizeof string_channels,
             "%s,%s,%s,%s,%s",
             m_channel[0],
             m_channel[1],
             m_channel[2],
             m_channel[3],
             m_channel[4]);    
    get_message_counts_for_a_single_timetoken(pbp_2, string_channels, m_timetoken_index[0]);
    get_message_counts_for_the_list_of_timetokens(pbp_2, string_channels);
    /* Use of the last timetoken acquired */
    get_message_counts_for_a_single_timetoken(pbp_2,
                                              string_channels,
                                              sizeof m_timetokens/sizeof m_timetokens[0]);
    
    sync_sample_free(pbp_2);
    sync_sample_free(pbp);

    puts("Pubnub message_counts demo over.");

    return 0;
}
