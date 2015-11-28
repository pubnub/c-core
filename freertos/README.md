# Pubnub C-core for the FreeRTOS+TCP

This is the part of C-core for the FreeRTOS+TCP.  It has the (FreeRTOS+TCP)
platform-specific files and a sample VisualStudio 2010 solution (the file
`Pubnub.sln`) which will build following programs to be run under the 
FreeRTOS(+TCP) Windows simulator:

- `pubnub_sync_sample`: a "walk-through" of the "sync" interface (API)
- `pubnub_callback_sample`: a "walk-through" of the "callback" interface

So, to build the samples load the solution in the VisualStudio 2010
and build, or use `msbuild` from VisualStudio command prompt.

Also, you need to put c-core source code tree and FreeRTOS+TCP code
tree in the right place in order for this to work. The tree should
look like this (just the main directories shown):

    +- c-core ----------+- core
                        +- lib ------ sockets
    |                   +- freertos
    |
    +- FreeRTOS_Labs ---+- FreeRTOS
                        +- FreeRTOS-Plus

Also, we don't provide source code of the FreeRTOS+TCP. At the time of 
this writing, the FreeRTOS+TCP source code could be downloaded from: 
http://www.freertos.org/FreeRTOS-Labs/RTOS_labs_download.html?0

The ZIP from that URL has files in a directory that is not named
`FreeRTOS_Labs`, but `FreeRTOS_Labs_<version>` - so, something like
`FreeRTOS_Labs_150825`. You can either rename the directory after
extraction, or provide a symbolic link (we suggest the symbolic link).

The samples are derived from the FreeRTOS+TCP "Minimal Demo", so
please look for info about running the sample in the documentation
for the FreeRTOS+TCP demos.

Of course, the source code of the FreeRTOS+TCP port of Pubnub C-core
(including the samples) is portable to any platform that FreeRTOS+TCP 
supports, not just the Windows simulator, but, we don't provide example 
projects/makefiles for others at this time.


## Remarks

Pubnub may provide support for other TCP/IP stacks on FreeRTOS in the 
future

There is yet no support for a SSL/TLS library.
