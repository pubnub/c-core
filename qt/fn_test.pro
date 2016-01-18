TEMPLATE = app
QT = core network
mac:CONFIG -= app_bundle
win32:CONFIG += console
CONFIG += c++11
HEADERS += pubnub_qt.h pubnub.hpp
SOURCES += pubnub_qt.cpp pubnub.cpp fntest/pubnub_fntest_runner.cpp ../cpp/fntest/pubnub_fntest.cpp ../cpp/fntest/pubnub_fntest_basic.cpp ../cpp/fntest/pubnub_fntest_medium.cpp ../core/pubnub_ccore.c ../core/pubnub_assert_std.c ../core/pubnub_json_parse.c ../core/pubnub_helper.c
win32:SOURCES += ../core/c99/snprintf.c

INCLUDEPATH += ../core ../cpp/fntest
win32:INCLUDEPATH += ../core/c99
DEPENDPATH += ../core

# install
target.path = $$[QT_INSTALL_EXAMPLES]/network/pubnub_qt
INSTALLS += target
