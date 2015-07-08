# Pubnub C-core for the POSIX platform

This is the part of C-core for the Windows platform.
It has the (Windows) platform-specific files and a
sample Makefile (`windows.mk`), which will build
two sample programs: `pubnub_sync_sample.exe` and 
`pubnub_callback_sample.exe`. The former uses the
"sync" interface (API) and the latter uses the
"callback" interface (API).

The sources of the samples are in `../core/samples`,
as they are portable across all or most hosted platforms
(POSIX, Windows...).

So, to build the samples, just run:

	nmake posix.mk
	
from a Visual Studio Command Prompt. This was tested mainly
on MSVS 2010 and Windows 7 & 8, but should work with pretty
much any MSVS since 2005 and Windows since XP.

There are no special requirements for C-core on
Windows, except for a working compiler and Windows SDK - and
having Windows XP or newer.

It could probably be made to build & work with other compilers
(like Cygwin, MINGW, etc), with small changes to the Makefile.