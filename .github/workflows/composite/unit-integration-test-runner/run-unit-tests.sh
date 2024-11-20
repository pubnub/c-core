#!/bin/bash
set -e

ls /usr/lib/x86_64-linux-gnu/libcrypto.*

echo "::group::Run unit tests ('$1' $CC / $CXX)"
cd "$GITHUB_WORKSPACE/core"
make clean generate_report
cd "$GITHUB_WORKSPACE/lib"
make clean generate_report
echo "::endgroup::"