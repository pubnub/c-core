/* -*- c-file-style:"stroustrup"; indent-tabs-mode: nil -*- */

/** @file pubnub_sync_proxy_manual_sample.c

    Minimal sample demonstrating a synchronous publish through a
    manually configured HTTP CONNECT proxy with NTLM authentication.

    Prerequisites (Windows):
      - VS Code with C/C++ extension
      - Build Tools for Visual Studio (provides cl.exe, nmake, Windows SDK)
        Download: https://visualstudio.microsoft.com/downloads/
        Select "Desktop development with C++" workload during install.

    Build (from the repo root, in a VS Developer Command Prompt or
    terminal initialized with vcvars64.bat):

        cd windows
        set "USE_PROXY=1" && nmake /f windows.mk pubnub_sync_proxy_manual_sample.exe

    Run:

        pubnub_sync_proxy_manual_sample.exe <proxy_host> <proxy_port> [DOMAIN\user] [password]

    If username/password are omitted, SSPI uses the current Windows
    logon credentials (transparent/SSO authentication).
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
#include <string.h>


int main(int argc, char* argv[])
{
    pubnub_t*              pb;
    enum pubnub_res        res;
    char const*            chan = "hello_world";
    char const*            proxy_host;
    uint16_t               proxy_port;
    char const*            username = NULL;
    char const*            password = NULL;
    struct Pubnub_UUID     uuid;
    struct Pubnub_UUID_String str_uuid;

    if (argc < 3) {
        fprintf(stderr,
                "Usage: %s <proxy_host> <proxy_port> [DOMAIN\\user] [password]\n",
                argv[0]);
        return 1;
    }

    proxy_host = argv[1];
    proxy_port = (uint16_t)atoi(argv[2]);
    if (argc > 3) { username = argv[3]; }
    if (argc > 4) { password = argv[4]; }

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
        pubnub_set_user_id(pb, "proxy-manual-sample");
    }

    printf("Setting proxy: %s:%u (HTTP CONNECT)\n", proxy_host, proxy_port);
    if (0 != pubnub_set_proxy_manual(pb, pbproxyHTTP_CONNECT, proxy_host, proxy_port)) {
        fprintf(stderr, "Failed to set proxy\n");
        pubnub_free(pb);
        return 1;
    }

    if (username != NULL) {
        printf("Setting credentials: user=%s\n", username);
        pubnub_set_proxy_authentication_username_password(pb, username, password);
    }
    else {
        printf("Using current Windows logon credentials (SSPI/SSO)\n");
    }

    printf("Publishing to channel '%s'...\n", chan);
    res = pubnub_publish(pb, chan, "\"Hello from manual proxy sample\"");
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
