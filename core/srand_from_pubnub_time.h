#if !defined INC_SRAND_FROM_PUBNUB_TIME
#define      INC_SRAND_FROM_PUBNUB_TIME
 
 
#include "pubnub_api_types.h"
#include "lib/pb_extern.h"
 
 
/** This helper function will call the C standard srand() function with the seed
    taken from the time returned from Pubnub's `time` operation (which can
    be initiated with pubnub_time()).
    
    It's useful if you want a high-fidelity time used for srand() and on 
    embedded system that don't have a Real-Time Clock.
    
    Keep in mind that this requires a round-trip to Pubnub, so it will take
    some time, depending on your network, at least miliseconds. So, it's best
    used only once, at the start of your program.
    
    @param pbp The Pubnub context to use to get time
    @return 0: OK, -1: error (srand() was not called)
*/
PUBNUB_EXTERN int srand_from_pubnub_time(pubnub_t *pbp);
 
 
#endif /* !defined INC_SRAND_FROM_PUBNUB_TIME */
