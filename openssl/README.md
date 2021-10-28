# Pubnub C-core for the OpenSSL library/platform

This is the part of C-core for the OpenSSL library/platform.  OpenSSL
is portable across POSIX and Windows, so the same code is used on all
and thus we deem OpenSSL a "library/platform".

Unfortunately, OpenSSL portability is only on the source level and
only for the communication functions. There are some "peripheral" 
differences which we'll explore below.


## Pubnub OpenSSL on POSIX

OpenSSL doesn't cover threads, so there is some POSIX-specific code
for `pthreads`. There is a sample Makefile (`posix.mk`), which will 
build sample programs on  POSIX:

- `pubnub_sync_sample`: a "walk-through" of the "sync" interface (API)
- `pubnub_callback_sample`: a "walk-through" of the "callback"
	interface (API)
- `cancel_subscribe_sync_sample`: an example how to cancel a subscribe
  loop safely, using the "sync" interface
- `subscribe_publish_callback_sample`: an example of how to have one
  outstanding publish and one outstanding subscribe transaction/operation
  at the same time, using the "callback" interface.
- `pubnub_console_sync`: a simple command-line Pubnub console, using
  the "sync" interface
- `pubnub_console_callback`: a simple command-line Pubnub console, using
  the "callback" interface

During the build, two (static) libraries will be created:

- `pubnub_sync.a`: library for the "sync" interface
- `pubnub_callback.a`: library for the "callback" interface

You are free to use these libraries in your projects, but keep in mind
that they are configured for the purposes of the samples. Please check
if those are right for you before using them.


The sources of the samples are in `../core/samples`, and for the
console in `../core/samples/console`, as they are portable across all
or most hosted platforms (POSIX, Windows...).

So, to build the samples, just run:

	make -f posix.mk
	
The only requirement / prerequisite of this Makefile is that you have
an OpenSSL package installed, which should be true for most modern
POSIX systems. If that's met, it should just build with the default C
compiler. To use a compiler of your choice (rather than the default
one), say, `clang`, run:

	make -f posix.mk CC=clang

### Build without the proxy support

To build without the proxy support, pass `USE_PROXY=0` to Make, like:

    make -f posix.mk USE_PROXY=0

Of course, if you have previously done a build, you will probably want
to first clean up the artifacts of that previous build by invoking the
`clean` target, like:

	make -f posix.mk clean


## Pubnub OpenSSL on Windows

OpenSSL doesn't cover threads, so there is some Windows-specific code. 

But, the biggest issue is that OpenSSL is not a part of any Windows installation
`per se`. Actually, a typical Windows computer has dozens of OpenSSL libraries
used by various programs. We provide one solution for this in our sample
Makefile (`windows.mk`) as described below.

You should decide what OpenSSL you want to compile & link against. To do
that, pass the path of your OpenSSL of choice as macro `OPENSSLPATH` to
`nmake`. We have found that "Shinning Light Productions" OpenSSL "distribution"
(could be found here https://slproweb.com/products/Win32OpenSSL.html
at the time of this writing) works fine.

For example, the default install directory of the "Shinning Light Productions" 
OpenSSL "distribution" for 32-bit OpenSSL for Windows is 
`c:\OpenSSL-Win32`, so to use that, you would:

	nmake -f windows.mk OPENSSLPATH=c:\OpenSSL-Win32

If you don't pass `OPENSSLPATH`, it will use a default, which might not be
good for you, so please check the default in `windows.mk` before invoking 
nmake without setting `OPENSSLPATH`.

We expect that the `OPENSSLPATH` has two sub-directories:

- `include`: with the OpenSSL include files
- `lib`: with OpenSSL (import) library files

In general, this should work with both import library files (for DLLs) and
static libraries, but that depends on your OpenSSL of choice. The important
thing here is that if you use import libraries, you will have to distribute
the DLLs that are a part of your OpenSSL with your applications.
	
The sample Makefile (`windows.mk`) will build the same sample 
programs on Windows as are built on POSIX (the executables just have 
`.exe` extension on Windows). It will also build the same static
libraries (but they will have `.lib` extension on Windows).

The sample Makefile is designed to work with MS Visual Studio compilers
(MSVS 2008 or newer should work fine). Rewriting it to use some other
compiler should not be too hard, mostly would involve changing the compiler
switches. But, if you have a compiler that supports MSVC switches
(like some Clang "distributions"), you should be able to use it instead
of the MSVC standard  `cl`, like:

	nmake -f windows.mk CC=clang-cl

