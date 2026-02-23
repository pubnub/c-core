/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

/* When building with OpenSSL, we need the OpenSSL version of pubnub_internal.h
   because it has a different struct pubnub_pal definition. The build system
   should define PUBNUB_USE_SSL=1 for OpenSSL builds. */
#if PUBNUB_USE_SSL
#include "openssl/pubnub_internal.h"
#else
#include "pubnub_internal.h"
#endif
#include "core/pubnub_proxy.h"
#if PUBNUB_USE_LOGGER
#include "core/pubnub_logger.h"
#endif // PUBNUB_USE_LOGGER
#include "core/pubnub_assert.h"

#include <winhttp.h>

#include <stdio.h>
#include <wchar.h>

#pragma comment(lib, "winhttp")


static void free_winhttp_stuff(
    WINHTTP_CURRENT_USER_IE_PROXY_CONFIG* ie_proxy_cfg,
    WINHTTP_PROXY_INFO*                   proxy_info)
{
    if (ie_proxy_cfg->lpszProxyBypass != NULL) {
        GlobalFree(ie_proxy_cfg->lpszProxyBypass);
    }
    if (ie_proxy_cfg->lpszAutoConfigUrl != NULL) {
        GlobalFree(ie_proxy_cfg->lpszAutoConfigUrl);
    }
    if (ie_proxy_cfg->lpszProxy != NULL) {
        GlobalFree(ie_proxy_cfg->lpszProxy);
    }

    if (proxy_info->lpszProxy != NULL) { GlobalFree(proxy_info->lpszProxy); }
    if (proxy_info->lpszProxyBypass != NULL) {
        GlobalFree(proxy_info->lpszProxyBypass);
    }
}


// This is a "dirty" function, it changes @p url4proxy, but, it's our own,
// local, beast, so it's not so bad.
static int set_from_url4proxy(pubnub_t* p, wchar_t* url4proxy)
{
    wchar_t* it;
    wchar_t* end = url4proxy;

    PUBNUB_LOG_TRACE(p, "Set proxy from the URL: %S", url4proxy);
    for (it = url4proxy; *end != '\0'; it = end + 1) {
        wchar_t*     port;
        wchar_t*     hostname_start;
        wchar_t*     hostname_end;
        const size_t separator_position = wcscspn(it, L"; ");

        end = it + separator_position;

        // Check for protocol prefix format (e.g., "http=127.0.0.1:8888" or
        // "https=[::1]:8888")
        wchar_t* equals_sign = wmemchr(it, L'=', end - it);
        if (equals_sign != NULL) {
            // Skip the protocol prefix (e.g., "http=" or "https=")
            it = equals_sign + 1;
        }

        // Skipping schema (if 'http://' or 'https://' is present).
        if (end - it > 3 && it[0] != L'[') {
            wchar_t* scheme_end = wmemchr(it, L':', end - it);
            if (scheme_end != NULL && scheme_end[1] == L'/')
                it = scheme_end + 3;
        }

        if (it[0] == L'[') {
            // IPv6 format: [2001:db8::1]:8080 or [2001:db8::1]
            hostname_start = it + 1;
            hostname_end = wmemchr(hostname_start, L']', end - hostname_start);

            if (hostname_end == NULL) {
                PUBNUB_LOG_WARNING(
                    p, "Malformed IPv6 proxy address (missing ']'): %S", it);
                continue;
            }

            if (hostname_end < end && hostname_end[1] == L':')
                port = hostname_end + 1;
            else port = end;
        }
        else {
            // IPv4 or hostname format: 192.168.1.1:8080 or test.proxy.com:3128
            hostname_start = it;
            port           = NULL;

            for (wchar_t* p_scan = it; p_scan < end; p_scan++) {
                if (*p_scan == L':') port = p_scan;
            }

            hostname_end = (port != NULL) ? port : end;
        }

        int bytes_written = WideCharToMultiByte(
            CP_UTF8,
            0,
            hostname_start,
            hostname_end - hostname_start,
            p->proxy_hostname,
            sizeof(p->proxy_hostname) - 1,
            NULL,
            NULL);

        if (bytes_written == 0) {
            PUBNUB_LOG_WARNING(
                p,
                "WideCharToMultiByte failed for proxy hostname with error "
                "code: "
                "%lu",
                GetLastError());
            continue;
        }
        if (bytes_written >= sizeof(p->proxy_hostname)) {
            PUBNUB_LOG_WARNING(
                p,
                "Proxy hostname is too long. %d bytes needed.",
                bytes_written);
            continue;
        }

        p->proxy_hostname[bytes_written] = '\0';

        PUBNUB_LOG_TRACE(p, "Set hostname: %s", p->proxy_hostname);

        if (port != NULL && port < end) {
            const wchar_t saved_char = *end;
            *end                     = L'\0';
            p->proxy_port            = (uint16_t)wcstol(port + 1, NULL, 10);
            *end                     = saved_char;

            PUBNUB_LOG_TRACE(p, "Set proxy port: %hu", p->proxy_port);
            if (0 == p->proxy_port) continue;
        }
        else {
#if PUBNUB_USE_SSL
            p->proxy_port = p->options.useSSL ? 443 : 80;
#else  /* PUBNUB_USE_SSL */
            p->proxy_port = 80;
#endif /* !PUBNUB_USE_SSL */
        }

        return 0;
    }

    return -1;
}


