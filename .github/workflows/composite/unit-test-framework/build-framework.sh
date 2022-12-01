#!/bin/bash

# Identify make tool which depends from environment.
[[ $1 =~ (ubuntu|macos) ]] && MAKE_TOOL=make || MAKE_TOOL=$(MAKE)

echo "BEFORE:"
ls -la "$GITHUB_WORKSPACE"

# Iterating over list of passed compilers.
for compiler in ${2//,/ }
do
  # Navigate to unit test framework source folder.
  cd "$GITHUB_WORKSPACE/cgreen"

  # Clean up previous build results.
  [[ -d build ]] && $MAKE_TOOL clean && rm -rf build

  echo "::group::Build unit test framework ('$1' $compiler)"
  if [[ $1 =~ (ubuntu|macos) ]]; then
    export CC="$compiler"
    [[ $compiler == "gcc" ]] && export CXX="g++" || export CXX="$compiler++"
  else
    echo "::notice::Here should be some setup for Windows compiler"
  fi

  echo "::debug title=cgreen::Prepare folders structure"
  if ! [[ -d "$GITHUB_WORKSPACE/cgreen-tmp/$compiler" ]]; then
    echo "$GITHUB_WORKSPACE/cgreen-tmp/$compiler DOESN'T EXISTS. CREATE"
    mkdir -p "$GITHUB_WORKSPACE/cgreen-tmp/$compiler"
    echo "COMPILER FOLDER CREATE:"
    ls -la "$GITHUB_WORKSPACE"
    echo "COMPILER FOLDER INSIDE:"
    ls -la "$GITHUB_WORKSPACE/cgreen-tmp"
  fi
  mkdir -p build && cd build

  # Enable 'gcov' only for build on Ubuntu.
  [[ "$1" == "ubuntu" ]] && WITH_GCOV=ON || WITH_GCOV=OFF
  if [[ "$WITH_GCOV" == ON ]]; then
    echo "::debug title=gcov::Installing 'gcov' for code coverage."
    pip install --user gcovr
  else
    echo "::notice title=gcov::'gcov' doesn't work as expected on $1."
    echo "::notice title=gcov::Configure 'cgreen' without 'gcov' support"
  fi

  echo "::debug title=cgreen::Configure 'cgreen' build"
  cmake -E env LDFLAGS="-lm" cmake -DCGREEN_INTERNAL_WITH_GCOV:BOOL=$WITH_GCOV ..

  echo "::debug title=cgreen::Build 'cgreen' framework"
  make -j2

  echo "::debug::Move built results to compiler specific colder"
  mv "$GITHUB_WORKSPACE/cgreen/build" "$GITHUB_WORKSPACE/cgreen-tmp/$compiler/build"
  cp -r "$GITHUB_WORKSPACE/cgreen/include" "$GITHUB_WORKSPACE/cgreen-tmp/$compiler/include"

  # Clean up configured environments.
  unset CC && unset CXX
  echo "::endgroup::"
done

echo "AFTER:"
ls -la "$GITHUB_WORKSPACE"

echo "::debug title=cgreen::Prepare files for caching"
rm -rf cgreen
mv cgreen-tmp cgreen