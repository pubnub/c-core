#include "pubnub_generate_uuid.h"

#include <FreeRTOS.h>
#include "task.h"

#include "FreeRTOS_IP.h"


int pubnub_generate_uuid_v4_random(struct Pubnub_UUID *uuid)
{
    /* Here we use the same random generator that the FreeRTOS+TCP stack uses.
        Its quality is highly questionable, but, it is the only that we can
        be sure to have. On any particular HW, you may have a better random
        generator - if so, use it instead of this one.
        */
    uint16_t *p = (uint16_t*)uuid;
    while ((char*)p < (char*)(uuid+1)) {
        *p++ = ipconfigRAND32();
    }

    uuid->uuid[6] &= 0x0F;
    uuid->uuid[6] |= 0x40;
    uuid->uuid[8] &= 0x3F;
    uuid->uuid[8] |= 0x80;

    return 0;
}