## Pubnub OpenSSL on Universal Windoows Platform (UWP)

OpenSSL doesn't cover threads, so there is some Windows-specific code. 

But, the biggest issue is that OpenSSL is not a part of any Windows installation
`per se`. Actually, a typical Windows computer has dozens of OpenSSL libraries
used by various programs. We provide one solution for this in our sample
Makefile (`uwp.mk`) as described below.

You should decide what OpenSSL you want to compile & link against. To do
that, pass the path of your OpenSSL of choice as macro `OPENSSLPATH` to
`nmake`. We have found that OpenSSL 
(could be found here https://github.com/openssl/openssl
at the time of this writing) works fine. To support UWP, you need to compile based on the instructions available at https://github.com/openssl/openssl/blob/master/NOTES-Windows.txt (check Special notes for Universal Windows Platform builds, aka VC-*-UWP). 
We used the following commands to build OpenSSL library
1. vcvarsall.bat x86 onecore 10.0.19041.0
2. nmake clean
3. perl Configure VC-WIN32-ONECORE --prefix=C:\OpenSSL-Win32 --openssldir=C:\SSL no-shared enable-capieng
4. nmake test
5. nmake install_sw

For example, the default install directory of OpenSSL for 32-bit OpenSSL for Windows is 
`c:\OpenSSL-Win32`, so to use that, you would:

	nmake -f uwp.mk OPENSSLPATH=c:\OpenSSL-Win32

If you don't pass `OPENSSLPATH`, it will use a default, which might not be
good for you, so please check the default in `uwp.mk` before invoking 
nmake without setting `OPENSSLPATH`.

We expect that the `OPENSSLPATH` has two sub-directories:

- `include`: with the OpenSSL include files
- `lib`: with OpenSSL (import) library files

In general, this should work with both import library files (for DLLs) and
static libraries, but that depends on your OpenSSL of choice. The important
thing here is that if you use import libraries, you will have to distribute
the DLLs that are a part of your OpenSSL with your applications.
	
The sample Makefile (`uwp.mk`) will build static
libraries (they will have `.lib` extension on Windows) so that they can be 
used in building UWP applications. SSPI and Proxy is NOT supported at this time.

The sample Makefile is designed to work with MS Visual Studio compilers
(MSVS 2008 or newer should work fine). Rewriting it to use some other
compiler should not be too hard, mostly would involve changing the compiler
switches. But, if you have a compiler that supports MSVC switches
(like some Clang "distributions"), you should be able to use it instead
of the MSVC standard  `cl`, like:

	nmake -f uwp.mk CC=clang-cl

## Notes

We have not tested Pubnub OpenSSL on OSX yet.

## Makefile remarks

The Makefiles are designed to be minimal and illustrate what modules
you need and what `#define`s and compiler/link options, to build
C-core. It is _not_ designed to be just copy-pasted into a production
Makefile, because that is pretty much impossible - there are way too
many ways to "make a Makefile", we can't cover them all.

## Footprint

Since C-core is designed to have low footprint, the question of
"how big is it" often comes up.

We present some numbers here, but, please keep in mind that these
are volatile, because they depend on:

1. C-core itself, which changes with time
2. your usage of the C-core
3. your compiler and its version
4. the C standard library you use and its version
5. the compiler options you pass to compiler (foremost, optimization, but many others too)

So, the number can only give you some idea, and are _not_ to be taken
for granted.

### Linux 64-bit, taken on 2017-05-03

These are the numbers gotten with the `posix.mk` makefile, with a GCC
4.8.4 on Linux Mint 17 (based on Ubuntu 14.04) - using it's "native"
GlibC and OpenSSL.

#### OOB - with proxy support

File | size [KB]
-----|-------------------
`pubnub_callback.a` | 1438
`pubnub_sync.a` | 1288
`pubnub_callback_sample` executable | 348
_stripped_ `pubnub_callback_sample` | 88
`pubnub_sync_sample` executable | 381
_stripped_ `pubnub_sync_sample` | 80

#### Without proxy support

This needed slight changes in the makefile (define `PUBNUB_PROXY_API` to `0`
and remove the proxy modules from the libraries):

File | size [KB]
-----|-------------------
`pubnub_callback.a` | 1181
`pubnub_sync.a` | 1034
`pubnub_callback_sample` executable | 339
_stripped_ `pubnub_callback_sample` | 71
`pubnub_sync_sample` executable | 302
_stripped_ `pubnub_sync_sample` | 62
