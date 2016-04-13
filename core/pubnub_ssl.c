/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_ssl.h"

#include "pubnub_assert.h"
#include "pubnub_internal.h"


void pubnub_set_ssl_options(pubnub_t *p, bool useSSL, bool reduceSecurityOnError, bool ignoreSecureConnectionRequirement)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(p));
#if PUBNUB_USE_SSL
    p->options.useSSL = useSSL;
    p->options.ignoreSSL = reduceSecurityOnError;
    p->options.fallbackSSL = ignoreSecureConnectionRequirement;
#endif
}
