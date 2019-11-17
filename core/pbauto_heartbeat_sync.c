/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_assert.h"
#include "pubnub_api_types.h"

#include <stddef.h>

struct pubnub_;

/** A pubnub context. An opaque data structure that holds all the data
    needed for a context.
 */
typedef struct pubnub_ pubnub_t;

int pubnub_enable_auto_heartbeat(pubnub_t* pb, size_t period_sec)
{
    PUBNUB_UNUSED(pb);
    return 0;
}

int pubnub_set_heartbeat_period(pubnub_t* pb, size_t period_sec)
{
    PUBNUB_UNUSED(pb);
    return 0;
}

void pubnub_disable_auto_heartbeat(pubnub_t* pb)
{
    PUBNUB_UNUSED(pb);
}

bool pubnub_is_auto_heartbeat_enabled(pubnub_t* pb)
{
    PUBNUB_UNUSED(pb);
    return false;
}

void pubnub_heartbeat_free_thumpers(void)
{
    return;
}

void pbauto_heartbeat_read_channelInfo(pubnub_t const* pb,
                                       char const** channel,
                                       char const** channel_group)
{
    PUBNUB_UNUSED(pb);
}

void pbauto_heartbeat_transaction_ongoing(pubnub_t const* pb)
{
    PUBNUB_UNUSED(pb);
}

void pbauto_heartbeat_start_timer(pubnub_t const* pb)
{
    PUBNUB_UNUSED(pb);
}

void pbauto_heartbeat_free_channelInfo(pubnub_t* pb)
{
    PUBNUB_UNUSED(pb);
}

enum pubnub_res pbauto_heartbeat_prepare_channels_and_ch_groups(pubnub_t* pb,
                                                                char const** channel,
                                                                char const** channel_group)
{
    PUBNUB_UNUSED(pb);
    return PNR_OK;
}

void pbauto_heartbeat_stop(void)
{
    return;
}
