/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#if !defined INC_PBPAL_DNS_QUERY_EX
#define INC_PBPAL_DNS_QUERY_EX

#if defined(_WIN32) && defined(PUBNUB_CALLBACK_API)

#include <pubnub_internal.h>


/** Check whether the OS DNS resolver (DnsQueryEx) should be used
    instead of the custom UDP DNS resolver.

    Returns true when no user-provided primary or secondary DNS
    servers are configured (IPv4 or IPv6).  In that case the OS
    resolver is preferred because it respects VPN DNS routing,
    split-tunnel configurations, and the system DNS cache.
 */
bool pbpal_os_dns_should_use(pubnub_t* pb);


/** Start asynchronous DNS resolution via DnsQueryEx for both A and
    AAAA record types.

    Populates @p pb->os_dns.  Completion callbacks will fire on a
    Windows thread-pool thread and enqueue @p pb for FSM processing.

    @param pb       PubNub context (must be under pb->monitor).
    @param hostname Origin hostname to resolve (narrow string).
    @return 0 on success (queries submitted), -1 on error.
 */
int pbpal_os_dns_start(pubnub_t* pb, const char* hostname);


/** Check whether the pending OS DNS resolution has completed and, if
    so, populate @p pb->dns_queries and @p pb->spare_addresses from
    the results.

    Called from pbpal_check_resolv_and_connect() on the socket-watcher
    thread while pb->monitor is held.

    @return 0  all results received and addresses populated.
    @return +1 still waiting (would-block).
    @return -1 resolution failed (no addresses obtained).
 */
int pbpal_os_dns_check(pubnub_t* pb);


/** Cancel any pending DnsQueryEx queries and free result records.

    Safe to call even when no OS DNS resolution is active.
 */
void pbpal_os_dns_cancel(pubnub_t* pb);


#endif /* defined(_WIN32) && defined(PUBNUB_CALLBACK_API) */
#endif /* !defined INC_PBPAL_DNS_QUERY_EX */
