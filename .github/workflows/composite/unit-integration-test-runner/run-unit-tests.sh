#!/bin/bash
set -e

if [ -d "/usr/lib/x86_64-linux-gnu/" ]; then
ls /usr/lib/x86_64-linux-gnu/libcrypto.*
echo "----"
nm -D /usr/lib/x86_64-linux-gnu/libcrypto.so
echo "----"
nm -D /usr/lib/x86_64-linux-gnu/libcrypto.so | grep EVP_DecryptFinal_ex
echo "----"
export LD_LIBRARY_PATH=/usr/lib/x86_64-linux-gnu:$LD_LIBRARY_PATH
fi


echo "::group::Run unit tests ('$1' $CC / $CXX)"
cd "$GITHUB_WORKSPACE/core"
make clean generate_report
cd "$GITHUB_WORKSPACE/lib"
make clean generate_report
echo "::endgroup::"