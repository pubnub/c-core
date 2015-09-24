# Make all that can be made on Windows

all:
	cd windows && $(MAKE) /f windows.mk
	cd openssl && $(MAKE) /f windows.mk
	cd cpp && $(MAKE) /f windows.mk
	cd cpp && $(MAKE) /f windows_openssl.mk


clean:
	cd windows && $(MAKE) /f windows.mk clean
	cd openssl && $(MAKE) /f windows.mk clean
	cd cpp && $(MAKE) /f windows.mk clean
	cd cpp && $(MAKE) /f windows_openssl.mk clean
