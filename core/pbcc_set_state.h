/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PBCC_SET_STATE
#define INC_PBCC_SET_STATE

#include "pubnub_api_types.h"

/** Adjusting the @pb state after the call to the pubnub services
    that afects the presence state 
  */
void pbcc_adjust_state(pubnub_t* pb,
                        char const* channel,
                        char const* channel_group,
                        char const* state);

#endif // INC_PBCC_SET_STATE
