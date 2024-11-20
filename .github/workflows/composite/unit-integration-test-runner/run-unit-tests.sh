#!/bin/bash
set -e
#
#ls /usr/include/openssl
#ls /usr/lib/x86_64-linux-gnu/libssl.so

echo "~~~> $1"

if [[ "$1" == "ubuntu" ]]; then
  if [ ! -f /usr/lib/x86_64-linux-gnu/pkgconfig/openssl.pc ]; then
    echo "~~~> 'openssl' package config not found!"
  else
    echo "~~~> 'openssl' package configuration exists"
    cat /usr/lib/x86_64-linux-gnu/pkgconfig/openssl.pc
    echo "----"
  fi
  echo "----"
  if [ ! -f /usr/lib/x86_64-linux-gnu/pkgconfig/libssl.pc ]; then
      echo "~~~> 'libssl' package config not found!"
  else
    echo "~~~> 'libssl' package configuration exists"
    cat /usr/lib/x86_64-linux-gnu/pkgconfig/libssl.pc
    echo "----"

    if [ ! -f /usr/lib/x86_64-linux-gnu/pkgconfig/openssl.pc ]; then
      echo "~~~> 'openssl' package config not found!"
      sudo ln -s /usr/lib/x86_64-linux-gnu/pkgconfig/libssl.pc /usr/lib/x86_64-linux-gnu/pkgconfig/openssl.pc
    else
      echo "~~~> 'openssl' package configuration exists"
    fi
  fi


  if [ ! -f /usr/lib/x86_64-linux-gnu/pkgconfig/openssl.pc ]; then
    OPENSSL_VERSION=$(openssl version | awk '{print $2}')
    echo "prefix=/usr
  exec_prefix=\${prefix}
  libdir=\${exec_prefix}/lib/x86_64-linux-gnu
  includedir=\${prefix}/include

  Name: OpenSSL
  Description: Secure Sockets Layer and cryptography libraries and tools
  Version: $OPENSSL_VERSION
  Requires:
  Libs: -L\${libdir} -lssl -lcrypto
  Cflags: -I\${includedir}" | sudo tee /usr/lib/x86_64-linux-gnu/pkgconfig/openssl.pc > /dev/null
  else
    OPENSSL_PC_FILE=/usr/lib/x86_64-linux-gnu/pkgconfig/openssl.pc
  # Fix Libs if required.
  if ! grep -q "Libs:" "$OPENSSL_PC_FILE"; then
    echo "~~~> Adding missing Libs field..."
    echo "Libs: -L/usr/lib/x86_64-linux-gnu -lssl -lcrypto" | sudo tee -a "$OPENSSL_PC_FILE"
  fi

  # Fix C flags if required.
  if ! grep -q "Cflags:" "$OPENSSL_PC_FILE"; then
    echo "~~~> Adding missing Cflags field..."
    echo "Cflags: -I/usr/include" | sudo tee -a "$OPENSSL_PC_FILE"
  fi
    echo "----"
    echo "$OPENSSL_PC_FILE"
    cat "$OPENSSL_PC_FILE"
    echo "----"
  fi

  export PKG_CONFIG_PATH=/usr/lib/x86_64-linux-gnu/pkgconfig:$PKG_CONFIG_PATH
  echo "CFLAGS: '$(pkg-config --debug --static --cflags openssl)'"
  echo "LIBS: '$(pkg-config --debug --static --libs openssl)'"
fi

echo "::group::Run unit tests ('$1' $CC / $CXX)"
cd "$GITHUB_WORKSPACE/core"
make clean generate_report
cd "$GITHUB_WORKSPACE/lib"
make clean generate_report
echo "::endgroup::"