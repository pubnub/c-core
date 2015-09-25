# Pubnub C-core for Qt

This is a part of C-core for the Qt. It is tested only with
Qt5, though it may work with Qt4 without much (if any) changes.

Unlike the C++ wrapper, which wraps a "full featured" C-core
platform, this is a "Qt-native C-core" of sorts. It provides
it's own (Qt) API, while using as much of C-core modules as
posible.

There is a sample project, `pubnub.pro`, which will build a
Qt command line application which executes most of the Pubnub
transactions/operations - similar to the `pubnub_sync_sample`
or `pubnub_callback_sample` from C-core.

To build the sample, run `qmake pubnub.pro`, and then `make`
or, on Windows if using MSVC, `nmake` or `jom`.

If you have both Qt5 and Qt4 on your machine, be sure to use
the Qt5 tools, for which purpose you may utilize the `qtchooser`
app. For example, do:

	qtchooser -run-tool=qmake -qt=5

That will create the `Makefile` that will use Qt5 tools.

Of course, you may also use the provided Qt project in Qt Creator.