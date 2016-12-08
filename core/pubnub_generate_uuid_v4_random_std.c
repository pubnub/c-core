#include "pubnub_generate_uuid.h"
 
#include <stdlib.h>
 
 
int pubnub_generate_uuid_v4_random(struct Pubnub_UUID *uuid)
{
    /* This uses the standard rand() function, which is, in most 
       implementations, not a very good random number generator (RNG). So,
       if you have a better RNG, please don't use this function. Also, it is
       essential that you call srand() with a good seed at least once in your
       program, otherwise your "random" numbers won't be so random after all.
       */
    int *p;
    for (p = (int*)uuid; p < (int*)(uuid + 1); ++p) {
        *p = rand();
    }
  
    uuid->uuid[6] &= 0x0F;
    uuid->uuid[6] |= 0x40;
    uuid->uuid[8] &= 0x3F;
    uuid->uuid[8] |= 0x80;
 
    return 0;
}
