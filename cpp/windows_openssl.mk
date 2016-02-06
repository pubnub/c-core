SOURCEFILES = ..\core\pubnub_coreapi.c ..\core\pubnub_ccore.c ..\core\pubnub_netcore.c  ..\openssl\pbpal_openssl.c ..\openssl\pbpal_resolv_and_connect_openssl.c ..\core\pubnub_alloc_std.c ..\core\pubnub_assert_std.c ..\core\pubnub_generate_uuid.c ..\core\pubnub_timers.c ..\core\pubnub_blocking_io.c ..\core\pubnub_json_parse.c ..\core\pubnub_helper.c  ..\openssl\pubnub_version_openssl.c ..\windows\pubnub_generate_uuid_windows.c ..\openssl\pbpal_openssl_blocking_io.c ..\core\c99\snprintf.c

!ifndef OPENSSLPATH
OPENSSLPATH=c:\OpenSSL-Win32
!endif

LIBS=ws2_32.lib rpcrt4.lib $(OPENSSLPATH)\lib\ssleay32.lib $(OPENSSLPATH)\lib\libeay32.lib

CFLAGS = /EHsc /Zi /MP /TP /I ..\core /I . /I ..\core\c99 /I ..\openssl /I $(OPENSSLPATH)\include /W3 /D PUBNUB_THREADSAFE
# /Zi enables debugging, remove to get a smaller .exe and no .pdb 
# /MP uses one compiler (`cl`) process for each input file, enabling faster build
# /TP means "compile all files as C++"
# /EHsc enables (standard) exception support

all: openssl\pubnub_sync_sample.exe openssl\pubnub_callback_sample.exe openssl\pubnub_callback_cpp11_sample.exe openssl\cancel_subscribe_sync_sample.exe openssl\subscribe_publish_callback_sample.exe openssl\futres_nesting_sync.exe openssl\futres_nesting_callback.exe openssl\futres_nesting_callback_cpp11.exe


openssl\pubnub_sync_sample.exe: samples\pubnub_sample.cpp $(SOURCEFILES) ..\core\pubnub_ntf_sync.c pubnub_futres_sync.cpp
	$(CXX) /Fe$@ $(CFLAGS) samples\pubnub_sample.cpp ..\core\pubnub_ntf_sync.c pubnub_futres_sync.cpp $(SOURCEFILES) /link $(LIBS)

openssl\cancel_subscribe_sync_sample.exe: samples\cancel_subscribe_sync_sample.cpp $(SOURCEFILES) ..\core\pubnub_ntf_sync.c pubnub_futres_sync.cpp
	$(CXX) /Fe$@ $(CFLAGS) samples\cancel_subscribe_sync_sample.cpp ..\core\pubnub_ntf_sync.c pubnub_futres_sync.cpp $(SOURCEFILES) /link $(LIBS)

openssl\futres_nesting_sync.exe: samples\futres_nesting.cpp $(SOURCEFILES) ..\core\pubnub_ntf_sync.c pubnub_futres_sync.cpp
	$(CXX) /Fe$@ $(CFLAGS) samples\futres_nesting.cpp ..\core\pubnub_ntf_sync.c pubnub_futres_sync.cpp $(SOURCEFILES) /link $(LIBS)

openssl\pubnub_callback_sample.exe: samples\pubnub_sample.cpp $(SOURCEFILES) ..\core\pubnub_timer_list.c ..\openssl\pubnub_ntf_callback_windows.c ..\openssl\pubnub_get_native_socket.c pubnub_futres_windows.cpp
	$(CXX) /Fe$@ -D PUBNUB_CALLBACK_API $(CFLAGS) samples\pubnub_sample.cpp ..\core\pubnub_timer_list.c ..\openssl\pubnub_ntf_callback_windows.c ..\openssl\pubnub_get_native_socket.c pubnub_futres_windows.cpp $(SOURCEFILES) /link $(LIBS)

openssl\pubnub_callback_cpp11_sample.exe: samples\pubnub_sample.cpp $(SOURCEFILES) ..\core\pubnub_timer_list.c ..\openssl\pubnub_ntf_callback_windows.c ..\openssl\pubnub_get_native_socket.c pubnub_futres_cpp11.cpp
	$(CXX) /Fe$@ /D PUBNUB_CALLBACK_API $(CFLAGS) samples\pubnub_sample.cpp ..\core\pubnub_timer_list.c ..\openssl\pubnub_ntf_callback_windows.c ..\openssl\pubnub_get_native_socket.c pubnub_futres_cpp11.cpp $(SOURCEFILES) /link $(LIBS)

openssl\subscribe_publish_callback_sample.exe: samples\subscribe_publish_callback_sample.cpp $(SOURCEFILES) ..\core\pubnub_timer_list.c ..\openssl\pubnub_ntf_callback_windows.c ..\openssl\pubnub_get_native_socket.c pubnub_futres_windows.cpp
	$(CXX) /Fe$@ -D PUBNUB_CALLBACK_API $(CFLAGS) samples\subscribe_publish_callback_sample.cpp ..\core\pubnub_timer_list.c ..\openssl\pubnub_ntf_callback_windows.c ..\openssl\pubnub_get_native_socket.c pubnub_futres_windows.cpp $(SOURCEFILES) /link $(LIBS)

openssl\futres_nesting_callback.exe: samples\futres_nesting.cpp $(SOURCEFILES) ..\core\pubnub_timer_list.c ..\openssl\pubnub_ntf_callback_windows.c ..\openssl\pubnub_get_native_socket.c pubnub_futres_windows.cpp
	$(CXX) /Fe$@ -D PUBNUB_CALLBACK_API $(CFLAGS)  samples\futres_nesting.cpp ..\core\pubnub_timer_list.c ..\openssl\pubnub_ntf_callback_windows.c ..\openssl\pubnub_get_native_socket.c pubnub_futres_windows.cpp $(SOURCEFILES) /link $(LIBS)

openssl\futres_nesting_callback_cpp11.exe: samples\futres_nesting.cpp $(SOURCEFILES) ..\core\pubnub_timer_list.c ..\openssl\pubnub_ntf_callback_windows.c ..\openssl\pubnub_get_native_socket.c pubnub_futres_cpp11.cpp
	$(CXX) /Fe$@ -D PUBNUB_CALLBACK_API $(CFLAGS)  samples\futres_nesting.cpp ..\core\pubnub_timer_list.c ..\openssl\pubnub_ntf_callback_windows.c ..\openssl\pubnub_get_native_socket.c pubnub_futres_cpp11.cpp $(SOURCEFILES) /link $(LIBS)


clean:
	del openssl\*.obj
	del openssl\*.exe
	del openssl\*.pdb
	del openssl\*.il?
