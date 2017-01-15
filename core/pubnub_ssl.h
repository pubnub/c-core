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


/** Sets the location(s) of CA certifications for verification
    purposes. This is only available on targets that have a file
    system.

    By default, both of these are NULL. If both are NULL, C-core shall
    use the certificates it knows of - but, these certificates may
    expire or be changed in time. If that happens, you may need to change
    your code (update C-core). Using this function, if you keep your
    certificate store up-to-date, you don't need to change the code.

    Both parameteres are kept as pointers, so, the user is responsible
    for making them valid during the lifetime of context @p p - or
    until they are set to NULL.

    @param p Pubnub context
    
    @param sCAfile The certificate store file. May contain more than
    one CA certificate. Set to NULL if you don't want to use the
    certificate file. 

    @param sCApath The certificate store directory. Each file in this
    directory should contain one CA certificate. Set to NULL if you
    don't want to use the certificate file.

    @return 0: OK, -1: error (not supported, invalid location(s))
 */
int pubnub_set_ssl_verify_locations(pubnub_t *p, char const* sCAfile, char const* sCApath);

/** Instructs C-core to use system certificate store with context @p p.
    This is only available on targets that have a system certificate
    store, like Windows.

    By default, system certificate store will not be used on a context.

    When enabled, using system certificate store takes precedent over
    other certificates.

    If enabled, you can later disable the use of system certificate
    store by calling pubnub_ssl_dont_use_system_certificate_store().

    @param p The context for which to use system certificate store
    @return 0: OK, -1: error (not supported)
 */
int pubnub_ssl_use_system_certificate_store(pubnub_t *p);


/** Instructs C-core to _not_ use system certificate store with
    context @p p, which is also the default.

    The only reason to use this functions is to disable use of system
    certificate store which was previousyl set with
    pubnub_ssl_use_system_certificate_store().

    @param p The context for which to not use system certificate store
 */
void pubnub_ssl_dont_use_system_certificate_store(pubnub_t *p);


#endif /* defined INC_PUBNUB_SSL */
