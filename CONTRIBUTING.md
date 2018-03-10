## How to contribute to PubNub C-core

#### Fixing code issues

If you have a fix for an issue, please create a Pull Request "against"
the `develop` branch.  Explain your fix in the PR comment (what it
fixes and how).

Please try to follow the existing coding style as much as possible and
avoid cosmetic (whitespace, code format) changes. In fact, please use
`clang-format` with the supplied `.clang-format` configuration to
format the code before submitting a PR.

#### Adding new features

If you have a new feature you want to add, please crate a Pull Request
"against" the `develop` branch.  Explain your feature, what it does
and for what purpose.

Please understand that C-core is intended for high portability, so, if
your feature is not small, try to make it optional. If your feature is
not feasible on all platforms, make provisions for C-core to work fine
without it on other platforms.

#### General coding guidelines for PRs

For C, use C89 for code, but it's OK to use C11 libraries that use only
C89 language features (say, `snprintf()`).

For C++, use C++98 and offer convenience features for C++11 or higher
"protected" by proper `#if __cplusplus` blocks.

Please observe that a lot of C-core code is portable and used on many
platforms, so, your changes can have non-obvious impact. So, be very
careful when changing "shared" code (in `/core` and `/lib`, mostly).

#### If you have questions about the code

Please send all your questions about the code, or anything related to
Pubnub, to Support@PubNub.com.

### Issues w/documentation

Documentation for Pubnub C-core is divided by platform. For example,
POSIX docs are at https://www.pubnub.com/docs/posix-c/pubnub-c-sdk and
all can be found under https://www.pubnub.com/docs.

If you find any issue or have any question about the documentation,
send them to Support@PubNub.com.

If you have a proposal to fix or improve the documentation, please
send the description of it to Support@PubNub.com.
