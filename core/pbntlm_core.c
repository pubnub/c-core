/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pbntlm_core.h"

#include "pbntlm_packer.h"

#include "pbbase64.h"

#include "pubnub_log.h"


void pbntlm_core_init(pubnub_t *pb)
{
    pb->ntlm_context.state = pbntlmSendTypeOne;
    pbntlm_packer_init(&pb->ntlm_context);
}


void pbntlm_core_deinit(pubnub_t *pb)
{
    pbntlm_packer_deinit(&pb->ntlm_context);
    pb->ntlm_context.state = pbntlmDone;
}


void pbntlm_core_handle(pubnub_t *pb, char const* base64_msg, size_t length)
{
    int i;
    uint8_t msg[512];
    pubnub_bymebl_t data = { msg, sizeof msg / sizeof msg[0] };

    if (pbntlmDone == pb->ntlm_context.state) {
        pbntlm_core_init(pb);
        return;
    }

    if (pb->ntlm_context.state != pbntlmRcvTypeTwo) {
        PUBNUB_LOG_ERROR("pbntlm_core_handle(): Unexpected state '%d'\n", pb->ntlm_context.state);
        pbntlm_core_deinit(pb);
        return;
    }
    i = pbbase64_decode_std(base64_msg, length, &data);
    if (0 != i) {
        PUBNUB_LOG_ERROR("pbntlm_core_handle(): Failed to Base64 decode '%.*s', result: %d\n", (int)length, base64_msg, i);
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

    case pbntlmDone:
        data->size = 0;
        return 0;
        
    default:
        PUBNUB_LOG_ERROR("pbntlm_core_prep_msg_to_send(): Unexpected state '%d'\n", pb->ntlm_context.state);
        return -1;
    }
}

