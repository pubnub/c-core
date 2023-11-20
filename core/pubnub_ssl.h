/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_SSL
#define	INC_PUBNUB_SSL


#include "pubnub_api_types.h"
#include "lib/pb_extern.h"

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

    @param ignoreSecureConnectionRequirement When SSL is enabled,
    should the client fallback to a non-SSL connection if it
    experiences issues handshaking across local proxies, firewalls,
    etc? (default: YES)
*/
PUBNUB_EXTERN void pubnub_set_ssl_options(pubnub_t *p, bool useSSL, bool ignoreSecureConnectionRequirement);

/** Sets the option to reuse the SSL session on a new connection to
    @p reuse on the context @p p.

    @note While reusing SSL sessions can provide for great speed-up of
    TLS/SSL session establishment, it is also prone to errors.

    @param p The context for which to set the option for SSL session reuse
    @param reuse The value (true/false == on/off) of the option
 */
PUBNUB_EXTERN void pubnub_set_reuse_ssl_session(pubnub_t *p, bool reuse);

/** Sets the location(s) of CA certificates for verification
    purposes. This is only available on targets that have a file
    system.

    By default, both of these are NULL. If both are NULL, C-core shall
    use the certificates it knows of - but, these certificates may
    expire or be changed in time. If that happens, you may need to change
    your code (update C-core). Using this function, if you keep your
    certificate store up-to-date, you don't need to change the code.

    Both parameters are kept as pointers, so, the user is responsible
    for making them valid during the lifetime of context @p p - or
    until they are set to NULL.

    @pre @p p is a valid context

    @param p Pubnub context
    
    @param sCAfile The certificate store file. May contain more than
    one CA certificate. Set to NULL if you don't want to use the
    certificate file. 

    @param sCApath The certificate store directory. Each file in this
    directory should contain one CA certificate. Set to NULL if you
    don't want to use the certificate file.

    @return 0: OK, -1: error (not supported, invalid location(s))
 */
PUBNUB_EXTERN int pubnub_set_ssl_verify_locations(pubnub_t *p, char const* sCAfile, char const* sCApath);

/** Instructs C-core to use system certificate store with context @p p.
    This is only available on targets that have a system certificate
    store, like Windows.

    By default, system certificate store will not be used on a context.

    When enabled, using system certificate store takes precedent over
    other certificates.

    If enabled, you can later disable the use of system certificate
    store by calling pubnub_ssl_dont_use_system_certificate_store().

    @pre @p p is a valid context

    @param p The context for which to use system certificate store
    @return 0: OK, -1: error (not supported)
 */
PUBNUB_EXTERN int pubnub_ssl_use_system_certificate_store(pubnub_t *p);


/** Instructs C-core to _not_ use system certificate store with
    context @p p, which is also the default.

    The only reason to use this functions is to disable use of system
    certificate store which was previously set with
    pubnub_ssl_use_system_certificate_store().

    @pre @p p is a valid context

    @param p The context for which to not use system certificate store
 */
PUBNUB_EXTERN void pubnub_ssl_dont_use_system_certificate_store(pubnub_t *p);


/** Sets the contents of a "user-defined", in-memory, PEM certificate
    to use for @p p context. This will be used in addition to other
    certificates. Certificate is usually found in a `.pem` file, from
    which you can read/copy it to a string and pass it to this
    function.

    Unlike other certificate-handling functions, this one is available
    on any platform, including deeply embedded ones.

    It is meant primarily for debugging purposes with proxy debuggers
    (like Fiddler, Charles, etc).

    There is only one "user-defined" PEM certificate per context. 

    @pre @p p is a valid context

    @param p The Pubnub context for which to set the certificate
    @param contents String containing the PEM certificate. Assumed
    to be valid during the lifetime of @p p. Use `NULL` if you don't
    want to use your certificate.
 */
PUBNUB_EXTERN void pubnub_ssl_set_usrdef_pem_cert(pubnub_t *p, char const *contents);

#endif /* defined INC_PUBNUB_SSL */
