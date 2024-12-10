# Make all that can be made on POSIX

all:
	make -C posix -f posix.mk pubnub_fntest
	make -C openssl -f posix.mk pubnub_fntest
	make -C cpp -f posix.mk fntest_runner
	make -C cpp -f posix_openssl.mk openssl/fntest_runner

unittest:
	make -C core

clean:
	make -C posix -f posix.mk clean
	make -C openssl -f posix.mk clean
	make -C cpp -f posix.mk clean
	make -C cpp -f posix_openssl.mk clean
	make -C core clean
