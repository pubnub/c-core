/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_internal.h"

#include "pubnub_sync_subscribe_loop.h"

#include "pubnub_pubsubapi.h"
#include "pubnub_ntf_sync.h"
#include "pubnub_assert.h"

#if !defined PUBNUB_NTF_RUNTIME_SELECTION
struct pubnub_subloop_descriptor pubnub_subloop_define(pubnub_t *p, char const *channel)
#else 
struct pubnub_subloop_descriptor pubnub_sync_subloop_define(pubnub_t *p, char const *channel)
#endif
{
    struct pubnub_subloop_descriptor rslt = { p, channel };
    rslt.options = pubnub_subscribe_defopts();

    return rslt;
}


enum pubnub_res pubnub_subloop_fetch(struct pubnub_subloop_descriptor const* pbsld, char const** message)
{
    enum pubnub_res pbres = PNR_OK;

    PUBNUB_ASSERT_OPT(NULL != pbsld);
    PUBNUB_ASSERT_OPT(NULL != message);

    *message = pubnub_get(pbsld->pbp);
    if (NULL == *message) {
        pbres = pubnub_subscribe_ex(pbsld->pbp, pbsld->channel, pbsld->options);
        if (PNR_STARTED == pbres) {
            pbres = pubnub_await(pbsld->pbp);
            if (PNR_OK == pbres) {
                *message = pubnub_get(pbsld->pbp);
            }
        }
    }

    return pbres;
}
