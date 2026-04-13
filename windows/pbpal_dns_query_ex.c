/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
/**
 * @file pbpal_dns_query_ex.c
 * @brief Windows DnsQueryEx-based async DNS resolution for the callback API.
 *
 * When no user-provided DNS servers are configured, this module uses the
 * Windows OS resolver (DnsQueryEx) instead of the SDK's custom UDP DNS.
 * This ensures DNS queries go through the OS DNS cache and respect VPN
 * routing (NRPT rules, split-tunnel, etc.).
 *
 * Minimum Windows version: Windows 8 (DnsQueryEx) / 8.1 (DnsCancelQuery).
 */

#if defined(_WIN32) && defined(PUBNUB_CALLBACK_API)

#include "pbpal_dns_query_ex.h"

#include <pubnub_internal.h>
#include "core/pubnub_assert.h"
#include "core/pubnub_dns_servers.h"
#if PUBNUB_USE_LOGGER
#include "core/pubnub_logger_internal.h"
#endif // PUBNUB_USE_LOGGER

#include <string.h>
#include <time.h>

#pragma comment(lib, "dnsapi.lib")


/* Maximum DNS hostname length (RFC 1035: 253 chars + NUL). */
#define PBDNS_MAX_HOSTNAME_WCHARS 256


/* ------------------------------------------------------------------ */
/*  Forward declarations for completion callbacks                      */
/* ------------------------------------------------------------------ */

static VOID WINAPI os_dns_completion_a(PVOID   pQueryContext,
                                       PDNS_QUERY_RESULT pQueryResults);
#if PUBNUB_USE_IPV6
static VOID WINAPI os_dns_completion_aaaa(PVOID   pQueryContext,
                                          PDNS_QUERY_RESULT pQueryResults);
#endif


/* ------------------------------------------------------------------ */
/*  pbpal_os_dns_should_use                                            */
/* ------------------------------------------------------------------ */

bool pbpal_os_dns_should_use(pubnub_t* pb)
{
    struct pubnub_ipv4_address tmp4;

    if (pubnub_get_dns_primary_server_ipv4(pb, &tmp4) == 0)
        return false;
    if (pubnub_get_dns_secondary_server_ipv4(pb, &tmp4) == 0)
        return false;

#if PUBNUB_USE_IPV6
    {
        struct pubnub_ipv6_address tmp6;
        if (pubnub_get_dns_primary_server_ipv6(pb, &tmp6) == 0)
            return false;
        if (pubnub_get_dns_secondary_server_ipv6(pb, &tmp6) == 0)
            return false;
    }
#endif

    return true;
}


/* ------------------------------------------------------------------ */
/*  pbpal_os_dns_start                                                 */
/* ------------------------------------------------------------------ */

