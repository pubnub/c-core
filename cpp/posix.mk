SOURCEFILES = ../core/pubnub_coreapi.c ../core/pubnub_ccore.c ../core/pubnub_netcore.c  ../lib/sockets/pbpal_sockets.c ../lib/sockets/pbpal_resolv_and_connect_sockets.c ../core/pubnub_alloc_std.c ../core/pubnub_assert_std.c ../core/pubnub_generate_uuid.c ../core/pubnub_blocking_io.c ../core/pubnub_json_parse.c ../core/pubnub_helper.c  ../posix/pubnub_version_posix.c ../posix/pubnub_generate_uuid_posix.c ../posix/pbpal_posix_blocking_io.c

CFLAGS =-g -I ../core -I ../posix -I . -Wall
# -g enables debugging, remove to get a smaller executable

all: pubnub_sync_sample pubnub_callback_sample pubnub_callback_cpp11_sample cancel_subscribe_sync_sample subscribe_publish_callback_sample futres_nesting_sync futres_nesting_callback futres_nesting_callback_cpp11


pubnub_sync_sample: samples/pubnub_sample.cpp $(SOURCEFILES) ../core/pubnub_ntf_sync.c pubnub_futres_sync.cpp
	$(CXX) -o pubnub_sync_sample $(CFLAGS) samples/pubnub_sample.cpp ../core/pubnub_ntf_sync.c pubnub_futres_sync.cpp $(SOURCEFILES)
#-D VERBOSE_DEBUG

cancel_subscribe_sync_sample: samples/cancel_subscribe_sync_sample.cpp $(SOURCEFILES) ../core/pubnub_ntf_sync.c pubnub_futres_sync.cpp
	$(CXX) -o cancel_subscribe_sync_sample $(CFLAGS) samples/cancel_subscribe_sync_sample.cpp ../core/pubnub_ntf_sync.c pubnub_futres_sync.cpp $(SOURCEFILES)

futres_nesting_sync: samples/futres_nesting.cpp $(SOURCEFILES) ../core/pubnub_ntf_sync.c pubnub_futres_sync.cpp
	$(CXX) -o futres_nesting_sync $(CFLAGS) samples/futres_nesting.cpp ../core/pubnub_ntf_sync.c pubnub_futres_sync.cpp $(SOURCEFILES)
#-D VERBOSE_DEBUG

pubnub_callback_sample: samples/pubnub_sample.cpp $(SOURCEFILES) ../posix/pubnub_ntf_callback_posix.c pubnub_futres_posix.cpp
	$(CXX) -o pubnub_callback_sample -D PUBNUB_CALLBACK_API $(CFLAGS) -D VERBOSE_DEBUG samples/pubnub_sample.cpp ../posix/pubnub_ntf_callback_posix.c pubnub_futres_posix.cpp $(SOURCEFILES) -lpthread

pubnub_callback_cpp11_sample: samples/pubnub_sample.cpp $(SOURCEFILES) ../posix/pubnub_ntf_callback_posix.c pubnub_futres_cpp11.cpp
	$(CXX) -o pubnub_callback_cpp11_sample --std=c++11 -D PUBNUB_CALLBACK_API $(CFLAGS) -D VERBOSE_DEBUG samples/pubnub_sample.cpp ../posix/pubnub_ntf_callback_posix.c pubnub_futres_cpp11.cpp $(SOURCEFILES) -lpthread

subscribe_publish_callback_sample: samples/subscribe_publish_callback_sample.cpp $(SOURCEFILES) ../posix/pubnub_ntf_callback_posix.c pubnub_futres_posix.cpp
	$(CXX) -o subscribe_publish_callback_sample -D PUBNUB_CALLBACK_API $(CFLAGS) -D VERBOSE_DEBUG samples/subscribe_publish_callback_sample.cpp ../posix/pubnub_ntf_callback_posix.c pubnub_futres_posix.cpp $(SOURCEFILES) -lpthread

futres_nesting_callback: samples/futres_nesting.cpp $(SOURCEFILES) ../posix/pubnub_ntf_callback_posix.c pubnub_futres_posix.cpp
	$(CXX) -o futres_nesting_callback -D PUBNUB_CALLBACK_API $(CFLAGS)  samples/futres_nesting.cpp ../posix/pubnub_ntf_callback_posix.c pubnub_futres_posix.cpp $(SOURCEFILES) -lpthread
#-D VERBOSE_DEBUG

futres_nesting_callback_cpp11: samples/futres_nesting.cpp $(SOURCEFILES) ../posix/pubnub_ntf_callback_posix.c pubnub_futres_cpp11.cpp
	$(CXX) -o futres_nesting_callback_cpp11 --std=c++11 -D PUBNUB_CALLBACK_API $(CFLAGS)  samples/futres_nesting.cpp ../posix/pubnub_ntf_callback_posix.c pubnub_futres_cpp11.cpp $(SOURCEFILES) -lpthread
#-D VERBOSE_DEBUG


clean:
	rm pubnub_sync_sample pubnub_callback_sample pubnub_callback_cpp11_sample cancel_subscribe_sync_sample subscribe_publish_callback_sample futres_nesting_sync futres_nesting_callback futres_nesting_callback_cpp11
