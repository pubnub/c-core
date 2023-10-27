# PubNub C-core for Qt

The PubNub Qt SDK is a part of the PubNub C-core SDK. Unlike the C++ wrapper, which wraps a full-featured C-core platform, this is a Qt-native C-core of sorts. It provides its own Qt API, using as many C-core modules as possible.

## Supported Qt versions

This SDK works with Qt5 and Qt6. It builds on Qt4, too, but there are some runtime issues, e.g.: publishing a JSON object fails.

If you have many Qt versions on your machine, use Qt5 or Qt6 (while Qt6 is Unix-tested only). To select the correct version, you can use the `qtchooser` app. For example, run `qtchooser -run-tool=qmake -qt=5 pubnub.pro`. This creates a `Makefile` that uses Qt5 tools.

Of course, you may also use the provided Qt projects in Qt Creator.

## Sample projects

We provide the following sample projects for your convenience:

- `pubnub.pro`, which builds a Qt command line application that executes most of the Pubnub transactions/operations. It's similar to `pubnub_sync_sample` or `pubnub_callback_sample` from the C-core SDK.
- `pubnub_gui.pro`, which builds a Qt GUI application, a minimalistic Pubnub console.

To build the projects:

1. Run `qmake pubnub.pro` or `qmakepubnub_gui.pro`
2. Run `make` or, if you use MSVC on Windows, `nmake` or `jom`.

## Additional defines

With PubNub Qt SDK, we provided some additional defines that change the behavior of the SDK:

|Flag|Required|Description|
|---|---|---|
|`PUBNUB_QT`|Required|Enables C-core code to fit Qt requirements.|
|`PUBNUB_QT_MOVE_TO_THREAD`|Pptional, enabled by default|Uses the [`moveToThread()`](https://doc.qt.io/qt-6/qobject.html#moveToThread) function to organize SDK timers.|
