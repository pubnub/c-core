TEMPLATE = app
QT = core network
CONFIG += C++11
mac:CONFIG -= app_bundle
win32:CONFIG += console
HEADERS += pubnub_qt.h pubnub_qt_sample.h
SOURCES += pubnub_qt.cpp pubnub_qt_sample.cpp ../core/pubnub_ccore.c ../core/pubnub_ccore_pubsub.c ../core/pbcc_subscribe_v2.c ../core/pbcc_advanced_history.c ../core/pbcc_objects_api.c ../core/pbcc_actions_api.c ../core/pubnub_url_encode.c ../core/pubnub_assert_std.c ../core/pubnub_json_parse.c ../core/pubnub_helper.c ../lib/pbcrc32.c ../lib/pb_strnlen_s.c
win32:SOURCES += ../core/c99/snprintf.c

INCLUDEPATH += ..
win32:INCLUDEPATH += ../core/c99
DEPENDPATH += ../core

# install
target.path = $$[QT_INSTALL_EXAMPLES]/network/pubnub_qt
INSTALLS += target
