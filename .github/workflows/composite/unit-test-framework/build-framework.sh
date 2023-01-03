#!/bin/bash

# Identify make tool which depends from environment.
[[ $1 =~ (ubuntu|macos) ]] && MAKE_TOOL=make || MAKE_TOOL=$(MAKE)

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
    echo "::warning title=compiler flags::Here should be some setup for Windows compiler"
  fi

  echo "Prepare folders structure"
  ! [[ -d "$GITHUB_WORKSPACE/cgreen-tmp/$compiler" ]] &&
    mkdir -p "$GITHUB_WORKSPACE/cgreen-tmp/$compiler"
  mkdir -p build && cd build

  # Enable 'gcov' only for build on Ubuntu.
  [[ "$1" == "ubuntu" ]] && WITH_GCOV=ON || WITH_GCOV=OFF
  if [[ "$WITH_GCOV" == ON ]]; then
    echo "Installing 'gcov' for code coverage."
    if ! install="$(pip install --user gcovr)"; then
      echo "::error title=gcov::Unable to install 'gcov':$install"
    else
    echo "Installing 'gcov' for code coverage."
    fi
  else
    echo "::warning title=gcov::'gcov' doesn't work as expected on $1."
    echo "::warning title=gcov::Configure 'cgreen' without 'gcov' support"
  fi

  echo "Configure 'cgreen' build"
  cmake -E env LDFLAGS="-lm" cmake -DCGREEN_INTERNAL_WITH_GCOV:BOOL=$WITH_GCOV ..

  echo "Build 'cgreen' framework"
  make -j2

  echo "Move built results to compiler specific colder"
  mv "$GITHUB_WORKSPACE/cgreen/build" "$GITHUB_WORKSPACE/cgreen-tmp/$compiler/build"
  cp -r "$GITHUB_WORKSPACE/cgreen/include" "$GITHUB_WORKSPACE/cgreen-tmp/$compiler/include"

  # Clean up configured environments.
  unset CC && unset CXX
  echo "::endgroup::"
done

echo "Prepare files for caching"
rm -rf "$GITHUB_WORKSPACE/cgreen"
mv "$GITHUB_WORKSPACE/cgreen-tmp" "$GITHUB_WORKSPACE/cgreen"