#include "srand_from_pubnub_time.h"
 
#include "pubnub_coreapi.h"
#include "pubnub_ntf_sync.h"
 
#include <stdlib.h>
#include <string.h>
 
 
int srand_from_pubnub_time(pubnub_t *pbp)
{
    pubnub_res rslt = pubnub_time(pbp);
    if (rslt != PNR_STARTED) {
        return -1;
    }
    rslt = pubnub_await(pbp);
    if (rslt != PNR_OK) {
        return -1;
    }
    char const* pbtime = pubnub_get(pbp);
    if (0 == pbtime)  {
        return -1;
    }
    size_t length_of_time = strlen(pbtime);
    if (0 == length_of_time) {
        return -1;
    }
    char const *s = pbtime + length_of_time - 1;
    unsigned int val_for_srand = 0;
    for (int i = 0; (i < 10) && (s > pbtime); ++i, --s) {
        val_for_srand = val_for_srand * 10 + *s - '0';
    }
    
    srand(val_for_srand);
    
    return 0;
}
