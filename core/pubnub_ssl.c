/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "pubnub_ssl.h"

#include "pubnub_assert.h"
#include "pubnub_internal.h"


void pubnub_set_ssl_options(pubnub_t *p, bool useSSL, bool ignoreSecureConnectionRequirement)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(p));
#if PUBNUB_USE_SSL
    p->options.useSSL = useSSL;
    p->options.fallbackSSL = ignoreSecureConnectionRequirement;
#endif
}


int pubnub_set_ssl_verify_locations(pubnub_t *p, char const* sCAfile, char const* sCApath)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(p));
#if PUBNUB_USE_SSL
    pubnub_mutex_lock(p->monitor);
    p->ssl_CAfile = sCAfile;
    p->ssl_CApath = sCApath;
    pubnub_mutex_unlock(p->monitor);
#endif

    return 0;
}


int pubnub_ssl_use_system_certificate_store(pubnub_t *p)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(p));
#if PUBNUB_USE_SSL
    pubnub_mutex_lock(p->monitor);
    p->options.use_system_certificate_store = true;
    pubnub_mutex_unlock(p->monitor);
#endif

    return 0;
}


void pubnub_ssl_dont_use_system_certificate_store(pubnub_t *p)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(p));
#if PUBNUB_USE_SSL
    pubnub_mutex_lock(p->monitor);
    p->options.use_system_certificate_store = false;
    pubnub_mutex_unlock(p->monitor);
#endif
}


void pubnub_ssl_set_pem_cert(pubnub_t *p, char const *contents)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(p));

#if PUBNUB_USE_SSL
    pubnub_mutex_lock(p->monitor);
    p->ssl_userPEMcert = contents;
    pubnub_mutex_unlock(p->monitor);
#endif
}


void pubnub_set_reuse_ssl_session(pubnub_t *p, bool reuse)
{
    PUBNUB_ASSERT(pb_valid_ctx_ptr(p));

#if PUBNUB_USE_SSL
    pubnub_mutex_lock(p->monitor);
    p->options.reuse_SSL_session = reuse;
    pubnub_mutex_unlock(p->monitor);
#endif
}
