PROJECT_SOURCEFILES = pubnub_pubsubapi.c pubnub_coreapi.c pubnub_ccore_pubsub.c pubnub_ccore.c pubnub_url_encode.c pubnub_netcore.c pubnub_alloc_static.c pubnub_assert_std.c pubnub_json_parse.c pubnub_keep_alive.c ..\core\pubnub_helper.c ..\core\c99\snprintf.c ..\core\pbcc_advanced_history.c ..\core\pubnub_advanced_history.c ..\lib\pb_strnlen_s.c ..\lib\pb_strncasecmp.c

all: pubnub_proxy_NTLM_test.exe

CFLAGS =-D PUBNUB_ADVANCED_KEEP_ALIVE=1 -D PUBNUB_LOG_LEVEL=PUBNUB_LOG_LEVEL_NONE -D PUBNUB_RECEIVE_GZIP_RESPONSE=0 -D PUBNUB_DYNAMIC_REPLY_BUFFER=1 -D HAVE_STRERROR_S -I. -I..\ -I ..\core\c99 -I test -I..\lib\base64 -I..\lib\md5

LDLIBS=ws2_32.lib rpcrt4.lib secur32.lib

PROXY_PROJECT_SOURCEFILES = pubnub_proxy_core.c pubnub_proxy.c pbhttp_digest.c pbntlm_core.c pbntlm_packer_sspi.c pubnub_generate_uuid_v4_random_std.c ..\lib\base64\pbbase64.c ..\lib\md5\md5.c

pubnub_proxy_NTLM_test.exe: pubnub_proxy_NTLM_test.c $(PROJECT_SOURCEFILES) $(PROXY_PROJECT_SOURCEFILES)
	$(CC) $(CFLAGS) -D PUBNUB_PROXY_API=1 -D PUBNUB_USE_WIN_SSPI=1 -D PUBNUB_USE_SSL=0 -D PUBNUB_CRYPTO_API=0 pubnub_proxy_NTLM_test.c $(PROJECT_SOURCEFILES) $(PROXY_PROJECT_SOURCEFILES) $(LDLIBS)
#	"$(CC)" -o $@ -D PUBNUB_PROXY_API=1 $(CFLAGS) -Wall -fprofile-arcs -ftest-coverage $(PROJECT_SOURCEFILES) $(PROXY_PROJECT_SOURCEFILES) pubnub_proxy_NTLM_test.c $(LDLIBS)

clean:
	del *.exe
	del *.obj
	del *.pdb
	del *.il?
	del *.lib
	del *.c~
	del *.h~
	del *~
