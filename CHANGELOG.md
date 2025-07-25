## v5.1.4
July 22 2025

#### Fixed
- Fix bug that UTF8 characters used in some end points were breaking the request format.

## v5.1.3
July 15 2025

#### Fixed
- Fixed memory leak when calling pubnub_free on an initialized PubNub object.
- Fixed socket leak when calling pubnub_free on a PubNub object with an open connection.
- Fixed socket leak when publishing using a context with use_http_keep_alive = 0.
- Fixed crash due to double-free when calling pubnub_free after pbpal_handle_socket_condition encounters a socket error. Fixed the following issues reported by [@blake-spangenberg](https://github.com/blake-spangenberg): [#221](https://github.com/pubnub/c-core/issues/221).

Thank you @blake-spangenberg for your contribution!

## v5.1.2
July 08 2025

#### Modified
- Use recursive semaphore for `mutex` lock and unlock in `pbpal_mutex.h`.

## v5.1.1
June 26 2025

#### Fixed
- Fix issue because of which it was impossible for sync PubNub context version to cancel operation awaited in secondary thread.

#### Modified
- Fix the code for which the compiler emitted warnings during the build for `mbedTLS` and `ESP_PLATFORM=1`. Fixed the following issues reported by [@blake-spangenberg](https://github.com/blake-spangenberg): [#213](https://github.com/pubnub/c-core/issues/213).

## v5.1.0
June 19 2025

#### Added
- Add functions which allow to enable and disable "smart heartbeat" behavior for automatic heartbeat: `pubnub_enable_smart_heartbeat` and `pubnub_disable_smart_heartbeat`.
- Add `pubnub_last_publish_timetoken` function to retrieve recently published message high precision timetoken.

#### Fixed
- Fix issue which caused deadlock on secondary thread and as result heartbeat stop.
- Fix signature of `pbntf_init_callback` and its usage in Windows dedicated code.

## v5.0.3
June 17 2025

#### Fixed
- Fixed wrong string copy if multiple channels(groups) are provided for `pubnub_set_state`. Note that using `memcpy` instead of `strncpy` is used intentionally because of the ESP support.
- Fixed crash when double comma is provided to `pubnub_set_state` as a channels(groups).
- Fixed wrong check if the reallocation is required in `pubnub_set_state`.
- Removed additional allocation of memory for temporary data.
- Changed the amount of bytes allocated based on the provided parameters instead of hardcoded values.

## v5.0.2
June 02 2025

#### Fixed
- Fix `pubnub_reconnect` and `pubnub_disconnect` not to print an error when called within incorrect subscription state. Warning will be printed instead and function won't execute further.

## v5.0.1
May 26 2025

#### Fixed
- Fix crashes that could sometimes happen during unsubscribe due to calling `strlen` on null pointers.
- Fix `pubnub_fetch_history` function when used with crypto api. Change `PUBNUB_MAX_URL_PARAMS` to 12.
- Fix the issue that was causing memory fragmentation fault at the moment of the next automated heartbeat call.

## v5.0.0
April 03 2025

#### Added
- Add `PUBNUB_NTF_RUNTIME_SELECTION` flag that allows to select API type at runtime.
- Introduce `void*` parameter for user data in listeners to let callbacks keep context on demand.

WARNING: Release contains breaking changes!

## v4.19.1
April 02 2025

#### Fixed
- Fix the issue that there was an assertion when using App Context functions with `limit` set to 100.

## v4.19.0
March 19 2025

#### Added
- Add the ability to set `timetoken` (for message catchup) in `pubnub_subscribe_v2_options` which is used with `pubnub_subscribe_v2`.

#### Fixed
- Fix issue which was source of segmentation fault when tried to access memory after it has been freed.

## v4.18.1
February 11 2025

#### Fixed
- Fix issue because of which `signature` value in query has been truncated.

## v4.18.0
February 06 2025

#### Added
- Add `status` and `type` support for channel and uuid metadata objects state update API.

## v4.17.0
January 16 2025

#### Added
- Add possibility to replace default log function with a callback provided by the user.

## v4.16.1
January 09 2025

#### Fixed
- Remove the `user_id` length restriction for dynamically allocated memory for it.

#### Modified
- Configure `try_compile` when building a static library.

## v4.16.0
December 25 2024

#### Added
- Additional flags for C/CPP can be set with: `USER_C_FLAGS` / `USER_CXX_FLAGS`.

#### Fixed
- Fix because of which one of the source files has been missed for Windows.
- Fix issue with unsupported concatenation of sources files / definitions (`+=`).

#### Modified
- Refactor our `Makefiles` from different folders and platforms to use `include` directives to include shared definitions, flags, source files.

## v4.15.0
November 25 2024

#### Added
- Add custom message type support for the following APIs: publish, signal, share file, subscribe and history.
- Add `pubnub_set_ipv4_connectivity` and `pubnub_set_ipv6_connectivity` to `pubnub_coreapi` to switch preferred connectivity protocol.

#### Fixed
- Make sure that in case of connection close (including because of error) proxy context object will be reset.

## v4.14.1
October 24 2024

#### Fixed
- Fix CMakeLists to build correctly on Windows.

#### Modified
- Prepare CMakeLists to support builds for Arm64 architecture.

## v4.14.0
October 15 2024

#### Added
- Add core Event Engine implementation with the required set of types and methods.
- Add Subscribe Event Engine built atop of the core Event Engine implementation.
- Add the following entities: channel, channel group, uuid and channel metadata objects.
- Add objects to manage subscriptions and provides interface for update listeners.
- Add new event listeners, which make it possible to add listeners to a specific entity or group of entities (though subscription and subscription set).
- Added ability to configure automated retry policies for failed requests.

## v4.13.1
September 05 2024

#### Fixed
- Removed additional null byte character.

## v4.13.0
August 09 2024

#### Added
- Add `delete message` API support to the advanced history module.

## v4.12.3
August 05 2024

#### Fixed
- Add the missing `ttl` parameter to the `pubnub_publish_options` for extended `publish` configuration.

## v4.12.2
August 05 2024

#### Fixed
- Fixed custom `bool` type for CMake builds.

## v4.12.1
August 05 2024

#### Fixed
- Fix query values for boolean flags for history endpoint (`include meta`, `include uuid`, `include message type` and `reverse`).

## v4.12.0
July 29 2024

#### Added
- Added `filter` and `sort` parameters to be closer to the other SDKs with object API.
- Configurable `bool` type.

#### Fixed
- Missing features needed for grant token API in CMakeLIsts.txt.

## v4.11.2
July 15 2024

#### Fixed
- Added missing subscribe v2 crypto implementation.

## v4.11.1
June 28 2024

#### Fixed
- Fixed `cmake` build for not ESP32 builds.

## v4.11.0
June 27 2024

#### Added
- Provided support for ESP32 devices via ESP-IDF framework.
- Provided support for MBedTLS library used within esp32 platform.

#### Modified
- Replace `strncpy` with `strcpy` in blocks where it is safer to be used.

## v4.10.0
June 14 2024

#### Added
- Added possibility to use strings in actions API.

#### Modified
- `pubnub_action_type` enum has been deprecated.



## v4.9.1
March 26 2024

#### Fixed
- Fix too small amount of memory allocated for aes cbc algorithm in some cases.

#### Modified
- Add possibility to include address sanitizer in build via CMake.

## v4.9.0
January 08 2024

#### Added
- Provide CMake support.
- Adjust `build.cs` unreal engine file for CMake build.

## v4.8.0
December 07 2023

#### Added
- Add `#if` switches into files that are related to PubNub features to not rely only on makefiles. [Be careful when update. It's not a breaking change at all but might fail build for custom makefiles!].

## v4.7.1
November 23 2023

#### Fixed
- Handle unencrypted message while getting messages with crypto.

## v4.7.0
November 20 2023

#### Added
- Provided `PUBNUB_EXTERN` macro to extern C functions.

## v4.6.2
November 14 2023

#### Fixed
- Fix `pubnub_free()` function on not initialised PubNub that can cause exceptions/undefined behaviours.

## v4.6.1
November 08 2023

#### Fixed
- Provide missing `publish()` function overload for QT wrapper that allows set publish related options.

## v4.6.0
October 30 2023

#### Added
- Add the `PUBNUB_QT_MOVE_TO_THREAD` flag as default to give users the opportunity to manage threads by themselves.

#### Fixed
- Move `pubnub_qt` into QT main thread by default to be sure that timers will be run in it.

## v4.5.0
October 16 2023

#### Added
- Update the crypto module structure and add enhanced AES-CBC cryptor.

#### Fixed
- Improved security of crypto implementation by increasing the cipher key entropy by a factor of two.
- Fixed missing return from failed `pbaes256_decrypt_alloc()` function.

## v4.4.0
September 28 2023

#### Added
- Provide module files to integrate SDK with Unreal Enigne.

## v4.3.0
July 24 2023

#### Added
- Add `publisher` field into `pubnub_v2_message`.

#### Fixed
- Fixed `flags` and `region` values that always equaled `0`.

## v4.2.2
May 24 2023

#### Fixed
- Conditionally use of using newest openssl API Ipv4 parsing is needed for working with proxy. Include object file with that function for proxy builds.

#### Modified
- Use newest openssl API.

## v4.2.1
April 26 2023

#### Modified
- Conditionally use `sha256` when build is linked with OpenSSL 3+ version.

## v4.2.0
February 07 2023

#### Added
- Updated QT to version `6.*`.

#### Fixed
- Fixed not building QT module.
- Align QT module with current SDK state.

#### Modified
- Removed some states, classes and structs that aren't currently used.

## v4.1.0
January 16 2023

#### Added
- Added pubnub_set_state_ex to support heartbeat.

#### Fixed
- Removed state param from subscribe request.
- Added state param to hearbeat request.

## v4.0.6
December 14 2022

#### Fixed
- Fixed hanging allocated memory after error in `parse_token` by cleaning the result memory on `cbor` error.

#### Modified
- Implemented more tests for `pubnub_token_parse` function to increase confidence about that function.
- Refactored implementation of some tests' setups.

## v4.0.5
December 02 2022

#### Fixed
- Fixed compilation error for MSVC in `pubnub_parse_token` function.

## v4.0.4
November 25 2022

#### Fixed
- Fixed crashing parsing token for not valid values by logging an error and returning `NULL`.

## v4.0.3
November 17 2022

#### Fixed
- Fixed wrong pointer reallocation in string concatenation.
- Fixed allocation counter that was not taking to the account recursed allocations.

## v4.0.2
November 15 2022

#### Fixed
- Improved accuracy of the base64 encoding size what fixes buffer underflow in encryption module.
- Fixed undefined behaviours in `pubnub_encrypt_decrypt_iv_sample.c` by including some additional checks and variable initialisations.

#### Modified
- Made same base for encrypt functions what makes codes easier to understand and maintain.

## v4.0.1
November 08 2022

#### Fixed
- `ERR_load_BIO_strings()` is deprecated in OpenSSL 3.0. Low-level encoding primitives are also deprecated. `EVP_EncodeBlock()` is available in all currently supported OpenSSL releases.

## v4.0.0
November 02 2022

#### Added
- Add `user_id` configuration option that deprecates `uuid` ones.
- BREAKING CHANGES: now `user_id` (old `uuid`) is a required property!.

## v3.5.2
October 11 2022

#### Fixed
- Fix memory leak in cpp `parse_token` method.
- Fix buffer overflow in core `pubnub_parse_token` function for some cases.
- Fix buffer overflow in core `pubnub_encrypt` function for randomized initial vector.

## v3.5.1
September 22 2022

#### Fixed
- Fix wrong parsing uuid in parse_token.
- Fix case sensitive header check.

## v3.5.0
September 08 2022

#### Added
- Implemented Fetch History.

## v3.4.3
July 05 2022

#### Fixed
- Removed extra parenthesis in get_dns_ip function code.
- Added uuid query param to history, set/get state, wherenow, channel-group operations.

## v3.4.2
April 25 2022

#### Fixed
- Handle state for subscribe and resubscribe.

## v3.4.1
March 09 2022

#### Fixed
- Support system name servers in async DNS client.
- Fix multiple memory safety and leak issues.
- Fix slash char encoding for pnsdk.

## v3.4.0
January 11 2022

#### Added
- Add token permissions revoke functionality.

#### Fixed
- Remove body from `signature` calculation for requests with DELETE HTTP method.

#### Modified
- Update `.pubnub.yml` file with access token revoke and secret key all access.

## v3.3.2
January 10 2022

#### Fixed
- Filter-expr query param typo.

## v3.3.1
January 05 2022

#### Fixed
- Encode = (equal) sign for filter expression.

## [v3.3.0](https://github.com/pubnub/c-core/releases/tag/v3.3.0)
October 11 2021

[Full Changelog](https://github.com/pubnub/c-core/compare/v3.2.0...v3.3.0)

- Implemented PAMv3 support. 
- Handle subscribe error for empty channel-group. 


