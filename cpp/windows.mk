SOURCEFILES = ..\core\pubnub_coreapi.c ..\core\pubnub_ccore.c ..\core\pubnub_netcore.c  ..\lib\sockets\pbpal_sockets.c ..\lib\sockets\pbpal_resolv_and_connect_sockets.c ..\core\pubnub_alloc_std.c ..\core\pubnub_assert_std.c ..\core\pubnub_generate_uuid.c ..\core\pubnub_blocking_io.c ..\core\pubnub_json_parse.c ..\core\pubnub_helper.c  ..\windows\pubnub_version_windows.c ..\windows\pubnub_generate_uuid_windows.c ..\windows\pbpal_windows_blocking_io.c ..\core\c99\snprintf.c

CFLAGS = /EHsc /Zi /TP /I ..\core /I . /I ..\core\c99 /I ..\windows /W3
# /Zi enables debugging, remove to get a smaller .exe and no .pdb 
# /TP means "compile all files as C++"
# /EHsc enables (standard) exception support

all: pubnub_sync_sample.exe pubnub_callback_sample.exe pubnub_callback_cpp11_sample.exe cancel_subscribe_sync_sample.exe subscribe_publish_callback_sample.exe futres_nesting_sync.exe futres_nesting_callback.exe futres_nesting_callback_cpp11.exe


pubnub_sync_sample.exe: samples\pubnub_sample.cpp $(SOURCEFILES) ..\core\pubnub_ntf_sync.c pubnub_futres_sync.cpp
	$(CXX) /Fe$@ $(CFLAGS) samples\pubnub_sample.cpp ..\core\pubnub_ntf_sync.c pubnub_futres_sync.cpp $(SOURCEFILES) /link ws2_32.lib rpcrt4.lib user32.lib
#-D VERBOSE_DEBUG

cancel_subscribe_sync_sample.exe: samples\cancel_subscribe_sync_sample.cpp $(SOURCEFILES) ..\core\pubnub_ntf_sync.c pubnub_futres_sync.cpp
	$(CXX) /Fe$@ $(CFLAGS) samples\cancel_subscribe_sync_sample.cpp ..\core\pubnub_ntf_sync.c pubnub_futres_sync.cpp $(SOURCEFILES) /link ws2_32.lib rpcrt4.lib user32.lib

futres_nesting_sync.exe: samples\futres_nesting.cpp $(SOURCEFILES) ..\core\pubnub_ntf_sync.c pubnub_futres_sync.cpp
	$(CXX) /Fe$@ $(CFLAGS) samples\futres_nesting.cpp ..\core\pubnub_ntf_sync.c pubnub_futres_sync.cpp $(SOURCEFILES) /link ws2_32.lib rpcrt4.lib user32.lib
#-D VERBOSE_DEBUG

pubnub_callback_sample.exe: samples\pubnub_sample.cpp $(SOURCEFILES) ..\windows\pubnub_ntf_callback_windows.c pubnub_futres_windows.cpp
	$(CXX) /Fe$@ -D PUBNUB_CALLBACK_API $(CFLAGS) -D VERBOSE_DEBUG samples\pubnub_sample.cpp ..\windows\pubnub_ntf_callback_windows.c pubnub_futres_windows.cpp $(SOURCEFILES) /link ws2_32.lib rpcrt4.lib user32.lib

pubnub_callback_cpp11_sample.exe: samples\pubnub_sample.cpp $(SOURCEFILES) ..\windows\pubnub_ntf_callback_windows.c pubnub_futres_cpp11.cpp
	$(CXX) /Fe$@ /D PUBNUB_CALLBACK_API $(CFLAGS) /D VERBOSE_DEBUG samples\pubnub_sample.cpp ..\windows\pubnub_ntf_callback_windows.c pubnub_futres_cpp11.cpp $(SOURCEFILES) /link ws2_32.lib rpcrt4.lib user32.lib

subscribe_publish_callback_sample.exe: samples\subscribe_publish_callback_sample.cpp $(SOURCEFILES) ..\windows\pubnub_ntf_callback_windows.c pubnub_futres_windows.cpp
	$(CXX) /Fe$@ -D PUBNUB_CALLBACK_API $(CFLAGS) -D VERBOSE_DEBUG samples\subscribe_publish_callback_sample.cpp ..\windows\pubnub_ntf_callback_windows.c pubnub_futres_windows.cpp $(SOURCEFILES) /link ws2_32.lib rpcrt4.lib user32.lib

futres_nesting_callback.exe: samples\futres_nesting.cpp $(SOURCEFILES) ..\windows\pubnub_ntf_callback_windows.c pubnub_futres_windows.cpp
	$(CXX) /Fe$@ -D PUBNUB_CALLBACK_API $(CFLAGS)  samples\futres_nesting.cpp ..\windows\pubnub_ntf_callback_windows.c pubnub_futres_windows.cpp $(SOURCEFILES) /link ws2_32.lib rpcrt4.lib user32.lib
#-D VERBOSE_DEBUG

futres_nesting_callback_cpp11.exe: samples\futres_nesting.cpp $(SOURCEFILES) ..\windows\pubnub_ntf_callback_windows.c pubnub_futres_cpp11.cpp
	$(CXX) /Fe$@ -D PUBNUB_CALLBACK_API $(CFLAGS)  samples\futres_nesting.cpp ..\windows\pubnub_ntf_callback_windows.c pubnub_futres_cpp11.cpp $(SOURCEFILES) /link ws2_32.lib rpcrt4.lib  user32.lib
#-D VERBOSE_DEBUG


clean:
	del *.obj
	del *.exe
	del *.pdb
	del *.il?
	