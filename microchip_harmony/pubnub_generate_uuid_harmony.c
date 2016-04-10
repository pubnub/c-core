#include "pubnub_generate_uuid.h"

#include "crypto.h"

#include <stdbool.h>


static CRYPT_RNG_CTX m_rng;
static bool m_init_done;


int pubnub_generate_uuid_v4_random(struct Pubnub_UUID *uuid)
{
    uint8_t *p = (uint8_t*)uuid;
    
    /* We should do this under a lock to be "abosolutely sure" */
    if (!m_init_done) {
        if (CRYPT_RNG_Initialize(&m_rng) < 0) {
            return -1;
        }
        m_init_done = true;
    }
    
    while ((uint8_t*)p < (uint8_t*)(uuid+1)) {
        if (CRYPT_RNG_Get(&m_rng, p++) < 0) {
            return -1;
        }
    }

    uuid->uuid[6] &= 0x0F;
    uuid->uuid[6] |= 0x40;
    uuid->uuid[8] &= 0x3F;
    uuid->uuid[8] |= 0x80;

    return 0;
}
