# C++ wrapper for the Pubnub C-core

This is the part of C-core which is the C++ wrapper.  It should work
on any platform that supports C++ and has a reasonably
feature-complete implementation of C-core. There are some modules
that are platform specific, but only if you use C++98. For C++11 or
later, code in the Pubnub C-core C++ wrapper is fully portable and
written in standard C++(11+).

C++ wrapper is usable with C++98, but has some improvements if you
use C++11.

The user interface is the same for both the "sync" and the "callback"
notification APIs of the C-core, though there are some differences in
usage patterns for advance usages (like publishing and subscribing
from the same thread).

There is a sample Makefile for POSIX, `posix.mk`, which will build sample
programs on POSIX:

- `pubnub_sync_sample`: a "walk-through" of the C++ wrapper API, using
  the "sync" notification "back-end"
- `pubnub_callback_sample`: a "walk-through" of the C++ wrapper API,
  using the "callback" notification "back-end" with POSIX "glue"
  code/module - built from the same source as the `pubnub_sync_sample`
- `pubnub_callback_cpp11_sample`: a "walk-through" of the C++ wrapper
  API, using the "callback" notification "back-end" with C++11 "glue"
  code/module - built from the same source as the `pubnub_sync_sample`
  and the `pubnub_callback_sample`
- `cancel_subscribe_sync_sample`: an example how to cancel a subscribe
  loop safely, using the "sync backend" 
- `subscribe_publish_callback_sample`: an example of how to have one
  outstanding publish and one outstanding subscribe transaction/operation
  at the same time, using the "callback" interface.

- `futres_nesting_sync`: an "example" of using the `pubnub::futres::then()`
  to "chain" reactions on the outcome of Pubnub transactions, using
  the "sync backend"
- `futres_nesting_callback`: an "example" of using the `pubnub::futres::then()`
  to "chain" reactions on the outcome of Pubnub transactions, using
  the "callback backend" with POSIX "glue" code/module - built from the
  same source as the `futres_nesting_sync`
- `futres_nesting_callback_cpp11`: an "example" of using the
  `pubnub::futres::then()` to "chain" reactions on the outcome of
  Pubnub transactions, using the "callback backend" with C++ 11 "glue"
  code/module - built from the same source as the
  `futres_nesting_sync` and `futres_nesting_callback`

The Makefile for Windows: `windows.mk` will build the same examples,
but they will have the `.exe` extension.

The sources of the samples are in `samples` and they are portable
across all or most hosted platforms (POSIX, Windows...).

So, to build the samples on POSIX, just run:

	make -f posix.mk

There are no special requirements for C++ wrapper of C-core on POSIX,
it should just build on any regular POSIX system with a C++ compiler. 
Since your compiler might not have the needed C++11 support, you can
build just the C++98 samples like this:

	make -f posix.mk cpp98

If you wish to build only the C++11 specific samples, you can do this:

	make -f posix.mk cpp11

To use a compiler of your choice (rather than the default one), say,
`clang`, run:

	make -f posix.mk CXX=clang++

On Windows, the Makefile is intended to be used with Microsoft (Visual
Studio) tool-chain. Visual Studio 2010 or later should work, except for
the modules that rely on C++11 support for threading, which need
VS2012 or later (for C++98 only examples, even older should work). From
Visual Studio command prompt (or "Developer command prompt"), run:

	nmake -f windows.mak

Other compilers (or frontends) that respect the Microsoft command line
switches should work, but were not tested.

Keep in mind that MSVS compilers don't support full C++11 as of this
writing (though they have significantly improved), thus didn't "bump"
`__cplusplus` to `201103L`, which means that Pubnub C++ wrapper will 
not use C++11 features itself.

## OpenSSL

We also have sample Makefiles for OpenSSL. The C++ wrapper source
code is the same as for "plain" POSIX and "plain" Windows. We just
link with OpenSSL modules (instead of "plain" ones).

The `posix_openssl.mk` will build the same executables on a POSIX
compatible system as `posix.mk`, but linking OpenSSL, thus using
SSL/TLS to communicate with PubNub. The results will be located in the 
`openssl` sub-directory.

For Windows, we have `windows_openssl.mk`, which will build the 
same executables on a Windows system as `windows.mk`, but linking 
OpenSSL, thus using SSL/TLS to communicate with PubNub. These will be 
located in the `openssl` sub-directory.
