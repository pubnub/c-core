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


int pubnub_set_ssl_verify_locations(pubnub_t *p, char const* sCAfile, char const* sCApath)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(p));
#if PUBNUB_USE_SSL
    p->ssl_CAfile = sCAfile;
    p->ssl_CApath = sCApath;
#endif

    return 0;
}


int pubnub_ssl_use_system_certificate_store(pubnub_t *p)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(p));
#if PUBNUB_USE_SSL
    p->options.use_system_certificate_store = true;
#endif

    return 0;
}


void pubnub_ssl_dont_use_system_certificate_store(pubnub_t *p)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(p));
#if PUBNUB_USE_SSL
    p->options.use_system_certificate_store = false;
#endif
}
