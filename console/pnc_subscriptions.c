#include "pnc_subscriptions.h"
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

char channels[PNC_SUBSCRIBE_CHANNELS_LIMIT][PNC_CHANNEL_NAME_SIZE];
char channel_groups[PNC_SUBSCRIBE_CHANNELS_LIMIT][PNC_CHANNEL_NAME_SIZE];
int channels_length = 0;
int channel_groups_length = 0;

void pnc_subscribe_list_channels(char *value) {
  if (channels_length == 0) {
    strcpy(value, "");
    return;
  }

  char channels_string[PNC_SUBSCRIBE_CHANNELS_LIMIT * PNC_CHANNEL_NAME_SIZE];
  int i = 0;

  strcpy(channels_string, "");

  for (i = 0; i < channels_length; i++) {
    if (strlen(channels[i]) > 0) {
      strcat(channels_string, channels[i]);
      if (i < (channels_length - 1)) {
        strcat(channels_string, ",");
      }
    }
  }

  strcpy(value, channels_string);
}

void pnc_subscribe_list_channel_groups(char *value) {
  if (channel_groups_length == 0) {
    strcpy(value, "");
    return;
  }

  char channel_groups_string[PNC_SUBSCRIBE_CHANNELS_LIMIT * PNC_CHANNEL_NAME_SIZE];
  int i = 0;

  strcpy(channel_groups_string, "");

  for (i = 0; i < channel_groups_length; i++) {
    if (strlen(channel_groups[i]) > 0) {
      strcat(channel_groups_string, channel_groups[i]);
      if (i < (channel_groups_length - 1)) {
        strcat(channel_groups_string, ",");
      }
    }
  }

  strcpy(value, channel_groups_string);
}

void pnc_subscribe_add_channel(char *channel) {
  printf("Adding channel \"%s\"... ", channel);
  if (strlen(channel) > PNC_CHANNEL_NAME_SIZE) {
    puts("Channel name too long. Ignoring.");
    return;
  }

  if (channels_length >= PNC_SUBSCRIBE_CHANNELS_LIMIT) {
    printf("Channels limit exceeded. (%d). Ignoring.\n", PNC_SUBSCRIBE_CHANNELS_LIMIT);
    return;
  }

  strcpy(channels[channels_length], channel);
  channels_length++;

  char channels_string[PNC_SUBSCRIBE_CHANNELS_LIMIT * PNC_CHANNEL_NAME_SIZE];
  pnc_subscribe_list_channels(channels_string);

  puts("Success!");
  printf("Channels list now contains %d items: %s\n", channels_length, channels_string);
}

void pnc_subscribe_add_channel_group(char *channel_group) {
  printf("Adding channel group \"%s\"... ", channel_group);
  if (strlen(channel_group) > PNC_CHANNEL_NAME_SIZE) {
    puts("Channel Group name too long. Ignoring.");
    return;
  }

  if (channel_groups_length >= PNC_SUBSCRIBE_CHANNELS_LIMIT) {
    printf("Channels limit exceeded. (%d). Ignoring.\n", PNC_SUBSCRIBE_CHANNELS_LIMIT);
    return;
  }

  strcpy(channel_groups[channel_groups_length], channel_group);
  channel_groups_length++;

  char channel_groups_string[PNC_SUBSCRIBE_CHANNELS_LIMIT * PNC_CHANNEL_NAME_SIZE];
  pnc_subscribe_list_channel_groups(channel_groups_string);

  puts("Success!");
  printf("Channels list now contains %d items: %s\n", channel_groups_length, channel_groups_string);
}

