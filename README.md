## Pubnub C/C++ client libraries

[![Build Status](https://travis-ci.com/pubnub/c-core.svg?branch=master)](https://travis-ci.com/pubnub/c-core)

This repository has the source for the C/C++ client libraries,
designed mostly for embedded systems, but perfectly suitable for
"regular" (POSIX, Windows) systems, too.

### Official Docs 
Docs for C/C++ for Posix, Windows, as well as FreeRTOS and other embedded platforms are available at 
https://www.pubnub.com/docs/posix-c/pubnub-c-sdk

## Please direct all Support Questions and Concerns to Support@PubNub.com

## Build 

Our repository can be build via two different approaches:

### CMake (new but recommended)

You can build repository using CMake using our [CMakeLists.txt file](CMakeLists.txt). 

```sh
mkdir build && cd build
cmake ..
[n]make
```

There are several options to be selected via switches (using GUI) or by using command line.
```sh 
cmake .. -DOPENSSL=ON -DEXAMPLES=ON <etc...>
```

Check the [CMakeLists file](CMakeLists.txt) for more info.

### Makefiles (legacy)

You can build our SDK using standard tooling depending on OS you're using. 
For more info check those links:

- [posix](posix/README.md),
- [windows](windows/README.md),
- [openssl](openssl/),

depending on what fits you best. 

## Directory Organization

The directories of the library repository are:

- `core` : The core part, with modules portable to all, or most, libraries for
  specific platforms

- `lib` : Modules for libraries that are available on more than one platform

- `windows` : Modules and Makefile for the Windows platform

- `posix` : Modules and Makefile for POSIX OSes (tested mostly on Linux)

- `openssl`: Modules and Makefile(s) for OpenSSL (on POSIX and Windows)

- `cpp`: Modules, Makefile(s) and examples for the C++ wrapper

- `qt`: Modules, Qt projects and examples for Qt

- `freertos` : Modules and Makefile for the FreeRTOS

- `microchip_harmony` : Modules and project for Microchip MPLAB Harmony

- `unreal` : Unreal Engine integration module

  
## Files

In this root directory we have some files, too:

- `posix.mk`: a "master" Makefile for POSIX - will build all the POSIX
  Makefiles there are (doesn't build Qt - you might not have Qt)
- `windows.mk`: a "master" Makefile for Windows - will build all the 
  Windows Makefiles there are (doesn't build Qt, you might not have Qt). For UWP support, use uwp.mk Makefile.
- `.pubnub.yml`: Standard Pubnub library description in YAML
- `.travis.yml`: Configuration for Travis CI (POSIX: Linux and MacOS)
- `.appveyor.yml`: Configuration for Appveyor CI (Windows)
- `.clang-format`: Configuration for Clang-format tool that we use to format
  C/C++ source code

  
## Contributing

Please read the [Contribution Guidelines](CONTRIBUTING.md).
