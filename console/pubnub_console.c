/* -*- c-file-style:"stroustrup" -*- */
#include "pubnub_console.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>

static const char pubkey[] = "demo";
static const char subkey[] = "demo";
static const char channel[] = "hello_world";

int main()
{
  pubnub_t *pn = pubnub_alloc();

  if (NULL == pn) {
    printf("Failed to allocate pubnub context");
  }

  pnc_ops_init(pn);
  pubnub_init(pn, pubkey, pubkey);
  pubnub_set_uuid(pn, PNC_DEFAULT_UUID);

  displayMenuOptions(pn);

  int option = 0;
  char opt_string[4];
  char channel[PNC_CHANNEL_NAME_SIZE];
  char channel_group[PNC_CHANNEL_NAME_SIZE];
  char auth_key[PNC_AUTH_KEY_SIZE];
  char uuid[PNC_UUID_SIZE];
  char state[2048];

  while(option != 9) {
    fgets(opt_string, sizeof(opt_string), stdin);
    opt_string[strlen(opt_string) - 1] = '\0';
    option = atoi(opt_string);
    printf("You entered %d(%s)\n", option, opt_string);

    switch (option) {
      case 0:
        displayMenuOptions(pn);
        break;
      case 1:
        // Subscribe
        {
          bool isGroup;

          pnc_read_bool_from_console("Group", &isGroup);

          if (isGroup) {
            pnc_read_string_from_console("Subscribe: Enter Group name",
                channel, PNC_CHANNEL_NAME_SIZE);
            pnc_ops_cg_subscribe(pn, channel) ;
          } else {
            pnc_read_string_from_console("Subscribe: Enter Channel name",
                channel, PNC_CHANNEL_NAME_SIZE);
            pnc_ops_subscribe(pn, channel) ;
          }
        }

        break;
      case 2:
        // Publish
        {
          bool store = false;
          // TODO: add json builded string

          pnc_read_string_from_console("Channel Name",
              channel, PNC_CHANNEL_NAME_SIZE);
          pnc_read_bool_from_console_optional("Store", &store, true);

          puts("Enter the message for publish. To exit loop enter Q");

          char message[PNC_READ_STRING_SIZE];
          int attempt_count = 0;

          while (fgets(message, PNC_READ_STRING_SIZE, stdin) != NULL && strcmp(message, "Q\n")) {
            if (strlen(message) == 1) {
              puts("Invalid input");
              continue;
            }

            message[strlen(message) - 1] = '\0';

            if (message[0] == '{' && message[strlen(message) - 1] == '}') {
              pnc_ops_parse_response("pubnub_publish()",
                  pubnub_publish(pn, channel, message), pn);
            } else {
              char encoded_message[strlen(message) + 2];
              sprintf(encoded_message, "\"%s\"", message);
              pnc_ops_parse_response("pubnub_publish()",
                  pubnub_publish(pn, channel, encoded_message), pn);
            }

            puts("Enter the message for publish. To exit loop enter Q.");
            attempt_count++;
          }
        }

        break;
      case 3:
        // Presence
        pnc_read_string_from_console("Channel name",
            channel, PNC_CHANNEL_NAME_SIZE);

        pnc_ops_presence(pn, channel);

        break;
      case 4:
        // History
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
          pnc_read_bool_from_console("Include timetokens", &include_token);

          pnc_ops_parse_response("pubnub_historyv2()",
              pubnub_historyv2(pn, channel, NULL, count, include_token), pn);
        }

        break;
      case 5:
        // Here now
        pnc_read_string_from_console_optional("Channel Name",
            channel, PNC_CHANNEL_NAME_SIZE, true);

        pnc_ops_parse_response("pubnub_here_now()",
            pubnub_here_now(pn, channel, NULL), pn);
        break;
      case 6:
        pnc_ops_unsubscribe();
        break;
      case 7:
        pnc_ops_presence_unsubscribe();
        break;
      case 8:
        // Time
        pnc_ops_parse_response("pubnub_time()", pubnub_time(pn), pn);
        break;
      case 10:
        pnc_ops_disconnect_and_resubscribe();
        break;
      case 11:
        pnc_ops_disconnect_and_resubscribe();
        break;
      case 12:
        // Set Auth
        pnc_read_string_from_console("Auth key",
            auth_key, PNC_AUTH_KEY_SIZE);
        pubnub_set_auth(pn, auth_key);

        break;
      case 13:
        // Set UUID
        pnc_read_string_from_console("UUID", uuid, PNC_UUID_SIZE);
        pubnub_set_uuid(pn, uuid);

        break;
      case 14:
        // Get State
        pnc_read_string_from_console("Channel Name",
            channel, PNC_CHANNEL_NAME_SIZE);
        pnc_read_string_from_console_optional("UUID",
            uuid, PNC_UUID_SIZE, true);

        if (strlen(uuid) == 0) {
          strcpy(uuid, pubnub_get_uuid(pn));
        }

        pnc_ops_parse_response("pubnub_get_state()",
            pubnub_state_get(pn, channel, NULL, uuid), pn);
        break;
      case 15:
        // Set State
        pnc_read_string_from_console("Channel Name",
            channel, PNC_CHANNEL_NAME_SIZE);
        pnc_read_string_from_console("JSON state without escaping",
            state, PNC_READ_STRING_SIZE);
        pnc_ops_parse_response("pubnub_set_state()",
            pubnub_set_state(pn, channel, NULL, pubnub_get_uuid(pn), state), pn);
        break;
      case 16:
        // Where Now
        pnc_read_string_from_console_optional("UUID",
            uuid, PNC_UUID_SIZE, true);
        pnc_ops_parse_response("pubnub_where_now()",
            pubnub_where_now(pn, uuid), pn);
        break;
      case 17:
        // Add channel
        pnc_read_string_from_console("Group",
            channel_group, PNC_CHANNEL_NAME_SIZE);
        pnc_read_string_from_console("Channel",
            channel, PNC_CHANNEL_NAME_SIZE);

        pnc_ops_parse_response("pubnub_add_channel_to_group()",
            pubnub_add_channel_to_group(pn, channel, channel_group), pn);

        break;
      case 18:
        // Remove channel
        pnc_read_string_from_console("Group",
            channel_group, PNC_CHANNEL_NAME_SIZE);
        pnc_read_string_from_console("Channel",
            channel, PNC_CHANNEL_NAME_SIZE);

        pnc_ops_parse_response("pubnub_remove_channel_from_group()",
            pubnub_remove_channel_from_group(pn, channel, channel_group), pn);

        break;
      case 19:
        // List channels
        pnc_read_string_from_console("Group",
            channel_group, PNC_CHANNEL_NAME_SIZE);

        pnc_ops_parse_response("pubnub_list_channel_group()",
            pubnub_list_channel_group(pn, channel_group), pn);

        break;
      case 20:
        // List groups
        puts("Not implemented yet");
        break;
      default:
        printf("Invalid input: %d\n", option);
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

void displayMenuOptions(pubnub_t *pn) {
  printf("ENTER 1  FOR Subscribe (Currently subscribed to %s)\n", "nothing");
  puts("ENTER 2  FOR Publish");
  puts("ENTER 3  FOR Presence");
  puts("ENTER 4  FOR History");
  puts("ENTER 5  FOR Here Now");
  puts("ENTER 6  FOR Unsubscribe(Only async)");
  puts("ENTER 7  FOR Presence-Unsubscribe(Only async)");
  puts("ENTER 8  FOR Time");
  puts("ENTER 9  FOR EXIT OR QUIT");
  puts("ENTER 10 FOR Disconnect-And-Resubscribe");
  puts("ENTER 11 FOR Disconnect-And-Resubscribe with timetoken");
  printf("ENTER 12 FOR Setting/Unsetting auth key ( current: %s )\n", pnc_get_auth(pn));
  printf("ENTER 13 FOR Setting UUID ( current: %s)\n", pnc_get_uuid(pn));
  puts("ENTER 14 FOR Getting Subscriber State");
  puts("ENTER 15 FOR Setting Subscriber State");
  puts("ENTER 16 FOR Where Now");
  puts("ENTER 17 FOR [Channel Group] Add Channel");
  puts("ENTER 18 FOR [Channel Group] Remove Channel");
  puts("ENTER 19 FOR [Channel Group] List Channels");
  puts("ENTER 20 FOR [Channel Group] List Groups");
  puts("ENTER 21 FOR [Channel Group] Remove Group");
}

char const *pnc_get_auth(pubnub_t *pb) {
  char const *auth = pb->core.auth;

  if (auth == NULL) {
    return "(NOT SET)";
  } else {
    return auth;
  }
}

char const *pnc_get_uuid(pubnub_t *pb) {
  char const *uuid = pb->core.uuid;

  if (uuid == NULL) {
    return "(NOT SET)";
  } else {
    return uuid;
  }
}
