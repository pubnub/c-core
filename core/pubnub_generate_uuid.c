/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_config.h"
#include "pubnub_internal.h"
#include "pubnub_generate_uuid.h"

#include "pubnub_assert.h"

#include <string.h>
#include <stdio.h>

#if PUBNUB_HAVE_MD5
#include "md5.h"
#endif

#if PUBNUB_HAVE_SHA1
#include "sha1.h"
#endif


/** Here we provide functions that are not dependent on the platform */

struct Pubnub_UUID_decomposed {
    uint32_t time_low;
    uint16_t time_mid;
    uint16_t time_hi_and_version;
    uint8_t clock_seq_hi_and_reserved;
    uint8_t clock_seq_low;
    uint8_t node[6];
};


int pubnub_generate_uuid_v1_time(
	struct Pubnub_UUID *o_uuid,
	uint16_t *io_clock_seq,
	uint8_t const i_timestamp[8],
	uint8_t const i_node[6]
	)
{
    static uint8_t s_timestamp[8];
    struct Pubnub_UUID_decomposed *ud = (struct Pubnub_UUID_decomposed *)o_uuid;

    if (0 < memcmp(i_timestamp, s_timestamp, sizeof s_timestamp)) {
        (*io_clock_seq)++;
    }

    ud->time_low = *(uint32_t*)i_timestamp;
    ud->time_mid = *(uint16_t*)(i_timestamp + 4);
    ud->time_hi_and_version = *(uint16_t*)(i_timestamp + 6);
    ud->time_hi_and_version |= (1 << 12);
    ud->clock_seq_low = *io_clock_seq & 0xFF;
    ud->clock_seq_hi_and_reserved = (*io_clock_seq & 0x3F00) >> 8;
    ud->clock_seq_hi_and_reserved |= 0x80;
    memcpy(&ud->node, &i_node, sizeof ud->node);

    memcpy(s_timestamp, i_timestamp, sizeof s_timestamp);

    return 0;
}


int pubnub_generate_uuid_v3_name_md5(
	struct Pubnub_UUID *uuid,
	struct Pubnub_UUID *nsid,
	void *name,
	unsigned namelen
    )
{
#if PUBNUB_HAVE_MD5
    MD5_CTX ctx;
    
    MD5Init(&ctx);
    MD5Update(&ctx, nsid, sizeof *nsid);
    MD5Update(&ctx, name, namelen);
    MD5Final(uuid, &ctx);

    uuid->uuid[6] &= 0x0F;
    uuid->uuid[6] |= 0x30;
    uuid->uuid[8] &= 0x3F;
    uuid->uuid[8] |= 0x80;
    
    return 0;
#else
	PUBNUB_UNUSED(uuid);
	PUBNUB_UNUSED(nsid);
	PUBNUB_UNUSED(name);
	PUBNUB_UNUSED(namelen);

	return -1;
#endif
}


int pubnub_generate_uuid_v5_name_sha1(
	struct Pubnub_UUID *uuid,
	struct Pubnub_UUID *nsid,
	void *name,
	unsigned namelen
    )
{
#if PUBNUB_HAVE_SHA1
    SHA1Context ctx;
    
    SHA1Reset(&ctx);
    SHAInput(&ctx, nsid, sizeof *nsid);
    SHA1Input(&ctx, name, namelen);
    if (0 == SHA1Result(&ctx)) {
        return -1;
    }
    memcpy(uuid->uuid, ctx.Message_Digest, sizeof uuid->uuid);
    
    uuid->uuid[6] &= 0x0F;
    uuid->uuid[6] |= 0x50;
    uuid->uuid[8] &= 0x3F;
    uuid->uuid[8] |= 0x80;
    
    return 0;
#else
	PUBNUB_UNUSED(uuid);
	PUBNUB_UNUSED(nsid);
	PUBNUB_UNUSED(name);
	PUBNUB_UNUSED(namelen);
    return -1;
#endif
}


struct Pubnub_UUID_String pubnub_uuid_to_string(struct Pubnub_UUID const *uuid)
{
    struct Pubnub_UUID_String rslt;
    struct Pubnub_UUID_decomposed const*u = (struct Pubnub_UUID_decomposed const*)uuid;
    
    snprintf(rslt.uuid, sizeof rslt.uuid, 
             "%8.8x-%4.4x-%4.4x-%2.2x%2.2x-%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x", 
             u->time_low, u->time_mid, u->time_hi_and_version, 
             u->clock_seq_hi_and_reserved, u->clock_seq_low,
             u->node[0], u->node[1], u->node[2], u->node[3], u->node[4], u->node[5]
        );
    
    return rslt;
}


int pubnub_uuid_compare(struct Pubnub_UUID const *left, struct Pubnub_UUID const *right)
{
    /* maybe this is not really the greatest way to compare, but it works...*/
    return memcmp(left, right, sizeof *left);
}
