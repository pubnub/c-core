/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_SSL
#define	INC_PUBNUB_SSL


#include "pubnub_api_types.h"

#include <stdbool.h>


/** @file pubnub_ssl.h 
    This is the "SSL/TLS" API of the Pubnub client library.
    It has functions that pertain to using SSL/TLS to connect
    to Pubnub. It is supported on platforms that have SSL/TLS.
*/


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
