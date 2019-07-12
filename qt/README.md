# Pubnub C-core for Qt

This SDK is a part of C-core for the [Qt framework](https://doc.qt.io).
It is tested to work only with Qt 5.
The SDK will build on Qt 4, but there are some runtime issues; for example, publishing some JSON objects will fail.

Unlike the C++ SDK, which wraps a "full featured" C-core platform, this is a "Qt-native C-core".
It provides its own (Qt) API, while incorporating as much of the C-core modules as possible.

## Documentation

To use [Doxygen](http://www.doxygen.nl) to generate documentation, run the following command in the `c-core/qt` directory:

```bash
 doxygen pubnub.doxygen
```

The Doxygen output will be created in the `c-core/qt/doc/html` directory.

## Sample projects

There are two sample projects:

- `pubnub.pro` builds a Qt command-line application which executes most of the Pubnub transactions/operations, similar to the `pubnub_sync_sample` or `pubnub_callback_sample` from C-core
- `pubnub_gui.pro` builds a Qt GUI application that provides a "minimalistic" Pubnub console

To build the samples, run `qmake pubnub.pro` or `qmake pubnub_gui.pro`, and then `make` or, on Windows if using MSVC, `nmake` or `jom`.

If you have both Qt 5 and Qt 4 on your machine, use `qtchooser` to select the Qt 5 tools. For example, the following command creates a `Makefile` that will use the Qt 5 tools:

```bash
qtchooser -run-tool=qmake -qt=5 pubnub.pro
```

Of course, you may also use the provided Qt projects in Qt Creator.
