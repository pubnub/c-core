/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PBCC_SET_STATE
#define INC_PBCC_SET_STATE

#include "pubnub_api_types.h"

/** Adjusting the @pb presence state for pubnub usage 
  */
void pbcc_adjust_state(pubnub_t* pb,
                        char const* channel,
                        char const* channel_group,
                        char const* state);

#endif // INC_PBCC_SET_STATE
