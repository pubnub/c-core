/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#if PUBNUB_PROXY_API

#include "pbntlm_packer.h"

#include "pubnub_log.h"
#include "pubnub_assert.h"

#include <string.h>


void pbntlm_packer_init(pbntlm_ctx_t *pb)
{
    pb->in_token_size = 0;
}


int pbntlm_pack_type_one(pbntlm_ctx_t *pb, char const* username, char const* password, pubnub_bymebl_t* msg)
{
    PUBNUB_LOG_ERROR("Generic (non-SSPI) NTLM packer not yet implemented\n");
    return -1;
}


int pbntlm_unpack_type2(pbntlm_ctx_t *pb, pubnub_bymebl_t msg)
{
    PUBNUB_ASSERT_OPT(pb != NULL);
    PUBNUB_ASSERT_OPT(msg.ptr != NULL);
    PUBNUB_ASSERT_OPT(msg.size > 0);

    if (msg.size > sizeof pb->in_token) {
        PUBNUB_LOG_ERROR("NTLM Type2 message too long: %zi bytes, max allowed %zi\n", msg.size, sizeof pb->in_token);
        return -1;
    }

    /* Should probably do some basic checking of the message */

    memcpy(pb->in_token, msg.ptr, msg.size);
    pb->in_token_size = msg.size;

    return 0;
}


int pbntlm_pack_type3(pbntlm_ctx_t *pb, char const* username, char const* password, pubnub_bymebl_t* msg)
{
    PUBNUB_LOG_ERROR("Generic (non-SSPI) NTLM packer not yet implemented\n");
    return -1;
}


void pbntlm_packer_deinit(pbntlm_ctx_t *pb)
{
    PUBNUB_UNUSED(pb);
}

#endif /* PUBNUB_PROXY_API */