int pbpal_os_dns_start(pubnub_t* pb, const char* hostname)
{
    DNS_QUERY_REQUEST request;
    DNS_STATUS        status;
    WCHAR             whostname[PBDNS_MAX_HOSTNAME_WCHARS];

    PUBNUB_ASSERT_OPT(pb != NULL);
    PUBNUB_ASSERT_OPT(hostname != NULL);

    if (0 == MultiByteToWideChar(
                 CP_UTF8, 0, hostname, -1,
                 whostname, PBDNS_MAX_HOSTNAME_WCHARS)) {
        PUBNUB_LOG_ERROR(pb,
                         "DnsQueryEx: MultiByteToWideChar failed for '%s'\n",
                         hostname);
        return -1;
    }

    /* Cancel any pending queries from a previous transaction, then
       reset state. DnsCancelQuery is synchronous -- the completion
       callback fires before it returns -- so no stale callback can
       arrive after this point. */
    pbpal_os_dns_cancel(pb);
    memset(&pb->os_dns, 0, sizeof pb->os_dns);
    pb->os_dns.active = true;

    memset(&request, 0, sizeof request);
    request.Version             = DNS_QUERY_REQUEST_VERSION1;
    request.QueryName           = whostname;
    request.QueryType           = DNS_TYPE_A;
    request.QueryOptions        = DNS_QUERY_STANDARD;
    request.pQueryContext        = pb;
    request.pQueryCompletionCallback = os_dns_completion_a;

    pb->os_dns.query_result_a.Version = DNS_QUERY_REQUEST_VERSION1;

    status = DnsQueryEx(&request,
                        &pb->os_dns.query_result_a,
                        &pb->os_dns.cancel_a);
    if (status != DNS_REQUEST_PENDING && status != ERROR_SUCCESS) {
        PUBNUB_LOG_ERROR(pb,
                         "DnsQueryEx(A) failed: status=%ld\n",
                         (long)status);
        pb->os_dns.active = false;
        return -1;
    }
    if (status == ERROR_SUCCESS) {
        /* Synchronous completion -- mark as done. */
        InterlockedExchange(&pb->os_dns.status_a, (LONG)ERROR_SUCCESS);
        InterlockedExchange(&pb->os_dns.completed_a, 1);
    }

#if PUBNUB_USE_IPV6
    memset(&request, 0, sizeof request);
    request.Version             = DNS_QUERY_REQUEST_VERSION1;
    request.QueryName           = whostname;
    request.QueryType           = DNS_TYPE_AAAA;
    request.QueryOptions        = DNS_QUERY_STANDARD;
    request.pQueryContext        = pb;
    request.pQueryCompletionCallback = os_dns_completion_aaaa;

    pb->os_dns.query_result_aaaa.Version = DNS_QUERY_REQUEST_VERSION1;

    status = DnsQueryEx(&request,
                        &pb->os_dns.query_result_aaaa,
                        &pb->os_dns.cancel_aaaa);
    if (status != DNS_REQUEST_PENDING && status != ERROR_SUCCESS) {
        PUBNUB_LOG_WARNING(pb,
                           "DnsQueryEx(AAAA) failed: status=%ld, "
                           "continuing with A only\n",
                           (long)status);
        /* Mark AAAA as completed so we don't wait for it. */
        InterlockedExchange(&pb->os_dns.status_aaaa,
                            (LONG)DNS_ERROR_RCODE_SERVER_FAILURE);
        InterlockedExchange(&pb->os_dns.completed_aaaa, 1);
    }
    if (status == ERROR_SUCCESS) {
        InterlockedExchange(&pb->os_dns.status_aaaa, (LONG)ERROR_SUCCESS);
        InterlockedExchange(&pb->os_dns.completed_aaaa, 1);
    }
#else
    /* IPv6 disabled -- mark AAAA as completed immediately. */
    InterlockedExchange(&pb->os_dns.completed_aaaa, 1);
#endif

    PUBNUB_LOG_TRACE(pb,
                     "DnsQueryEx started for '%s'\n",
                     hostname);
    return 0;
}


/* ------------------------------------------------------------------ */
/*  Completion callbacks  (run on Windows thread pool thread)           */
/* ------------------------------------------------------------------ */

static VOID WINAPI os_dns_completion_a(PVOID             pQueryContext,
                                       PDNS_QUERY_RESULT pQueryResults)
{
    pubnub_t* pb = (pubnub_t*)pQueryContext;

    InterlockedExchange(&pb->os_dns.status_a,
                        (LONG)pQueryResults->QueryStatus);
    InterlockedExchange(&pb->os_dns.completed_a, 1);

    /* Wake the socket-watcher thread so the FSM picks this up. */
    pbntf_requeue_for_processing(pb);
}


#if PUBNUB_USE_IPV6
static VOID WINAPI os_dns_completion_aaaa(PVOID             pQueryContext,
                                          PDNS_QUERY_RESULT pQueryResults)
{
    pubnub_t* pb = (pubnub_t*)pQueryContext;

    InterlockedExchange(&pb->os_dns.status_aaaa,
                        (LONG)pQueryResults->QueryStatus);
    InterlockedExchange(&pb->os_dns.completed_aaaa, 1);

    pbntf_requeue_for_processing(pb);
}
#endif /* PUBNUB_USE_IPV6 */


