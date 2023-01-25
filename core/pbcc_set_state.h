/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PBCC_SET_STATE
#define INC_PBCC_SET_STATE

#include "pubnub_api_types.h"
#include "core/pubnub_ccore_pubsub.h"

/** Adjusting the @pb presence state for pubnub usage 
  */
void pbcc_adjust_state(struct pbcc_context* pb,
                        char const* channel,
                        char const* channel_group,
                        char const* state);

#endif // INC_PBCC_SET_STATE
