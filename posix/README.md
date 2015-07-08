# Pubnub C-core for the POSIX platform

This is the part of C-core for the POSIX platform.
It has the (POSIX) platform-specific files and a
sample Makefile (`posix.mk`), which will build
two sample programs: `pubnub_sync_sample` and 
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