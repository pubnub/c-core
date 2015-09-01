# Please direct all Support Questions and Concerns to Support@PubNub.com

## Pubnub C/C++ client libraries

This repository has the source for the C/C++ client libraries,
designed mostly for embedded systems, but perfectly suitable for
"regular" (POSIX, Windows) systems, too.



## Directory Organization

The directories of the library repository are:

- `core` : The core part, with modules portable to all, or most, libraries for
	specific platforms

- `lib` : Modules for libs that are available on more than one platform

- `windows` : Modules and Makefile for the Windows platform

- `posix` : Modules and Makefile for POSIX OSes (tested mostly on Linux)

- `openssl`: Modules and Makefile(s) for OpenSSL (on POSIX and Windows)

- `cpp`: Modules, Makefile(s) and examples for the C++ wrapper

- `contiki` : Modules and Makefile for the Contiki OS

- `freertos` : Modules and Makefile for the FreeRTOS OS

  
## Files

In this root directory we have some files, too:

- `posix.mk`: a "master" Makefile for POSIX - will build all the POSIX
  Makefiles there are
- `windows.mk`: a "master" Makefile for Windows - will build all the 
  Windows Makefiles there are
- `VERSION`: Identifies the latest stable / recommended release 

  
# Please direct all Support Questions and Concerns to Support@PubNub.com
