/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PUBNUB_PROXY_CORE
#define      INC_PUBNUB_PROXY_CORE

#include "pubnub_config.h"
#include "pubnub_api_types.h"

#include <stdlib.h>


/** @file pubnub_proxy_core.h

    This is the module that deals with Proxy support, mostly
    authentication. Some part of proxy support is embedded in
    the Pubnub "net-core" FSM,
 */


#if PUBNUB_PROXY_API
/** Processes a proxy related HTTP @p header on the Pubnub context @p
    p.
*/
void pbproxy_handle_http_header(pubnub_t *p, char const* header);
#else
#define pbproxy_handle_http_header(p, header)
#endif


/** Will put string to send as HTTP header in @p header, which is
    caller-allocated buffer of size @p n, on Pubnub context @p p, if
    there is a need to send some proxy-related HTTP header.

    @return 0: @p header is populated, send it; -1: do not send a header
 */
int pbproxy_http_header_to_send(pubnub_t *p, char* header, size_t n);


/** Possible "instructions" on how to proceed after receiving a HTTP
    response.
*/
enum pbproxyFinInstruction {
    /** Do not proceed, there is an error */
    pbproxyFinError,
    /** Retry the Pubnub request, (re-)using the same TCP connection,
        if possible.
     */
    pbproxyFinRetry,
    /** There is nothing proxy-related in this HTTP response, do
	as if we had no proxy support.
    */
    pbproxyFinGoOn
};

/** Returns and instruction on how to proceed after receiving the HTTP
    response, depending on proxy-related needs.
 */
enum pbproxyFinInstruction pbproxy_handle_finish(pubnub_t *pb);


#endif /* !defined INC_PUBNUB_PROXY_CORE */
