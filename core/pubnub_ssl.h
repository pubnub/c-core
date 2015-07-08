/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_SSL
#define	INC_PUBNUB_SSL


#include "pubnub_res.h"

#include <stdbool.h>


/** @file pubnub_coreapi.h 
    This is the "Core" API of the Pubnub client library.
    It has the functions that are present in all variants and
    platforms and have the same interface in all of them.
    For the most part, they have the same implementation in
    all of them, too.
*/


struct pubnub;

/** A pubnub context. An opaque data structure that holds all the
    data needed for a context.
 */
typedef struct pubnub pubnub_t;



/** Sets the SSL options for a context. If SSL support is enabled,
    these options will get their default values in pubnub_init().
    If you wish to change them, call this function.

    @pre Call this after pubnub_init() on the context
    @param p The Context to set SSL options for

    @param useSSL Should the PubNub client establish the connection to
    PubNub using SSL? (default: YES)

    @param reduceSecurityOnError When SSL is enabled, should PubNub
    client ignore all SSL certificate-handshake issues and still
    continue in SSL mode if it experiences issues handshaking across
    local proxies, firewalls, etc? (default: YES)

    @param ignoreSecureConnectionRequirement When SSL is enabled,
    should the client fallback to a non-SSL connection if it
    experiences issues handshaking across local proxies, firewalls,
    etc? (default: YES)
*/
void pubnub_set_ssl_options(pubnub_t *p, bool useSSL, bool reduceSecurityOnError, bool ignoreSecureConnectionRequirement);



#endif /* defined INC_PUBNUB_SSL */
