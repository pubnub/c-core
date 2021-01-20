# Pubnub C-core for the Windows platform

This is the part of C-core for the Windows platform.
It has the (Windows) platform-specific files and a
sample Makefile (`windows.mk`), which will build
sample programs:

- `pubnub_sync_sample.exe`: a "walk-through" of the "sync" interface (API)
- `pubnub_callback_sample.exe`: a "walk-through" of the "callback"
	interface (API)
- `cancel_subscribe_sync_sample.exe`: an example how to cancel a subscribe
  loop safely, using the "sync" interface
- `subscribe_publish_callback_sample.exe`: an example of how to have one
  outstanding publish and one outstanding subscribe transaction/operation
  at the same time, using the "callback" interface.

During the build, two (static) libraries will be created:

- `pubnub_sync.lib`: library for the "sync" interface
- `pubnub_callback.lib`: library for the "callback" interface

You are free to use these libraries in your projects, but keep in mind
that they are configured for the purposes of the samples. Please check
if those are right for you before using them.

The sources of the samples are in `../core/samples`,
as they are portable across all or most hosted platforms
(POSIX, Windows...).

So, to build the samples, just run:

	nmake windows.mk

from a Visual Studio Command Prompt. This was tested mainly
on MSVS 2010 and Windows 7 & 10, but should work with pretty
much any MSVS since 2008 and Windows since XP.

There are no special requirements for C-core on
Windows, except for a working compiler and Windows SDK - and
having Windows XP or newer.

If you have Clang for Windows installed, this should work:

    nmake -f windows.mk CC=clang-cl

# Pubnub C-core for the Universal Windows platform(UWP)

This is the part of C-core for the UWP platform.
It has a sample Makefile (`uwp.mk`), which will build, two (static) libraries will be created:

- `pubnub_sync.lib`: library for the "sync" interface
- `pubnub_callback.lib`: library for the "callback" interface

You are free to use these libraries in your projects, but keep in mind
that they are configured for the purposes of the samples. Please check
if those are right for you before using them.

So, to build the libraries, just run:

	nmake uwp.mk

from a Visual Studio Command Prompt. This was tested mainly
on MSVS 2019 and Windows 10.

If you have Clang for Windows installed, this should work:

    nmake -f uwp.mk CC=clang-cl

## Makefile for GCC

There is a Makefile for a `gcc` compatible compiler - `windows-gcc.mk`.
So, if you have MINGW, this should work:

    mingw32-make -f windows-gcc.mk

Of course, if `mingw32-make.exe` is not on PATH, you may need to
provide the full path to it. In some setups, things may work better if
you:

    sh -c "make -f windows-gcc.mk"

and, again, if you don't have `sh.exe` on PATH and/or `make.exe` on
Windows or MINGW PATH, you may need to provide full path to them.

This should work with Clang (for Windows) also, like:

    mingw32-make CC=clang -f windows-gcc.mk

It should also work with `scan-build`, though it's usually a little
tricky to make everything work. On one machine, the command is a
rather complicated:

    scan-build --use-analyzer="e:/program files/llvm/bin/clang.exe"  e:\msys64\usr\bin\sh.exe -c "e:/msys64/usr/bin/make -f windows-gcc.mk"

Also, keep in mind that `windows-gcc.mk` is less used and, since
GCC/Clang (MINGW, Cygwin) setups can be very different/strange, it
doesn't build all. But, you should have enough to start to make your
own Makefile for you project.


## Makefile remarks

The Makefile is designed to be minimal and illustrate what modules you
need and what `#define`s and compiler/link options, to build
C-core. It is _not_ designed to be just copy-pasted into a production
Makefile, because that is pretty much impossible - there are way too
many ways to "make a Makefile", we can't cover them all.