int pubnub_set_proxy_from_system(pubnub_t* p, enum pubnub_proxy_type protocol)
{
    PUBNUB_ASSERT_OPT(p != NULL);

    switch (protocol) {
    case pbproxyHTTP_GET:
    case pbproxyHTTP_CONNECT:
        break;
    default:
        /* other proxy protocols not yet supported */
        return -1;
    }

    WINHTTP_CURRENT_USER_IE_PROXY_CONFIG ie_proxy_cfg   = { 0 };
    WINHTTP_AUTOPROXY_OPTIONS            autoproxy_opts = { 0 };
    WINHTTP_PROXY_INFO                   proxy_info     = { 0 };
    bool                                 use_auto_proxy = true;
    wchar_t*                             url4proxy      = NULL;
    int                                  rslt;

    if (WinHttpGetIEProxyConfigForCurrentUser(&ie_proxy_cfg)) {
        if (ie_proxy_cfg.lpszProxy != NULL) {
            PUBNUB_LOG_DEBUG(
                p, "Found proxy in the registry: %S", ie_proxy_cfg.lpszProxy);
            url4proxy = ie_proxy_cfg.lpszProxy;
            /* This may be overridden if auto-detect is also on */
        }

        if (ie_proxy_cfg.lpszAutoConfigUrl != NULL) {
            autoproxy_opts.lpszAutoConfigUrl = ie_proxy_cfg.lpszAutoConfigUrl;

            PUBNUB_LOG_DEBUG(
                p, "Use PAC: %S", autoproxy_opts.lpszAutoConfigUrl);
        }
        else {
            use_auto_proxy = ie_proxy_cfg.fAutoDetect;
        }
    }

    pubnub_mutex_lock(p->monitor);
    if (use_auto_proxy) {
        char const* origin = PUBNUB_ORIGIN_SETTABLE ? p->origin : PUBNUB_ORIGIN;
        char        url_for_proxy[256];
        wchar_t     wide_origin[256];
        HINTERNET   winhttp;
        int         url_len;

#if PUBNUB_USE_SSL
        char const* scheme = p->options.useSSL ? "https" : "http";
#else  /* PUBNUB_USE_SSL */
        char const* scheme = "http";
#endif /* !PUBNUB_USE_SSL */

        url_len = snprintf(
            url_for_proxy, sizeof(url_for_proxy), "%s://%s", scheme, origin);
        if (url_len < 0 || url_len >= sizeof(url_for_proxy)) {
            PUBNUB_LOG_ERROR(p, "Origin URL too long: '%s'", origin);
            pubnub_mutex_unlock(p->monitor);
            return -1;
        }

        if (0 == MultiByteToWideChar(
                     CP_UTF8,
                     0,
                     url_for_proxy,
                     -1,
                     wide_origin,
                     sizeof wide_origin / sizeof wide_origin[0])) {
#if PUBNUB_LOG_ENABLED(ERROR)
            if (pubnub_logger_should_log(p, PUBNUB_LOG_LEVEL_ERROR)) {
                pubnub_log_value_t data = pubnub_log_value_map_init();
                PUBNUB_LOG_MAP_SET_STRING(&data, url_for_proxy, origin)
                PUBNUB_LOG_MAP_SET_NUMBER(&data, GetLastError(), error_code)
                pubnub_log_object(
                    p,
                    PUBNUB_LOG_LEVEL_ERROR,
                    PUBNUB_LOG_LOCATION,
                    &data,
                    "URL for proxy to wide string conversion failed:");
            }
#endif
            pubnub_mutex_unlock(p->monitor);
            return -1;
        }
        winhttp = WinHttpOpen(
            L"C-core",
            WINHTTP_ACCESS_TYPE_NO_PROXY,
            WINHTTP_NO_PROXY_NAME,
            WINHTTP_NO_PROXY_BYPASS,
            0);
        if (NULL != winhttp) {
            if (autoproxy_opts.lpszAutoConfigUrl != NULL) {
                autoproxy_opts.dwFlags = WINHTTP_AUTOPROXY_CONFIG_URL;
            }
            else {
                autoproxy_opts.dwFlags = WINHTTP_AUTOPROXY_AUTO_DETECT;
                autoproxy_opts.dwAutoDetectFlags =
                    WINHTTP_AUTO_DETECT_TYPE_DHCP |
                    WINHTTP_AUTO_DETECT_TYPE_DNS_A;
            }
            autoproxy_opts.fAutoLogonIfChallenged = TRUE;

            if (WinHttpGetProxyForUrl(
                    winhttp, wide_origin, &autoproxy_opts, &proxy_info)) {
                if (proxy_info.lpszProxy != NULL) {
                    url4proxy = proxy_info.lpszProxy;
                    PUBNUB_LOG_DEBUG(
                        p, "Auto detected proxy: %S", proxy_info.lpszProxy);
                }
            }
            else {
                PUBNUB_LOG_WARNING(
                    p,
                    "Cannot auto detect proxy, WinHttpGetProxyForUrl() "
                    "returns: %lu",
                    GetLastError());
            }
            WinHttpCloseHandle(winhttp);
        }
        else {
            PUBNUB_LOG_WARNING(p, "WinHttpOpen() error: %lu", GetLastError());
        }
    }

    rslt = (NULL == url4proxy) ? -1 : set_from_url4proxy(p, url4proxy);
    PUBNUB_LOG_DEBUG(p, "Proxy set from URL with result: %d", rslt);
    if (0 == rslt) {
        p->proxy_type = protocol;
#if defined(PUBNUB_CALLBACK_API)
#if defined(PUBNUB_NTF_RUNTIME_SELECTION)
        if (PNA_CALLBACK == p->api_policy) {
#endif
            /* If we haven't got numerical address for proxy we'll have to do
               DNS resolution(from proxy host name) later on, but in order to do
               that we have to have all proxy addresses(on the given context)
               set to zeros.
             */
            if (0 != pubnub_parse_ipv4_addr(
                         p->proxy_hostname, &(p->proxy_ipv4_address))) {
                memset(
                    &(p->proxy_ipv4_address), 0, sizeof p->proxy_ipv4_address);
#if PUBNUB_USE_IPV6
                if (0 != pubnub_parse_ipv6_addr(
                             p, p->proxy_hostname, &(p->proxy_ipv6_address))) {
                    memset(
                        &(p->proxy_ipv6_address),
                        0,
                        sizeof p->proxy_ipv6_address);
                }
#endif
            }
#if PUBNUB_USE_MULTIPLE_ADDRESSES
            pbpal_multiple_addresses_reset_counters(&p->spare_addresses);
#endif
#if defined(PUBNUB_NTF_RUNTIME_SELECTION)
        } /* if (PNA_CALLBACK == p->api_policy) */
#endif
#endif /* defined(PUBNUB_CALLBACK_API) */
    }
    free_winhttp_stuff(&ie_proxy_cfg, &proxy_info);
    pubnub_mutex_unlock(p->monitor);

    return rslt;
}
