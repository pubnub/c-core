# Pubnub C-core for the MBedTLS library

This is the part of C-core for the [MBedTLS](https://github.com/Mbed-TLS/mbedtls) library. 

MBedTLS support for C-core library is designed to be used with ESP32 IDF environment
but it is not restricted to be used only in such a case. You use this support whenever you 
want but the difference is that it is compilable out of the box only using CMake path
of building the library. 

## How to compile 

To compile this part of the SDK you need to turn on the `MBEDTLS` flag as a part 
of the CMake configuration.

You can do it via CMake GUI, [CMakeLists.txt](../CMakeLists.txt) file or directly using CLI:

```sh 
cmake . -DMBEDTLS=ON 
```

:warning: You cannot compile the C-core library to use MBedTLS and OpenSSL at the same time! :warning:

## ESP32 IDF user 

If you use [IDF Frontend](https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-guides/tools/idf-py.html)
to compile your project for ESP32 MCU than you can setup to build the MBedTLS support with following command:

```sh 
idf.py build -DMBEDTLS=ON 
``` 

or you can simply edit your `CMakeLists` file to add this option during compilation of the component.

