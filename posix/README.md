# Pubnub C-core for the POSIX platform

This is the part of C-core for the POSIX platform.  It has the (POSIX)
platform-specific files and a sample Makefile (`posix.mk`), which will
build sample programs:

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

So, to build the samples, just execute:

	make -f posix.mk
	
There are no special requirements for C-core on POSIX, it should just
build on any regular POSIX system with a C compiler. To use a compiler
of your choice (rather than the default one), say, `clang`, execute:

	make -f posix.mk CC=clang


## Build without the proxy support

To build without the proxy support, pass `USE_PROXY=0` to Make, like:

    make -f posix.mk USE_PROXY=0

Of course, if you have previously done a build, you will probably want
to first clean up the artifacts of that previous build by invoking the
`clean` target, like:

	make -f posix.mk clean


## OSX / Darwin remarks

While being a "mostly POSIX" compliant environment, OSX, in its
"Darwin" OS base, doesn't support POSIX standard `clock_gettime()`
API. This is well-known and often complained about, and the actual
code to implement it is rather simple, so it is strange that Apple
hasn't fixed this yet.

Anyway, to be able to work on OSX / Darwin, we have a small module
that abstracts "getting a monotonic clock time", with an
implementation for POSIX and another for Darwin. You have to link the
right one, and we do that in `posix.mk`.


## Makefile remarks

The Makefile is designed to be minimal and illustrate what modules you
need and what `#define`s and compiler/link options, to build
C-core. It is _not_ designed to be just copy-pasted into your
production Makefile, because there are way too many ways to "make a
Makefile", we can't cover them all.

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

These are the numbers gotten with the `posix.mk` makefile, with a GCC 4.8.4
on Linux Mint 17 (based on Ubuntu 14.04).

#### OOB - with proxy support

File | size [KB]
-----|-------------------
`pubnub_callback.a` | 496
`pubnub_sync.a` | 428
`pubnub_callback_sample` executable | 188
_stripped_ `pubnub_callback_sample` | 84
`pubnub_sync_sample` executable | 168
_stripped_ `pubnub_sync_sample` | 68

#### Without proxy support

This needed slight changes in the makefile (define `PUBNUB_PROXY_API` to `0`
and remove the proxy modules from the libraries):


File | size [KB]
-----|-------------------
`pubnub_callback.a` | 393
`pubnub_sync.a` | 331
`pubnub_callback_sample` executable | 139
_stripped_ `pubnub_callback_sample` | 63
`pubnub_sync_sample` executable | 120
_stripped_ `pubnub_sync_sample` | 55
