/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_alloc.h"
#include "pubnub_coreapi.h"
#include "pubnub_version.h"
#include "pubnub_blocking_io.h"

#include "pnc_config.h"
#include "pnc_helpers.h"
#include "pnc_readers.h"
#include "pnc_ops_callback.h"
#include "pnc_subscriptions.h"


#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>


static void displayMenuOptions(pubnub_t *pn);


#define STRINGIFY_2(X) #X
#define STRINGIFY(X) STRINGIFY_2(X)

#define MENU_DISPLAY 0

#define MENU_PUBLISH 1
#define MENU_HISTORY 2
#define MENU_HERE_NOW 3
#define MENU_TIME 4
#define MENU_AUTH 5
#define MENU_UUID 6  
#define MENU_STATE_GET 7
#define MENU_SET_STATE 8
#define MENU_WHERE_NOW 9
#define MENU_ADD_PRESENCE_CHANNEL 10
#define MENU_SUBSCRIBE 11
#define MENU_UNSUBSCRIBE 12
#define MENU_SUBSCRIPTION_ADD_CHANNEL    13
#define MENU_SUBSCRIPTION_ADD_GROUP      14
#define MENU_SUBSCRIPTION_REMOVE_CHANNEL 15
#define MENU_SUBSCRIPTION_REMOVE_GROUP   16
#define MENU_GROUP_ADD_CHANNEL    17
#define MENU_GROUP_REMOVE_CHANNEL 18
#define MENU_GROUP_LIST_CHANNELS  19
#define MENU_GROUP_REMOVE_GROUP   20

#define MENU_EXIT 99

static const char pubkey[] = "demo";
static const char subkey[] = "demo";
static const char channel[] = "hello_world";


