#!/bin/bash

echo "::group::Run unit tests ('${{ inputs.os }}' gcc)"
cd "$GITHUB_WORKSPACE/core"
make clean generate_report
cd "$GITHUB_WORKSPACE/lib"
make clean generate_report
echo "::endgroup::"