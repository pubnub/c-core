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


