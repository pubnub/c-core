# Pubnub C-core for the Microchip MPLAB Harmony

This is the part of C-core for the Microchip MPLAB Harmony.  It is tested
with Harmony v1.07. It has the  platform-specific files and a sample MPLAB X 
project which will build  firmware for several target platforms. This sample 
is a "walk-through" of the "callback" interface (API).

The "callback" interface is the only available interface for Harmony.
Some future versions may provide a "sync" interface on the condition
of using a RTOS, or some additional "Harmony specific" interface.

Harmony proper, without additional RTOS-es, is "poll-driven", so,
you need to call the Pubnub "poll" function from your main loop -
this functions is usually called `Task`(s) in Harmony module,
and in Pubnub library it is called `pubnub_task()`. Also, you need to
initialize the TCP/IP stack, Pubnub library doesn't do that, as it
varies significantly between various devices. This is all illustrated
in the sample project, see file `app.c`.

For the sample to work, you need to put the C-core directory tree
into the Harmony directory structure, as illustrated below:

    harmony-+
            +-v1_07-+
                    +-apps-+
                    |      +-c-core-+-core
                    |      |        +-microchip_harmony
                    |      |        ...
                    |      +-examples
                    |      ...
                    +-framework
                    ...

To use the SSL (HTTPS) support, you need to define the preprocessor
macro `PUBNUB_USE_SSL` to 1 in your project. If it is not defined, 
Pubnub library will use TCP-IP (HTTP) only. Which SSL/TLS provider
you use is configured via Harmony configurator, Pubnub doesn't
know (or care) which is it - the sample project uses WolfSSL that
is shipped with Harmony.

To use the Pubnub library in your project, `#include "pubnub_callback.h`
and include all the needed modules - for a complete list, it's best
to see (and clone) the sample project.


## Remarks

Support for running multi-threaded code (w/RTOS) is planned, but
we don't have a ship-date yet. Ditto for integration of Pubnub
library into MHC.

Currently, Pubnub library only supports IPv4. Next version will
support IPv6, too.
