/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_PROXY
#define	INC_PUBNUB_PROXY


#include "pubnub_api_types.h"

#include <stdint.h>


/** @file pubnub_proxy.h 

    This is the "Proxy" API of the Pubnub client library.  Functions
    here influence the way that Pubnub client library works with
    Internet proxies.

    C-core supports various proxies, and various ways of configuring
    them. Depending on your configuration, you may not be utlizing
    some of those - maybe even none of those.

    This API provides only the two main methods of configuring the
    proxy server: manual and "from system configuration". API for
    other methods of configuring the proxy, like the WPAD(+PAC)
    protocol, if they exist on your platform, is provided separately.

    The Pubnub network doesn't use HTTP authentication, but your proxy
    server might (if it uses HTTP itself as the "proxy protocol").
    So, here we also have API for setting HTTP authentication options,
    rather than it being in some "more general place".
*/


/** Known Internet proxies, by protocol used. C-core can be configured
    to support none, some, or all of them.
 */
enum pubnub_proxy_type {
    /** The simplest proxy, works by using the full URL in the
        `GET` request. Can't work over HTTPS.
    */
    pbproxyHTTP_GET,
    /** A more complex proxy using HTTP(S). A separate message
        (`CONNECT`) is sent to establish a "tunnel" to the "end-point"
        HTTP(S) server, then the "regular" HTTP follows.  In practice,
        this is rarely used for HTTP proper, but only HTTPS.
     */
    pbproxyHTTP_CONNECT,
    /**  Use SOCKSv5 protocol - supports IPv6. Similar, but not
         compatible w/SOCKSv4(a).
     */
    pbproxySOCKSv5,
    /**  Use SOCKSv4a protocol - an upgrade of SOCKSv4.
     */
    pbproxySOCKSv4a,
    /** Use SOCKSv4 protocol - the "proxy speficic" binary protocol,
        widely used.
     */
    pbproxySOCKSv4,
    /** CGI proxy - send the target URL to a HTTP server via "Web
        forms". This is a placholder, there is no plan/roadmap when
        will the support for CGI proxy be available.
    */
    pbproxyCGI,
    /** "Suffix" proxy - add the given suffix to the URL you wish to
        connect to. This is a placholder, there is no plan/roadmap
        when will the support for Suffix proxy be available.
    */
    pbproxySuffix,
    /** Use Tor (The Onion Router) for proxy purposes. This is
        a placholder, for the moment, no direct support is planned
        for Tor.
     */
    pbproxyTor,
    /** Use I2P anonymous proxy/network (the "garlic" routing
        protocol) for proxy purposes. This is a placholder, for the
        moment, no direct support is planned for I2P.
     */
    pbproxyI2P,
    /** No proxy what-so-ever. This is the default.
     */
    pbproxyNONE
};


/** Known HTTP authentication schemes to be used with the HTTP proxy.
 */
enum pubnub_http_authentication_scheme {
    /** The basic authentication scheme. It is not secure and should
        only be used w/HTTPS. This is a placeholder, basic
        authentication scheme is not yet available.
    */
    pbhtauBasic,
    /** The digest authentication scheme. It's complex and slows the
        HTTP protocol down, but it is reasonably secure even with
        "plain" HTTP. There are several hashes to choose from with
        this scheme. This is a placeholder, digest authentication
        scheme is not yet available.
    */
    pbhtauDigest,
    /** No authentication scheme. This is the default */
    phtauNone
};


/** The known hashes for HTTP digest authentication scheme */
enum pubnub_http_digest_hash_algorithm {
    /** Use MD5 algorithm for HTTP digest */
    pbhtdhashMD5,
    /** Use SHA-512-256 algorithm for HTTP digest */
    pbhtdhashSHA_512_256,
    /** Use SHA-256 algorithm for HTTP digest */
    pbhtdhashSHA_256,
};


/** Returns the current proxy type/protocol for the context @p p. */
enum pubnub_proxy_type pubnub_proxy_protocol_get(pubnub_t *p);

/** Sets the configuration for the Internet proxy, by explicitly
    specifying the protocol to use and the proxy server.
    
    If proxy support is available, pubnub_init() will default to "no
    proxy".

    @pre Call this after pubnub_init() on the context
    @pre (protocol != pbproxyNONE) => (ip_address_or_url != NULL)
    @param p The Context to set proxy configuration for
    @param protocol Proxy protocol to use on @p p context
    @param ip_address_or_url The string with IP address or URL of
    the proxy server.
    @param port The port number to use on the proxy - there is no standard,
    the HTTP port (80) is seldom used, while 3128 seems to be a popular one
    
    @return 0: OK, otherwise: error, specified protocol not supported
    or @p ip_address_or_url too long
*/
int pubnub_set_proxy_manual(pubnub_t *p, enum pubnub_proxy_type protocol, char const *ip_address_or_url, uint16_t port);

/** Sets the configuration for the Internet proxy, by reading from the
    "system" configuration.
    
    On some platforms (like Windows), there is some (de-facto)
    standard way of setting a proxy. On others, there may not be.
    C-core will try to do the best it can on a given platform.

    @pre Call this after pubnub_init() on the context
    @pre (protocol != pbproxyNONE) => (ip_address_or_url != NULL)
    @param p The Context to set proxy configuration for
    @param protocol Proxy protocol to use on @p p context
    
    @return 0: OK, otherwise: error, reading system configuration failed
*/
int pubnub_set_proxy_from_system(pubnub_t *p, enum pubnub_proxy_type protocol);

/** Sets the authentication scheme to be used for the Internet proxy
    to be the "basic" authentication scheme.

    @pre Call this after pubnub_init() on the context
    @pre (realm != NULL) && (username != NULL) && (password != NULL)
    @param p The Context to set proxy authentication for
    @param realm Authentication realm
    @param username Authentication username
    @param password Authentication password

    @return 0: OK, otherwise: error, scheme not supported
 */
int pubnub_set_proxy_authentication_basic(pubnub_t *p, char const *realm, char const *username, char const *password);

/** Sets the authentication scheme to be used for the Internet proxy
    to be the "digest" authentication scheme.

    @pre Call this after pubnub_init() on the context
    @pre (realm != NULL) && (username != NULL) && (password != NULL)
    @param p The Context to set proxy authentication for
    @param hash The hash (algorithm) to use
    @param realm Authentication realm
    @param username Authentication username
    @param password Authentication password

    @return 0: OK, otherwise: error, scheme not supported
 */
int pubnub_set_proxy_authentication_digest(pubnub_t *p, enum pubnub_http_digest_hash_algorithm hash, char const *realm, char const *username, char const *password);

/** Set the context @p p to not use _any_ authentication scheme.  This
    is the default, so you only need to call this function if you're
    "resetting" the use of an authentication scheme on the context @p
    p, for whatever reason.

    @pre Call this after pubnub_init() on the context
    @param p The Context to set proxy authentication for
    @return 0: OK, otherwise: error, scheme not supported
 */
int pubnub_set_proxy_authentication_none(pubnub_t *p);


/** Returns the currently set HTTP proxy authentication scheme
    for context @p p.
*/
enum pubnub_http_authentication_scheme pubnub_proxy_authentication_scheme_get(pubnub_t *p);


#endif /* defined INC_PUBNUB_PROXY */
