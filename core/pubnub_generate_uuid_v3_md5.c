/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "core/pubnub_generate_uuid.h"

#include "lib/md5/pbmd5.h"
#include "core/pubnub_assert.h"

#include <stdlib.h>


int pubnub_generate_uuid_v3_name_md5(
	struct Pubnub_UUID *uuid,
	struct Pubnub_UUID *nsid,
	void *name,
	unsigned namelen
    )
{
    PBMD5_CTX ctx;

    PUBNUB_ASSERT_OPT(uuid != NULL);
    PUBNUB_ASSERT_OPT(nsid != NULL);
    PUBNUB_ASSERT_OPT(name != NULL);

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
