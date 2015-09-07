# Make all that can be made on POSIX

all:
	make -C posix -f posix.mk
	make -C openssl -f posix.mk
	make -C cpp -f posix.mk
	make -C cpp -f posix_openssl.mk

unittest:
	make -C core

clean:
	make -C posix -f posix.mk clean
	make -C openssl -f posix.mk clean
	make -C cpp -f posix.mk clean
	make -C cpp -f posix_openssl.mk clean
	make -C core clean
