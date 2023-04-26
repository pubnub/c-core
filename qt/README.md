# Pubnub C-core for Qt

This is a part of C-core for the Qt. It is tested to work only with
Qt5 and Qt6. It builds on Qt4, too, but there are some run-time issues, i.e.
publishing some JSON object fails.

Unlike the C++ wrapper, which wraps a "full featured" C-core
platform, this is a "Qt-native C-core" of sorts. It provides
it's own (Qt) API, while using as much of C-core modules as
posible.

There are sample projects:

- `pubnub.pro`, which will build a Qt command line application which
executes most of the Pubnub transactions/operations - similar to the
`pubnub_sync_sample` or `pubnub_callback_sample` from C-core.
- `pubnub_gui.pro`, which will build a Qt GUI application, a
"minimalistic" Pubnub console

To build the samples, run `qmake pubnub.pro` or `qmake
pubnub_gui.pro`, and then `make` or, on Windows if using MSVC, `nmake`
or `jom`.

If you have many Qt versions on your machine, be sure to use
the Qt5 or Qt6 (whereas QT6 is unix tested only) tools, for which purpose you may utilize the `qtchooser`
app. For example, do:

	qtchooser -run-tool=qmake -qt=5 pubnub.pro

That will create the `Makefile` that will use Qt5 tools.

Of course, you may also use the provided Qt projects in Qt Creator.