int main()
{
    int option;
    char opt_string[4];
    char channel[PNC_CHANNEL_NAME_SIZE];
    char channel_group[PNC_CHANNEL_NAME_SIZE];
    char auth_key[PNC_AUTH_KEY_SIZE];
    char uuid[PNC_UUID_SIZE + 1];
    char state[2048];
    pubnub_t *pn = pubnub_alloc();
    pubnub_t *pn_sub = pubnub_alloc();
    
    if (NULL == pn || NULL == pn_sub) {
        printf("Failed to allocate pubnub context");
        return -1;
    }
    
    pubnub_init(pn, pubkey, pubkey);
    pubnub_init(pn_sub, pubkey, pubkey);
    pubnub_set_uuid(pn, PNC_DEFAULT_UUID);
    pubnub_set_uuid(pn, PNC_DEFAULT_SUBSCRIBE_UUID);

    pnc_ops_init(pn, pn_sub);
    
    displayMenuOptions(pn);
    
    option = 0;
    
    while (option != MENU_EXIT) {
        fgets(opt_string, sizeof(opt_string), stdin);
        opt_string[strlen(opt_string) - 1] = '\0';
        option = atoi(opt_string);
        printf("You entered %d(%s)\n", option, opt_string);
        
        switch (option) {
        case MENU_DISPLAY:
            break;
        case MENU_PUBLISH:
        {
            char message[PNC_READ_STRING_SIZE];
            bool store = false;
            // TODO: add json built string
            
            pnc_read_string_from_console("Channel Name",
                                         channel, PNC_CHANNEL_NAME_SIZE);
            pnc_read_bool_from_console_optional("Store", &store, true);
            
            puts("Enter the message for publish. To exit loop enter Q");

            while (fgets(message, PNC_READ_STRING_SIZE, stdin) != NULL && strcmp(message, "Q\n")) {
                if (strlen(message) == 1) {
                    puts("Invalid input");
                    continue;
                }
                
                message[strlen(message) - 1] = '\0';
                
                if (message[0] == '{' && message[strlen(message) - 1] == '}') {
                    pnc_ops_parse_response("pubnub_publish()",
                                           pubnub_publish(pn, channel, message), pn);
                }
                else {
                    char encoded[PNC_READ_STRING_SIZE];
                    snprintf(encoded, sizeof encoded,  "\"%s\"", message);
                    pnc_ops_parse_response("pubnub_publish()",
                                           pubnub_publish(pn, channel, encoded), pn);
                }
                
                puts("Enter the message for publish. To exit loop enter Q.");
            }
            break;
        }
        
        case MENU_HISTORY:
        {
            int count = 100;
            bool include_token = false;
            
            pnc_read_int_from_console_optional("Count", &count, true);
            
            if (count <= 0) {
                count = 100;
                puts("Count value cannot be less than 1. Default value(100) was applied.");
            }
            
            pnc_read_string_from_console("Channel Name",
                                         channel, PNC_CHANNEL_NAME_SIZE);
            
            // WARNING: Default buffer can be not big enough to get 100-items
            // history response with timetokens
            pnc_read_bool_from_console("Include timetokens", &include_token);
            
            pnc_ops_parse_response("pubnub_history()",
                                   pubnub_history(pn, channel, count, include_token), pn);
            break;
        }
        
        case MENU_HERE_NOW:
            pnc_read_string_from_console("Channel Name",
                                                  channel, PNC_CHANNEL_NAME_SIZE);
            
            pnc_ops_parse_response("pubnub_here_now()",
                                   pubnub_here_now(pn, channel, NULL), pn);
            break;
        case MENU_TIME:
            pnc_ops_parse_response("pubnub_time()", pubnub_time(pn), pn);
            break;
        case MENU_AUTH:
            pnc_read_string_from_console("Auth key", auth_key, PNC_AUTH_KEY_SIZE);
            pubnub_set_auth(pn, auth_key);
            
            break;
        case MENU_UUID:
            pnc_read_string_from_console("UUID", uuid, PNC_UUID_SIZE);
            pubnub_set_uuid(pn, uuid);
            break;
        case MENU_STATE_GET:
            pnc_read_string_from_console("Channel Name",
                                         channel, PNC_CHANNEL_NAME_SIZE);
            pnc_read_string_from_console_optional("UUID",
                                                  uuid, PNC_UUID_SIZE, true);
            
            if (strlen(uuid) == 0) {
                strcpy(uuid, pubnub_uuid_get(pn));
            }
            
            pnc_ops_parse_response("pubnub_get_state()",
                                   pubnub_state_get(pn, channel, NULL, uuid), pn);
            break;
        case MENU_SET_STATE:
            // Set State
            pnc_read_string_from_console("Channel Name",
                                         channel, PNC_CHANNEL_NAME_SIZE);
            pnc_read_string_from_console("JSON state without escaping",
                                         state, PNC_READ_STRING_SIZE);
            pnc_ops_parse_response("pubnub_set_state()",
                                   pubnub_set_state(pn, channel, NULL, pubnub_uuid_get(pn), state), pn);
            break;
        case MENU_WHERE_NOW:
            pnc_read_string_from_console_optional("UUID", uuid, PNC_UUID_SIZE, true);
            if (strlen(uuid) == 0) {
                puts("Using current UUID");
                strcpy(uuid, PNC_DEFAULT_UUID);
            }
            
            pnc_ops_parse_response("pubnub_where_now()",
                                   pubnub_where_now(pn, uuid), pn);
            break;
        case MENU_ADD_PRESENCE_CHANNEL:
        {
            char presence_channel[PNC_CHANNEL_NAME_SIZE + sizeof(PNC_PRESENCE_SUFFIX)];

            pnc_read_string_from_console("Channel name", channel, PNC_CHANNEL_NAME_SIZE);
            strcpy(presence_channel, channel);
            strcat(presence_channel, PNC_PRESENCE_SUFFIX);
            
            pnc_subscribe_add_channel(presence_channel);
            break;
        }
        
        case MENU_SUBSCRIBE:
            pnc_ops_subscribe(pn_sub);
            break;
        case MENU_UNSUBSCRIBE:
            pnc_ops_unsubscribe(pn_sub);
            break;
        case MENU_SUBSCRIPTION_ADD_CHANNEL:
            pnc_read_string_from_console("Enter Channel name",
                                         channel, PNC_CHANNEL_NAME_SIZE);
            pnc_subscribe_add_channel(channel);
            break;
        case MENU_SUBSCRIPTION_ADD_GROUP:
            pnc_read_string_from_console("Enter Group name",
                                         channel, PNC_CHANNEL_NAME_SIZE);
            pnc_subscribe_add_channel_group(channel);
            break;
        case MENU_SUBSCRIPTION_REMOVE_CHANNEL:
            pnc_read_string_from_console("Enter Channel name",
                                         channel, PNC_CHANNEL_NAME_SIZE);
            pnc_subscribe_remove_channel(channel);
            break;
        case MENU_SUBSCRIPTION_REMOVE_GROUP:
            pnc_read_string_from_console("Enter Group name",
                                         channel, PNC_CHANNEL_NAME_SIZE);
            pnc_subscribe_remove_channel_group(channel);
            break;
        case MENU_GROUP_ADD_CHANNEL:
            pnc_read_string_from_console("Group name",
                                         channel_group, PNC_CHANNEL_NAME_SIZE);
            pnc_read_string_from_console("Channel to add",
                                         channel, PNC_CHANNEL_NAME_SIZE);
            
            pnc_ops_parse_response("pubnub_add_channel_to_group()",
                                   pubnub_add_channel_to_group(pn, channel, channel_group), pn);
            
            break;
        case MENU_GROUP_REMOVE_CHANNEL:
            pnc_read_string_from_console("Group name",
                                         channel_group, PNC_CHANNEL_NAME_SIZE);
            pnc_read_string_from_console("Channel to remove",
                                         channel, PNC_CHANNEL_NAME_SIZE);
            
            pnc_ops_parse_response("pubnub_remove_channel_from_group()",
                                   pubnub_remove_channel_from_group(pn, channel, channel_group), pn);
            
            break;
        case MENU_GROUP_LIST_CHANNELS:
            pnc_read_string_from_console("Group to list",
                                         channel_group, PNC_CHANNEL_NAME_SIZE);
            
            pnc_ops_parse_response("pubnub_list_channel_group()",
                                   pubnub_list_channel_group(pn, channel_group), pn);
            
            break;
        case MENU_GROUP_REMOVE_GROUP:
            pnc_read_string_from_console("Group to remove",
                                         channel, PNC_CHANNEL_NAME_SIZE);
            pubnub_remove_channel_group(pn, channel);
            break;
        default:
            printf("Invalid input: %d\n", option);
            break;
        }
        
        printf("\n================================\n\n");
        displayMenuOptions(pn);
    }
    
    puts("Exiting");
    
    if (pubnub_free(pn) != 0) {
        puts("Failed to free the Pubnub context");
    }
    
    return 0;
}


