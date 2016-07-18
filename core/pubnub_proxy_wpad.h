/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_PROXY_WPAD
#define	INC_PUBNUB_PROXY_WPAD


#include "pubnub_api_types.h"


/** @file pubnub_proxy_wpad.h 

    This is the API for the use of Web Proxy Autodiscovery Protocol
    (WPAD). This is a placeholder, WPAD support is not available at
    this time.

    WPAD has a narrow focus - to find a "PAC" (Proxy Auto Config) file
    URL, which it does using the DHCP and, if it fails, the DNS
    protocol. C-core will use only DNS at this time, but support for
    DHCP may be added in the future.

    Since the URL of the PAC file is pretty useless to us on it's own,
    this module will actually also retrieve the PAC file contents and,
    if that succeeds, parse the contents, and set the proxy parameter
    of the Pubnub C-core context accordingly.

    Actually, from a user POV, executing a "configuration via WPAD"
    can be perceived as "just another C-core transaction".

    Keep in mind that the PAC parser is minimal, not a "full-blown
    Javascript" interpreter. It is designed to parse most of the PAC
    file examples that are commonly available. If you encounter some
    complex PAC that C-core can't parse, you may get the contents of
    the PAC file and parse it yourself (you can use some third-party
    Javascript interpreter for such purposes), then set the
    configuration of the C-core Pubnub context "manually". Also,
    C-core PAC parser may not reject some contents that is actually
    invalid Javascript source code.

    You can also skip the "WPAD" part, and provide the URL of the PAC
    file yourself, again, observing the same caveats for the contents
    of the PAC file as mentioned above.

*/


/** This starts a transaction to set the proxy configuration on the
    context @p p using the WPAD protocol. Although this is not really
    a Pubnub transaction, for all intents and purposes, you should
    treat it as one.
*/
enum pubnub_res pubnub_proxy_set_configuration_via_wpad(pubnub_t *p);


/** This starts a transaction to set the proxy configuration on the
    context @p p using the PAC file found at @p url. Although this is
    not really a Pubnub transaction, for all intents and purposes, you
    should treat it as one.
*/
enum pubnub_res pubnub_proxy_set_configuration_via_pac(pubnub_t *p, char const *url);


#endif /* defined INC_PUBNUB_PROXY_WPAD */
