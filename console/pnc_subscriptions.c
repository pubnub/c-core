/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pnc_subscriptions.h"
#include "pnc_config.h"

#include "pubnub_assert.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>


static char m_channels[PNC_SUBSCRIBE_CHANNELS_LIMIT][PNC_CHANNEL_NAME_SIZE];
static int m_channels_length = 0;

static char m_groups[PNC_SUBSCRIBE_CHANNELS_LIMIT][PNC_CHANNEL_NAME_SIZE];
static int m_groups_length = 0;


void pnc_subscribe_list_channels(char *value, unsigned max_length)
{
    int i;
    unsigned length;

    PUBNUB_ASSERT_OPT(max_length > 0);

    value[0] = '\0';
    length = 0;
    if (m_channels_length == 0) {
        return;
    }
   
    for (i = 0; i < m_channels_length; ++i) {
        unsigned channel_length = strlen(m_channels[i]);
        if (channel_length > 0) {
            if (length + channel_length + 1 >= max_length) {
                PUBNUB_ASSERT_OPT(0);
                return;
            }
            strcat(value, m_channels[i]);
            length += channel_length;
            if (i < (m_channels_length - 1)) {
                strcat(value, ",");
                ++length;
            }
        }
    }
}


void pnc_subscribe_list_channel_groups(char *value, unsigned max_length)
{
    int i;
    unsigned length;

    PUBNUB_ASSERT_OPT(max_length > 0);

    value[0] = '\0';
    length = 0;
    if (m_channels_length == 0) {
        return;
    }
   
    for (i = 0; i < m_groups_length; ++i) {
        unsigned group_length = strlen(m_groups[i]);
        if (group_length > 0) {
            if (length + group_length + 1 >= max_length) {
                PUBNUB_ASSERT_OPT(0);
                return;
            }
            strcat(value, m_groups[i]);
            length += group_length;
            if (i < (m_groups_length - 1)) {
                strcat(value, ",");
                ++length;
            }
        }
    }
}


void pnc_subscribe_add_channel(char const *channel)
{
    printf("Adding channel \"%s\"... ", channel);

    if (strlen(channel) > PNC_CHANNEL_NAME_SIZE) {
        puts("Channel name too long. Ignoring.");
        return;
    }
    
    if (m_channels_length >= PNC_SUBSCRIBE_CHANNELS_LIMIT) {
        printf("Channels limit exceeded. (%d). Ignoring.\n", PNC_SUBSCRIBE_CHANNELS_LIMIT);
        return;
    }
    
    strcpy(m_channels[m_channels_length++], channel);
    
    puts("Channel successfully added!");
}


void pnc_subscribe_add_channel_group(char const *channel_group)
{
    printf("Adding channel group \"%s\"... ", channel_group);
    if (strlen(channel_group) > PNC_CHANNEL_NAME_SIZE) {
        puts("Channel Group name too long. Ignoring.");
        return;
    }
    
    if (m_groups_length >= PNC_SUBSCRIBE_CHANNELS_LIMIT) {
        printf("Channels limit exceeded. (%d). Ignoring.\n", PNC_SUBSCRIBE_CHANNELS_LIMIT);
        return;
    }
    
    strcpy(m_groups[m_groups_length++], channel_group);
    
    puts("Channel Group successfully added!");
}


void pnc_subscribe_remove_channel(char const *channel)
{
    int i;
    
    for (i = 0; i < m_channels_length; ++i) {
        if (strcmp(channel, m_channels[i]) == 0) {
            puts("Exists");
            memmove(m_channels + i, m_channels + i + 1,
                    ((PNC_SUBSCRIBE_CHANNELS_LIMIT - 1) - i) * PNC_CHANNEL_NAME_SIZE);
            --m_channels_length;
            break;
        }
    }
    
    puts("Channel successfully removed!");
}


void pnc_subscribe_remove_channel_group(char const *channel_group)
{
    int i;
    
    for (i = 0; i < m_groups_length; i++) {
        if (strcmp(channel_group, (char *) m_groups[i]) == 0) {
            puts("Exists");
            memmove(m_groups + i, m_groups + i + 1,
                    ((PNC_SUBSCRIBE_CHANNELS_LIMIT - 1) - i) * PNC_CHANNEL_NAME_SIZE);
            --m_groups_length;
            break;
        }
    }
    
    puts("Channel Group successfully removed!");
}
