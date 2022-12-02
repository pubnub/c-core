#!/bin/bash

INFO_BG="\033[0m\033[48;2;5;49;70m"
INFO_FG="\033[38;2;19;181;255m"
BOLD_INFO_FG="${INFO_FG}\033[1m"
SUCCESS_BG="\033[0m\033[48;2;30;69;1m"
SUCCESS_FG="\033[38;2;95;215;0m"
BOLD_SUCCESS_FG="${SUCCESS_FG}\033[1m"
CLEAR="\033[0m"

echo "::group::Run integration tests ('$1' $CC / $CXX)"
cd "$GITHUB_WORKSPACE"

if ! build="$(make -f posix.mk clean all 2>&1)"; then
  echo "::error file=posix.mk::Unable to build test suite: $build"
  exit 1
else
  echo -e "${SUCCESS_BG}Test suit is built 🎉${CLEAR}"
fi

[[ "$build" =~ "deprecated" ]] && echo "::warning title=deprecated::There is deprecated functions used in code."
[[ "$build" =~ "not used" ]] && echo "::warning title=unused::There is unused variables and / or functions in code."

echo -e "${INFO_BG}${INFO_FG}Running ${BOLD_INFO_FG}POSIX ${INFO_BG}${INFO_FG}integration tests...${CLEAR}"
posix/pubnub_fntest
echo -e "${SUCCESS_BG}${BOLD_SUCCESS_FG}POSIX ${SUCCESS_BG}${SUCCESS_FG}integration tests completed 🎉${CLEAR}"
echo -e "${INFO_BG}${INFO_FG}Running ${BOLD_INFO_FG}OpenSSL ${INFO_BG}${INFO_FG}integration tests${CLEAR}"
openssl/pubnub_fntest
echo -e "${SUCCESS_BG}${BOLD_SUCCESS_FG}POSIX ${SUCCESS_BG}${SUCCESS_FG}integration tests completed 🎉${CLEAR}"
echo -e "${INFO_BG}${INFO_FG}Running ${BOLD_INFO_FG}C++ ${INFO_BG}${INFO_FG}integration tests${CLEAR}"
cpp/fntest_runner
echo -e "${SUCCESS_BG}${BOLD_SUCCESS_FG}POSIX ${SUCCESS_BG}${SUCCESS_FG}integration tests completed 🎉${CLEAR}"
echo -e "${INFO_BG}${INFO_FG}Running ${BOLD_INFO_FG}C++ OpenSSL ${INFO_BG}${INFO_FG}integration tests${CLEAR}"
cpp/openssl/fntest_runner
echo -e "${SUCCESS_BG}${BOLD_SUCCESS_FG}POSIX ${SUCCESS_BG}${SUCCESS_FG}integration tests completed 🎉${CLEAR}"
echo "::endgroup::"
