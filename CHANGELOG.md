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


