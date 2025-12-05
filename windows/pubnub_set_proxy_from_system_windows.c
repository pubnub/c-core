/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */
#include "core/pubnub_proxy.h"

#include "pubnub_internal.h"
#include "core/pubnub_log.h"
#include "core/pubnub_assert.h"

#include <winhttp.h>

#include <stdio.h>
#include <wchar.h>

#pragma comment(lib, "winhttp")


static void free_winhttp_stuff(WINHTTP_CURRENT_USER_IE_PROXY_CONFIG *ie_proxy_cfg,
                               WINHTTP_PROXY_INFO *proxy_info)
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

    if (proxy_info->lpszProxy != NULL) {
        GlobalFree(proxy_info->lpszProxy);
    }
    if (proxy_info->lpszProxyBypass != NULL) {
        GlobalFree(proxy_info->lpszProxyBypass);
    }
}


// This is a "dirty" function, it changes @p url4proxy, but, it's our own,
// local, beast, so it's not so bad.
static int set_from_url4proxy(pubnub_t *p, wchar_t *url4proxy)
{
    wchar_t *it;
    wchar_t *end = url4proxy;

    PUBNUB_LOG_TRACE("set_from_url4proxy(url4proxy=%S)\n", url4proxy);
    for (it = url4proxy; *end != '\0'; it = end + 1) {
        wchar_t *port;
        wchar_t *hostname_start;
        wchar_t *hostname_end;
        const size_t separator_position = wcscspn(it, L"; ");

        end = it + separator_position;

        // Skipping schema (if 'http://' or 'https://' is present).
        if (end - it > 3 && it[0] != L'[') {
            wchar_t *scheme_end = wmemchr(it, L':', end - it);
            if (scheme_end != NULL && scheme_end[1] == L'/')
                it = scheme_end + 3;
        }

        if (it[0] == L'[') {
            // IPv6 format: [2001:db8::1]:8080 or [2001:db8::1]
            hostname_start = it + 1;
            hostname_end = wmemchr(hostname_start, L']', end - hostname_start);

            if (hostname_end == NULL) {
                PUBNUB_LOG_WARNING("Malformed IPv6 proxy address (missing ']'): %S\n", it);
                continue;
            }

            if (hostname_end < end && hostname_end[1] == L':')
                port = hostname_end + 1;
            else
                port = end;
        } else {
            // IPv4 or hostname format: 192.168.1.1:8080 or test.proxy.com:3128
            hostname_start = it;
            port = NULL;

            for (wchar_t *p_scan = it; p_scan < end; p_scan++) {
                if (*p_scan == L':')
                    port = p_scan;
            }

            hostname_end = (port != NULL) ? port : end;
        }

        int bytes_written = WideCharToMultiByte(CP_UTF8,
                                                0,
                                                hostname_start,
                                                hostname_end - hostname_start,
                                                p->proxy_hostname,
                                                sizeof(p->proxy_hostname) - 1,
                                                NULL,
                                                NULL);

        if (bytes_written == 0) {
            PUBNUB_LOG_WARNING("WideCharToMultiByte failed for proxy hostname: %lu\n",
                             GetLastError());
            continue;
        }
        if (bytes_written >= sizeof(p->proxy_hostname)) {
            PUBNUB_LOG_WARNING("Proxy hostname too long (needs %d bytes)\n",
                             bytes_written);
            continue;
        }

        p->proxy_hostname[bytes_written] = '\0';

        PUBNUB_LOG_TRACE("Set proxy_hostname = %s\n", p->proxy_hostname);

        if (port != NULL && port < end) {
            const wchar_t saved_char = *end;
            *end = L'\0';
            p->proxy_port = (uint16_t)wcstol(port + 1, NULL, 10);
            *end = saved_char;

            PUBNUB_LOG_TRACE("Set proxy_port = %hu\n", p->proxy_port);
            if (0 == p->proxy_port)
                continue;
        }
        else
            p->proxy_port = 80;
    }

    return -1;
}


int pubnub_set_proxy_from_system(pubnub_t *p, enum pubnub_proxy_type protocol)
{
    WINHTTP_CURRENT_USER_IE_PROXY_CONFIG ie_proxy_cfg = {0};
    WINHTTP_AUTOPROXY_OPTIONS autoproxy_opts = {0};
    WINHTTP_PROXY_INFO proxy_info = {0};
    bool use_auto_proxy = true;
    wchar_t *url4proxy = NULL;
    int rslt;

    if (WinHttpGetIEProxyConfigForCurrentUser(&ie_proxy_cfg)) {
        if (ie_proxy_cfg.lpszProxy != NULL) {
            PUBNUB_LOG_INFO("Found proxy in Registry: %S",
                            ie_proxy_cfg.lpszProxy);
            url4proxy = ie_proxy_cfg.lpszProxy;
            /* This may be overriden if auto-detect is also on */
        }

        if (ie_proxy_cfg.lpszAutoConfigUrl != NULL) {
            autoproxy_opts.lpszAutoConfigUrl = ie_proxy_cfg.lpszAutoConfigUrl;

            PUBNUB_LOG_INFO("Will use Proxy PAC: %S\n",
                            autoproxy_opts.lpszAutoConfigUrl);
        }
        else {
            use_auto_proxy = ie_proxy_cfg.fAutoDetect;
        }
    }

    if (use_auto_proxy) {
        char const *origin = PUBNUB_ORIGIN_SETTABLE ? p->origin : PUBNUB_ORIGIN;
        char url_for_proxy[256];
        wchar_t wide_origin[256];
        HINTERNET winhttp;
        int url_len;

#if PUBNUB_USE_SSL
        char const *scheme = p->options.useSSL ? "https" : "http";
#else /* PUBNUB_USE_SSL */
        char const *scheme = "http";
#endif /* !PUBNUB_USE_SSL */

        url_len = snprintf(url_for_proxy, sizeof(url_for_proxy),
                           "%s://%s", scheme, origin);
        if (url_len < 0 || url_len >= sizeof(url_for_proxy)) {
            PUBNUB_LOG_ERROR("Origin URL too long: '%s'\n", origin);
            return -1;
        }

        if (0 ==
            MultiByteToWideChar(CP_UTF8, 0, url_for_proxy, -1, wide_origin,
                                sizeof wide_origin / sizeof wide_origin[0])) {
            PUBNUB_LOG_ERROR("Origin '%s' to wide string failed: %lu\n",
                             url_for_proxy, GetLastError());
            return -1;
        }
        winhttp = WinHttpOpen(L"C-core", WINHTTP_ACCESS_TYPE_NO_PROXY,
                              WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
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

            if (WinHttpGetProxyForUrl(winhttp, wide_origin, &autoproxy_opts,
                                      &proxy_info)) {
                if (proxy_info.lpszProxy != NULL) {
                    url4proxy = proxy_info.lpszProxy;
                    PUBNUB_LOG_INFO("Auto detected proxy: %S\n",
                                    proxy_info.lpszProxy);
                }
            }
            else {
                PUBNUB_LOG_WARNING("Cannot auto detect proxy, "
                                   "WinHttpGetProxyForUrl() returns: %lu\n",
                                   GetLastError());
            }
            WinHttpCloseHandle(winhttp);
        }
        else {
            PUBNUB_LOG_WARNING("WinHttpOpen() error: %lu\n", GetLastError());
        }
    }

    rslt = (NULL == url4proxy) ? -1 : set_from_url4proxy(p, url4proxy);
    WATCH_INT(rslt);
    if (0 == rslt) {
        p->proxy_type = protocol;
    }
    free_winhttp_stuff(&ie_proxy_cfg, &proxy_info);

    return rslt;
}
