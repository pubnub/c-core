TEMPLATE = app
QT += widgets network
HEADERS += pubnub_qt.h pubnub_qt_gui_sample.h
SOURCES += pubnub_qt.cpp pubnub_qt_gui_sample.cpp ../core/pubnub_ccore.c ../core/pubnub_assert_std.c ../core/pubnub_json_parse.c ../core/pubnub_helper.c
win32:SOURCES += ../core/c99/snprintf.c

INCLUDEPATH += ../core
win32:INCLUDEPATH += ../core/c99
DEPENDPATH += ../core

# install
target.path = $$[QT_INSTALL_EXAMPLES]/network/pubnub_qt
INSTALLS += target