static void displayMenuOptions(pubnub_t *pn)
{
    char channels_string[PNC_SUBSCRIBE_CHANNELS_LIMIT * PNC_CHANNEL_NAME_SIZE];
    char channel_groups_string[PNC_SUBSCRIBE_CHANNELS_LIMIT * PNC_CHANNEL_NAME_SIZE];
    pnc_subscribe_list_channels(channels_string, sizeof channels_string);
    pnc_subscribe_list_channel_groups(channel_groups_string, sizeof channel_groups_string);

    printf("Active Channels/Channel Groups are: '%s'/'%s'\n\n",
           channels_string, channel_groups_string);
    
    puts("ENTER "STRINGIFY(MENU_PUBLISH)"  FOR Publish");
    puts("ENTER "STRINGIFY(MENU_HISTORY)"  FOR History");
    puts("ENTER "STRINGIFY(MENU_HERE_NOW)"  FOR Here Now");
    puts("ENTER "STRINGIFY(MENU_TIME)"  FOR Time");
    printf("ENTER "STRINGIFY(MENU_AUTH)"  FOR Setting/Unsetting auth key (current: %s)\n", pubnub_auth_get(pn));
    printf("ENTER "STRINGIFY(MENU_UUID)"  FOR Setting UUID (current: %s)\n", pubnub_uuid_get(pn));
    puts("ENTER "STRINGIFY(MENU_STATE_GET)"  FOR Getting Subscriber State");
    puts("ENTER "STRINGIFY(MENU_SET_STATE)"  FOR Setting Subscriber State");
    puts("ENTER "STRINGIFY(MENU_WHERE_NOW)"  FOR Where Now");
    puts("ENTER "STRINGIFY(MENU_ADD_PRESENCE_CHANNEL)" FOR [Current Subscription] Add Presence Channel");
    puts("ENTER "STRINGIFY(MENU_SUBSCRIBE)" FOR [Current Subscription] Subscribe");
    puts("ENTER "STRINGIFY(MENU_UNSUBSCRIBE)" FOR [Current Subscription] Unsubscribe");
    puts("ENTER "STRINGIFY(MENU_SUBSCRIPTION_ADD_CHANNEL)" FOR [Current Subscription] Add Channel");
    puts("ENTER "STRINGIFY(MENU_SUBSCRIPTION_ADD_GROUP)" FOR [Current Subscription] Add Channel Group");
    puts("ENTER "STRINGIFY(MENU_SUBSCRIPTION_REMOVE_CHANNEL)" FOR [Current Subscription] Remove Channel");
    puts("ENTER "STRINGIFY(MENU_SUBSCRIPTION_REMOVE_GROUP)" FOR [Current Subscription] Remove Channel Group");
    puts("ENTER "STRINGIFY(MENU_GROUP_ADD_CHANNEL)" FOR [Channel Group] Add Channel");
    puts("ENTER "STRINGIFY(MENU_GROUP_REMOVE_CHANNEL)" FOR [Channel Group] Remove Channel");
    puts("ENTER "STRINGIFY(MENU_GROUP_LIST_CHANNELS)" FOR [Channel Group] List Channels");
    puts("ENTER "STRINGIFY(MENU_GROUP_REMOVE_GROUP)" FOR [Channel Group] Remove Group");

    puts("\nENTER "STRINGIFY(MENU_DISPLAY)" to display this menu");
    puts("ENTER "STRINGIFY(MENU_EXIT)" FOR EXIT OR QUIT");
    puts("> ");
}
