> **Legacy SDK:** C-Core remains supported for existing applications. If you are starting a new C project, use the [new PubNub C SDK](https://www.pubnub.com/docs/sdks/c).

# PubNub C-Core (legacy) SDK

[![GitHub release](https://img.shields.io/github/v/release/pubnub/c-core)](https://github.com/pubnub/c-core/releases)

PubNub provides global infrastructure for real-time, interactive applications.

Publish and receive messages in C-Core (legacy). Use this SDK for existing portable C applications on the platforms maintained by C-Core.

[Documentation](https://www.pubnub.com/docs/sdks/c-core) · [API reference](https://www.pubnub.com/docs/sdks/c-core/api-reference/publish-and-subscribe) · [Changelog](https://www.pubnub.com/docs/sdks/c-core/changelog) · [Support](https://support.pubnub.com/)

## Requirements

| Requirement | Supported version or setup |
| --- | --- |
| C or C++ compiler | [Use the compiler and platform requirements listed in C-Core Platform Support](https://www.pubnub.com/docs/sdks/c-core/platform-support) |
| Platform | [POSIX, Windows, FreeRTOS, Mbed, and other maintained C-Core targets](https://www.pubnub.com/docs/sdks/c-core/platform-support) |
| Platform-specific networking and TLS implementation | Build and link the C-Core platform implementation matching the target operating system and networking stack. |

## Installation

Clone the repository and build the POSIX target. Run `make` from the `posix` directory. The makefile paths are relative to that directory.

```sh
git clone https://github.com/pubnub/c-core.git
cd c-core/posix
make -f posix.mk
```

For Windows, FreeRTOS, OpenSSL, Qt, and CMake, see [environment setup](https://www.pubnub.com/docs/sdks/c-core). Platform notes are in [`posix/README.md`](posix/README.md), [`windows/README.md`](windows/README.md), [`openssl/README.md`](openssl/README.md), [`freertos/README.md`](freertos/README.md), and [`CMakeLists.txt`](CMakeLists.txt).

## Quickstart

This example runs in POSIX C. It subscribes to `hello_world`, publishes one message, and prints the received text.

A sync context runs one transaction at a time. The first subscribe connects and stores a timetoken. The sample publishes after that, then subscribes again to receive the message.

### Get your keys

1. Open the [PubNub Admin Portal](https://admin.pubnub.com/).
2. Create an app and a keyset for development, or select an existing development keyset.
3. Copy its publish key and subscribe key.

If Access Manager is enabled, obtain a token from your trusted backend and call `pubnub_set_auth_token(pb, token)` before accessing protected resources. Keep the secret key on the backend.

### Send and receive a message

Create `hello_world.c` and build it with the POSIX sync library from the installation step.

Replace the key placeholders. Use a User ID that identifies the user or device in your app.

```c
#include "pubnub_sync.h"

#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    enum pubnub_res result;
    char const*     message;
    pubnub_t*       pb = pubnub_alloc();

    if (NULL == pb) {
        return EXIT_FAILURE;
    }

    /* Replace with your keys from the Admin Portal.
       Use a User ID that identifies the user or device. */
    pubnub_init(pb, "YOUR_PUBLISH_KEY", "YOUR_SUBSCRIBE_KEY");
    pubnub_set_user_id(pb, "hello-world-user");

    /* Connect to hello_world. This first subscribe only gets a timetoken.
       Messages published before it finishes are missed. */
    result = pubnub_subscribe(pb, "hello_world", NULL);
    if (PNR_STARTED == result) {
        result = pubnub_await(pb);
    }
    if (PNR_OK != result) {
        printf("Failed to connect\n");
        pubnub_free(pb);
        return EXIT_FAILURE;
    }

    /* Drop anything returned by the connect so the next subscribe can start. */
    while (NULL != pubnub_get(pb)) {
        continue;
    }

    /* Publish Hello world. The payload is a JSON string. */
    result = pubnub_publish(pb, "hello_world", "\"Hello world\"");
    if (PNR_STARTED == result) {
        result = pubnub_await(pb);
    }
    if (PNR_OK != result) {
        printf("Failed to publish\n");
        pubnub_free(pb);
        return EXIT_FAILURE;
    }

    /* Subscribe again to receive the message published above. */
    result = pubnub_subscribe(pb, "hello_world", NULL);
    if (PNR_STARTED == result) {
        result = pubnub_await(pb);
    }
    if (PNR_OK != result) {
        printf("Failed to receive\n");
        pubnub_free(pb);
        return EXIT_FAILURE;
    }

    message = pubnub_get(pb);
    if (NULL == message) {
        printf("No message received\n");
        pubnub_free(pb);
        return EXIT_FAILURE;
    }
    printf("%s\n", message);

    pubnub_free(pb);
    return EXIT_SUCCESS;
}
```

### Run the example

Build `hello_world.c` with the POSIX sync library, then run the `hello_world` binary.

The terminal should show:

```text
"Hello world"
```

The printed text includes quotes because the published payload is a JSON string.

Cancel unfinished transactions with `pubnub_cancel()` where necessary and release every context created with `pubnub_alloc()` by calling `pubnub_free()`.

For a complete application, see the [getting started guide](https://www.pubnub.com/docs/sdks/c-core).

## Next steps

| Task | Documentation |
| --- | --- |
| Configure the client | [Configuration](https://www.pubnub.com/docs/sdks/c-core/api-reference/configuration) |
| Work with subscriptions and messages | [Publish & Subscribe](https://www.pubnub.com/docs/sdks/c-core/api-reference/publish-and-subscribe) |
| Check channel occupancy | [Presence](https://www.pubnub.com/docs/sdks/c-core/api-reference/presence) |
| Read message history | [Message Persistence](https://www.pubnub.com/docs/sdks/c-core/api-reference/storage-and-playback) |

## Build with an AI coding assistant

The [PubNub MCP server](https://www.pubnub.com/docs/ai/pubnub-mcp-server) gives an AI coding assistant access to PubNub SDK documentation and PubNub APIs. Connect the assistant to the hosted server at `https://mcp.pubnub.com`, or run `npx @pubnub/mcp@latest` locally.

The [server repository](https://github.com/pubnub/pubnub-mcp-server) has setup steps for VS Code, Cursor, Claude Code, Claude Desktop, Codex, Gemini CLI, and OpenCode.

## Before production

Use [Access Manager](https://www.pubnub.com/docs/sdks/c-core/api-reference/access-manager) to grant each client the permissions it needs. Keep the secret key on your backend. Never include it in a distributed client.

Reuse allocated C-Core contexts instead of allocating one for every operation. Cancel unfinished work with `pubnub_cancel()` and release contexts created by `pubnub_alloc()` with `pubnub_free()`.

Live delivery through PubNub SDKs is at-most-once. A subscriber can miss messages while disconnected or if its buffer overflows. For longer-gap recovery, see [Message Persistence](https://www.pubnub.com/docs/sdks/c-core/api-reference/storage-and-playback).

## Troubleshooting

| Symptom | Check |
| --- | --- |
| The application links the wrong C-Core platform implementation | Build and link the implementation matching the target operating system and networking stack. See [C-Core Platform Support](https://www.pubnub.com/docs/sdks/c-core/platform-support). |
| A publish succeeds but no message appears | Check the keyset and channel name. The first `pubnub_subscribe()` only connects. Publish after that call finishes, then subscribe again to receive the message. |
| A new C application is being started with C-Core | C-Core is the legacy SDK. New C applications should use the [new C SDK](https://www.pubnub.com/docs/sdks/c). |
| Extra clients or duplicate messages during development | Reuse C-Core contexts rather than allocating contexts repeatedly. Cancel unfinished work and release allocated contexts when their owner terminates. |

For setup help, see [troubleshooting](https://www.pubnub.com/docs/sdks/c-core/troubleshoot). Check [network status](https://status.pubnub.com/) for service incidents.

## Releases

Read the [changelog](https://www.pubnub.com/docs/sdks/c-core/changelog) before upgrading.

To move an existing C-Core v7 application to the new C SDK, follow [Migrating from C-Core v7](https://www.pubnub.com/docs/sdks/c/migration-guides/migrating-from-c-core-v7).

## Support and contributions

For setup or account help, contact [PubNub Support](https://support.pubnub.com/).

For a reproducible SDK bug, use [GitHub Issues](https://github.com/pubnub/c-core/issues). Include the SDK version, runtime, and a small reproduction with credentials removed.

See [CONTRIBUTING.md](https://github.com/pubnub/c-core/blob/master/CONTRIBUTING.md) and run the test suite for the affected platform before opening a pull request.

## License

See the [PubNub Software Development Kit License](https://github.com/pubnub/c-core/blob/master/LICENSE).
