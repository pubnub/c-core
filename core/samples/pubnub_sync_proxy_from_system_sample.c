/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

/** @file pubnub_sync_proxy_from_system_sample.c

    Minimal sample demonstrating a synchronous publish using the
    system-configured proxy (auto-detected from Windows IE/WinHTTP
    settings). NTLM authentication uses the current logon session
    credentials via SSPI automatically.

    Prerequisites (Windows):
      - VS Code with C/C++ extension
      - Build Tools for Visual Studio (provides cl.exe, nmake, Windows SDK)
        Download: https://visualstudio.microsoft.com/downloads/
        Select "Desktop development with C++" workload during install.

    Build (from the repo root, in a VS Developer Command Prompt or
    terminal initialized with vcvars64.bat):

        cd windows
        set "USE_PROXY=1" && nmake /f windows.mk pubnub_sync_proxy_from_system_sample.exe

    Run:

        pubnub_sync_proxy_from_system_sample.exe

    The proxy is auto-detected from Windows system settings (IE LAN
    settings, WinHTTP proxy, or WPAD/PAC auto-discovery).
*/

#include "pubnub_sync.h"

#include "core/pubnub_helper.h"
#include "core/pubnub_alloc.h"
#include "core/pubnub_pubsubapi.h"
#include "core/pubnub_coreapi.h"
#include "core/pubnub_proxy.h"
#include "core/pubnub_generate_uuid.h"

#include <stdio.h>
#include <stdlib.h>


int main(int argc, char* argv[])
{
    pubnub_t*              pb;
    enum pubnub_res        res;
    char const*            chan = "hello_world";
    struct Pubnub_UUID     uuid;
    struct Pubnub_UUID_String str_uuid;

    (void)argc;
    (void)argv;

    pb = pubnub_alloc();
    if (NULL == pb) {
        fprintf(stderr, "Failed to allocate PubNub context\n");
        return 1;
    }
    pubnub_init(pb, "demo", "demo");

    if (0 == pubnub_generate_uuid_v4_random(&uuid)) {
        str_uuid = pubnub_uuid_to_string(&uuid);
        pubnub_set_user_id(pb, str_uuid.uuid);
    }
    else {
        pubnub_set_user_id(pb, "proxy-system-sample");
    }

    printf("Detecting proxy from system configuration...\n");
    if (0 != pubnub_set_proxy_from_system(pb, pbproxyHTTP_CONNECT)) {
        fprintf(stderr,
                "No system proxy found or auto-detection failed.\n"
                "Check IE LAN settings or WinHTTP proxy configuration.\n");
        pubnub_free(pb);
        return 1;
    }
    printf("System proxy configured successfully.\n");

    printf("Publishing to channel '%s'...\n", chan);
    res = pubnub_publish(pb, chan, "\"Hello from system proxy sample\"");
    if (PNR_STARTED == res) { res = pubnub_await(pb); }

    if (PNR_OK == res) {
        printf("Published OK! Response: %s\n", pubnub_last_publish_result(pb));
    }
    else {
        fprintf(stderr,
                "Publish failed: %d (%s)\n",
                res,
                pubnub_res_2_string(res));
    }

    pubnub_free(pb);
    return (res == PNR_OK) ? 0 : 1;
}
