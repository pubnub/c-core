# Pubnub C-core for the OpenSSL library/platform

This is the part of C-core for the OpenSSL library/platform.  OpenSSL
is portable across POSIX and Windows, so the same code is used on all
and thus we deem it "library/platform".

It has the (OpenSSL) library/platform-specific files and a sample
Makefile (`posix.mk`), which will build sample programs on POSIX:

- `pubnub_sync_sample`: a "walk-through" of the "sync" interface (API)
- `pubnub_callback_sample`: a "walk-through" of the "callback"
	interface (API)
- `cancel_subscribe_sync_sample`: an example how to cancel a subscribe
  loop safely, using the "sync" interface
- `subscribe_publish_callback_sample`: an example of how to have one
  outstanding publish and one outstanding subscribe transaction/operation
  at the same time, using the "callback" interface.


The sources of the samples are in `../core/samples`, as they are
portable across all or most hosted platforms (POSIX, Windows...).

So, to build the samples, just run:

	make -f posix.mk
	
The only requirement / prerequisite of this Makefile is that you have
an OpenSSL package installed, which should be true for most modern
POSIX systems. In that's met, it should just build with the default C
compiler. To use a compiler of your choice (rather than the default
one), say, `clang`, run:

	make -f posix.mk CC=clang

# Notes

- We shall add sample Makefile for Windows soon.
