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

There is yet no support for a SSL/TLS library (excluding ESP32 platform).

## ESP32 support

You can build the FreeRTOS+TCP Pubnub C-core for the ESP32 platform 
using the ESP-IDF build system. The ESP-IDF build system is based on 
CMake and we provide a CMakeLists.txt file for building the Pubnub 
C-core for the ESP32 platform as idf_component.

Additionally, we support SSL/TLS on the ESP32 platform using the 
mbedTLS library. 
All you have to do is to set the `MBEDTLS` option to `ON` 
when configuring the build system.

To use the Pubnub C-core in your ESP-IDF project you can select the 
one of the following options:

### ESP-IDF registry

> :warning: **Note**: This option is not available yet. :warning:

### clone the repository

You can clone the repository and add it as a component to your project.
All you have to do is to ensure that your project is capable of building 
external components. You can find more information about this in the 
[ESP-IDF documentation](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-guides/build-system.html#external-components).

```sh 
# in root directory of your project
git clone https://github.com/pubnub/c-core.git components/c-core
```

Then calling the `idf.py build` command should look into the components
directory, find the Pubnub C-core and build it as a part of your project.
