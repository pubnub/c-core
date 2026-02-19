/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#if PUBNUB_PROXY_API

#include "pbntlm_packer.h"

#include "pubnub_assert.h"

#include <string.h>


void pbntlm_packer_init(pubnub_t* context, pbntlm_ctx_t* pb)
{
    PUBNUB_UNUSED(context);
    pb->in_token_size = 0;
}


int pbntlm_pack_type_one(
    pubnub_t*        context,
    pbntlm_ctx_t*    pb,
    char const*      username,
    char const*      password,
    pubnub_bymebl_t* msg)
{
    PUBNUB_LOG_ERROR(
        context, "Generic (non-SSPI) NTLM packer not yet implemented.");
    return -1;
}


int pbntlm_unpack_type2(
    pubnub_t*       context,
    pbntlm_ctx_t*   pb,
    pubnub_bymebl_t msg)
{
    PUBNUB_ASSERT_OPT(pb != NULL);
    PUBNUB_ASSERT_OPT(msg.ptr != NULL);
    PUBNUB_ASSERT_OPT(msg.size > 0);

    if (msg.size > sizeof pb->in_token) {
        PUBNUB_LOG_ERROR(
            context,
            "NTLM Type2 message too long: %zu bytes, %zu bytes allowed.",
            msg.size,
            sizeof pb->in_token);
        return -1;
    }

    /* Should probably do some basic checking of the message */

    memcpy(pb->in_token, msg.ptr, msg.size);
    pb->in_token_size = msg.size;

    return 0;
}


int pbntlm_pack_type3(
    pubnub_t*        context,
    pbntlm_ctx_t*    pb,
    char const*      username,
    char const*      password,
    pubnub_bymebl_t* msg)
{
    PUBNUB_LOG_ERROR(
        context, "Generic (non-SSPI) NTLM packer not yet implemented.");
    return -1;
}


void pbntlm_packer_deinit(pubnub_t* context, pbntlm_ctx_t* pb)
{
    PUBNUB_UNUSED(context);
    PUBNUB_UNUSED(pb);
}

#endif /* PUBNUB_PROXY_API */
