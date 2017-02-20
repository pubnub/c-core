/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
//#include "pubnub_config.h"
//#include "pubnub_internal.h"
#include "pubnub_generate_uuid.h"

#include "pbmd5.h"
#include "pubnub_assert.h"

//#include <string.h>
//#include <stdio.h>


int pubnub_generate_uuid_v3_name_md5(
	struct Pubnub_UUID *uuid,
	struct Pubnub_UUID *nsid,
	void *name,
	unsigned namelen
    )
{
    PBMD5_CTX ctx;
    
    pbmd5_init(&ctx);
    pbmd5_update(&ctx, nsid, sizeof *nsid);
    pbmd5_update(&ctx, name, namelen);
    pbmd5_final(&ctx, uuid->uuid);

    uuid->uuid[6] &= 0x0F;
    uuid->uuid[6] |= 0x30;
    uuid->uuid[8] &= 0x3F;
    uuid->uuid[8] |= 0x80;
    
    return 0;
}
