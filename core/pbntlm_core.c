/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pbntlm_core.h"

#include "pbntlm_packer.h"

#include "pbbase64.h"

#include "pubnub_log.h"


void pbntlm_core_init(pubnub_t *p)
{
    p->ntlm_context.state = pbntlmIdle;
    pbntlm_packer_init(&p->ntlm_context);
}


void pbntlm_core_deinit(pubnub_t *pb)
{
    pbntlm_packer_deinit(&pb->ntlm_context);
    pb->ntlm_context.state = pbntlmDone;
}


void pbntlm_core_handle(pubnub_t *pb, char const* base64_msg)
{
    uint8_t msg[512];
    pubnub_bymebl_t data = { msg, sizeof msg / sizeof msg[0] };

    if (pb->ntlm_context.state != pbntlmRcvTypeTwo) {
        PUBNUB_LOG_ERROR("pbntlm_core_handle(): Unexpected state '%d'\n", pb->ntlm_context.state);
        pbntlm_core_deinit(pb);
        return;
    }
    if (0 != pbbase64_decode_std_str(base64_msg, &data)) {
        PUBNUB_LOG_ERROR("pbntlm_core_handle(): Failed to Base64 decode '%s'\n", base64_msg);
        pbntlm_core_deinit(pb);
        return;
    }
    (void)pbntlm_unpack_type2(&pb->ntlm_context, data);
    pb->ntlm_context.state = pbntlmSendTypeThree;
}


int pbntlm_core_prep_msg_to_send(pubnub_t *pb, pubnub_bymebl_t* data)
{
    int rslt;

    switch (pb->ntlm_context.state) {

    case pbntlmSendTypeOne:
        rslt = pbntlm_pack_type_one(&pb->ntlm_context, pb->proxy_auth_username, pb->proxy_auth_password, data);
        if (0 == rslt) {
            pb->ntlm_context.state = pbntlmRcvTypeTwo;
        }
        else {
            pbntlm_core_deinit(pb);
        }
        return rslt;

    case pbntlmSendTypeThree:
        rslt = pbntlm_pack_type3(&pb->ntlm_context, pb->proxy_auth_username, pb->proxy_auth_password, data);
        pb->proxy_authorization_sent = true;
        pbntlm_core_deinit(pb);
        return rslt;

    default:
        PUBNUB_LOG_ERROR("pbntlm_core_prep_msg_to_send(): Unexpeted state '%d'\n", pb->ntlm_context.state);
        return -1;
    }
}