/* ------------------------------------------------------------------ */
/*  pbpal_os_dns_check                                                 */
/* ------------------------------------------------------------------ */

int pbpal_os_dns_check(pubnub_t* pb)
{
    PDNS_RECORD rec;
    bool        got_any = false;

    PUBNUB_ASSERT_OPT(pb != NULL);

    if (!InterlockedCompareExchange(&pb->os_dns.completed_a, 0, 0)
        || !InterlockedCompareExchange(&pb->os_dns.completed_aaaa, 0, 0)) {
        return +1;
    }

    if ((DNS_STATUS)pb->os_dns.status_a == ERROR_SUCCESS
        && pb->os_dns.query_result_a.pQueryRecords != NULL) {
#if PUBNUB_USE_MULTIPLE_ADDRESSES
        int ipv4_idx = 0;
#endif
        for (rec = pb->os_dns.query_result_a.pQueryRecords;
             rec != NULL;
             rec = rec->pNext) {
            if (rec->wType != DNS_TYPE_A)
                continue;

            if (!pb->dns_queries.received_a) {
                pb->dns_queries.dns_a_addr.sin_family = AF_INET;
                memcpy(&pb->dns_queries.dns_a_addr.sin_addr.s_addr,
                       &rec->Data.A.IpAddress,
                       4);
                pb->dns_queries.received_a = true;
                pb->dns_queries.sent_a     = true;
                got_any = true;
                PUBNUB_LOG_TRACE(pb,
                                 "DnsQueryEx A resolved: %u.%u.%u.%u "
                                 "(TTL=%lu)\n",
                                 ((uint8_t*)&rec->Data.A.IpAddress)[0],
                                 ((uint8_t*)&rec->Data.A.IpAddress)[1],
                                 ((uint8_t*)&rec->Data.A.IpAddress)[2],
                                 ((uint8_t*)&rec->Data.A.IpAddress)[3],
                                 (unsigned long)rec->dwTtl);
            }
            /* Store ALL addresses (including primary) in the spare
               array to match the legacy DNS codec behavior.  The retry
               logic increments ipv4_index before trying the next one. */
#if PUBNUB_USE_MULTIPLE_ADDRESSES
            if (ipv4_idx < PUBNUB_MAX_IPV4_ADDRESSES) {
                memcpy(pb->spare_addresses.ipv4_addresses[ipv4_idx].ipv4,
                       &rec->Data.A.IpAddress,
                       4);
                pb->spare_addresses.ttl_ipv4[ipv4_idx] =
                    (uint16_t)(rec->dwTtl > 0xFFFF ? 0xFFFF : rec->dwTtl);
                ipv4_idx++;
            }
#endif
        }
#if PUBNUB_USE_MULTIPLE_ADDRESSES
        pb->spare_addresses.n_ipv4      = ipv4_idx;
        pb->spare_addresses.ipv4_index  = 0;
#endif
    }
    else {
        pb->dns_queries.received_a = true;
        pb->dns_queries.sent_a     = true;
        PUBNUB_LOG_DEBUG(pb,
                         "DnsQueryEx A: no records (status=%ld)\n",
                         (long)pb->os_dns.status_a);
    }

#if PUBNUB_USE_IPV6
    if ((DNS_STATUS)pb->os_dns.status_aaaa == ERROR_SUCCESS
        && pb->os_dns.query_result_aaaa.pQueryRecords != NULL) {
#if PUBNUB_USE_MULTIPLE_ADDRESSES
        int ipv6_idx = 0;
#endif
        for (rec = pb->os_dns.query_result_aaaa.pQueryRecords;
             rec != NULL;
             rec = rec->pNext) {
            if (rec->wType != DNS_TYPE_AAAA)
                continue;

            if (!pb->dns_queries.received_aaaa) {
                struct sockaddr_in6* sin6 =
                    (struct sockaddr_in6*)&pb->dns_queries.dns_aaaa_addr;
                sin6->sin6_family = AF_INET6;
                memcpy(sin6->sin6_addr.s6_addr,
                       &rec->Data.AAAA.Ip6Address,
                       16);
                pb->dns_queries.received_aaaa = true;
                pb->dns_queries.sent_aaaa     = true;
                got_any = true;
                PUBNUB_LOG_TRACE(pb, "DnsQueryEx AAAA resolved (TTL=%lu)\n",
                                 (unsigned long)rec->dwTtl);
            }
#if PUBNUB_USE_MULTIPLE_ADDRESSES
            if (ipv6_idx < PUBNUB_MAX_IPV6_ADDRESSES) {
                memcpy(pb->spare_addresses.ipv6_addresses[ipv6_idx].ipv6,
                       &rec->Data.AAAA.Ip6Address,
                       16);
                pb->spare_addresses.ttl_ipv6[ipv6_idx] =
                    (uint16_t)(rec->dwTtl > 0xFFFF ? 0xFFFF : rec->dwTtl);
                ipv6_idx++;
            }
#endif
        }
#if PUBNUB_USE_MULTIPLE_ADDRESSES
        pb->spare_addresses.n_ipv6     = ipv6_idx;
        pb->spare_addresses.ipv6_index = 0;
#endif
    }
    else {
        pb->dns_queries.received_aaaa = true;
        pb->dns_queries.sent_aaaa     = true;
        PUBNUB_LOG_DEBUG(pb,
                         "DnsQueryEx AAAA: no records (status=%ld)\n",
                         (long)pb->os_dns.status_aaaa);
    }
#endif /* PUBNUB_USE_IPV6 */

#if PUBNUB_USE_MULTIPLE_ADDRESSES
    time(&pb->spare_addresses.time_of_the_last_dns_query);
#endif

    if (pb->os_dns.query_result_a.pQueryRecords != NULL) {
        DnsRecordListFree(pb->os_dns.query_result_a.pQueryRecords,
                          DnsFreeRecordList);
        pb->os_dns.query_result_a.pQueryRecords = NULL;
    }
#if PUBNUB_USE_IPV6
    if (pb->os_dns.query_result_aaaa.pQueryRecords != NULL) {
        DnsRecordListFree(pb->os_dns.query_result_aaaa.pQueryRecords,
                          DnsFreeRecordList);
        pb->os_dns.query_result_aaaa.pQueryRecords = NULL;
    }
#endif

    pb->os_dns.active = false;

    if (!got_any) {
        PUBNUB_LOG_ERROR(pb,
                         "DnsQueryEx: no addresses resolved "
                         "(A status=%ld, AAAA status=%ld)\n",
                         (long)pb->os_dns.status_a,
                         (long)pb->os_dns.status_aaaa);
        return -1;
    }

    return 0;
}


