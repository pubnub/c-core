/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

#if PUBNUB_PROXY_API

#include "core/pbntlm_core.h"

#include "core/pbntlm_packer.h"

#include "lib/base64/pbbase64.h"

#include <stdlib.h>


void pbntlm_core_init(pubnub_t* pb)
{
    pb->ntlm_context.state = pbntlmSendNegotiate;
    pbntlm_packer_init(pb, &pb->ntlm_context);
}


void pbntlm_core_deinit(pubnub_t* pb)
{
    pbntlm_packer_deinit(pb, &pb->ntlm_context);
    pb->ntlm_context.state = pbntlmDone;
}


void pbntlm_core_handle(pubnub_t* pb, char const* base64_msg, size_t length)
{
    pubnub_bymebl_t data;

    if (pbntlmDone == pb->ntlm_context.state) {
        pbntlm_core_init(pb);
        return;
    }

    if (pb->ntlm_context.state != pbntlmRcvChallenge) {
        PUBNUB_LOG_ERROR(
            pb,
            "NTLM context is in unexpected state: %d",
            pb->ntlm_context.state);
        pbntlm_core_deinit(pb);
        return;
    }
    data = pbbase64_decode_alloc_std(base64_msg, length);
    if (NULL == data.ptr) {
        PUBNUB_LOG_ERROR(pb, "Base64 decode (alloc) of NTLM challenge failed");
        pbntlm_core_deinit(pb);
        return;
    }
    if (0 != pbntlm_unpack_type2(pb, &pb->ntlm_context, data)) {
        PUBNUB_LOG_ERROR(pb, "Failed to unpack NTLM Type-2 challenge");
        free(data.ptr);
        pbntlm_core_deinit(pb);
        return;
    }
    pb->ntlm_context.state = pbntlmSendAuthenticate;
    free(data.ptr);
}


int pbntlm_core_prep_msg_to_send(pubnub_t* pb, pubnub_bymebl_t* data)
{
    int rslt;

    switch (pb->ntlm_context.state) {

    case pbntlmSendNegotiate:
        rslt = pbntlm_pack_type_one(
            pb,
            &pb->ntlm_context,
            pb->proxy_auth_username,
            pb->proxy_auth_password,
            data);
        if (0 == rslt) { pb->ntlm_context.state = pbntlmRcvChallenge; }
        else {
            pbntlm_core_deinit(pb);
        }
        return rslt;

    case pbntlmSendAuthenticate:
        rslt = pbntlm_pack_type3(
            pb,
            &pb->ntlm_context,
            pb->proxy_auth_username,
            pb->proxy_auth_password,
            data);
        pb->proxy_authorization_sent = true;
        pbntlm_core_deinit(pb);
        return rslt;

    case pbntlmDone:
        data->size = 0;
        return 0;

    default:
        PUBNUB_LOG_ERROR(
            pb,
            "NTLM context is in unexpected state: %d",
            pb->ntlm_context.state);
        return -1;
    }
}

#endif /* PUBNUB_PROXY_API */
