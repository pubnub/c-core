# Pubnub C-core for the OpenSSL library/platform

This is the part of C-core for the OpenSSL library/platform.
OpenSSL is portable across POSIX and Windows, so the
same code is used on all and thus we deem it "library/platform".

It has the (OpenSSL) library/platform-specific files and a
sample Makefile (`posix.mk`), which will build
two sample programs on POSIX: `pubnub_sync_sample` and 
`pubnub_callback_sample`. The former uses the
"sync" interface (API) and the latter uses the
"callback" interface (API).

The sources of the samples are in `../core/samples`,
as they are portable across all or most hosted platforms
(POSIX, Windows...).

So, to build the samples, just run:

	make -f posix.mk
	
There are no special requirements for C-core on
POSIX, it should just build on any regular POSIX
system with a GCC compiler (probably with any
other too, but that would require some changes to the
Makefile).

We shall add sample Makefile for Windows soon.