/* ------------------------------------------------------------------ */
/*  pbpal_os_dns_cancel                                                */
/* ------------------------------------------------------------------ */

void pbpal_os_dns_cancel(pubnub_t* pb)
{
    if (pb == NULL || !pb->os_dns.active)
        return;

    if (!InterlockedCompareExchange(&pb->os_dns.completed_a, 0, 0))
        DnsCancelQuery(&pb->os_dns.cancel_a);

#if PUBNUB_USE_IPV6
    if (!InterlockedCompareExchange(&pb->os_dns.completed_aaaa, 0, 0))
        DnsCancelQuery(&pb->os_dns.cancel_aaaa);
#endif

    if (pb->os_dns.query_result_a.pQueryRecords != NULL) {
        DnsRecordListFree(pb->os_dns.query_result_a.pQueryRecords,
                          DnsFreeRecordList);
        pb->os_dns.query_result_a.pQueryRecords = NULL;
    }
#if PUBNUB_USE_IPV6
    if (pb->os_dns.query_result_aaaa.pQueryRecords != NULL) {
        DnsRecordListFree(pb->os_dns.query_result_aaaa.pQueryRecords,
                          DnsFreeRecordList);
        pb->os_dns.query_result_aaaa.pQueryRecords = NULL;
    }
#endif

    pb->os_dns.active = false;
}


#endif /* defined(_WIN32) && defined(PUBNUB_CALLBACK_API) */
