FROM ubuntu:20.04 AS cucumber_cpp
ARG DEBIAN_FRONTEND=noninteractive
WORKDIR /home

ENV GMOCK_VER=1.7.0
ENV CMAKE_CXX_COMPILER=/usr/bin/g++

RUN apt-get update
RUN apt-get install -y cmake g++ ruby ruby-dev git ninja-build libboost-all-dev gcovr libssl-dev gdb curl
RUN gem install bundler

ENV GMOCK_VER=1.7.0
ENV CMAKE_CXX_COMPILER=/usr/bin/g++

RUN git clone https://github.com/cucumber/cucumber-cpp.git

WORKDIR /home/cucumber-cpp

RUN bundle install
RUN git submodule init
RUN git submodule update
RUN cmake -E make_directory build
RUN cmake -E chdir build cmake --DCUKE_ENABLE_EXAMPLES=on ..
RUN cmake --build build
RUN cmake --build build --target test

FROM ubuntu:20.04 AS cgreen
ARG DEBIAN_FRONTEND=noninteractive
WORKDIR /home

ENV GMOCK_VER=1.7.0
ENV CMAKE_CXX_COMPILER=/usr/bin/g++

RUN apt-get update
RUN apt-get install -y cmake g++ ruby ruby-dev git ninja-build libboost-all-dev gcovr libssl-dev gdb curl

RUN git clone https://github.com/cgreen-devs/cgreen.git
RUN cd cgreen && git checkout 1.4.1 && make

FROM ubuntu:20.04 
ARG DEBIAN_FRONTEND=noninteractive
WORKDIR /home

ENV GMOCK_VER=1.7.0 
ENV CMAKE_CXX_COMPILER=/usr/bin/g++

RUN apt-get update
RUN apt-get install -y cmake g++ ruby ruby-dev git ninja-build libboost-all-dev gcovr libssl-dev gdb curl

RUN gem install cucumber
RUN apt-get install -y python3-pip
RUN pip install requests

COPY --from=cucumber_cpp /home/cucumber-cpp /home/cucumber-cpp/
COPY --from=cgreen /home/cgreen /home/cgreen/

COPY core /home/core/
COPY lib /home/lib/
RUN cd core && make generate_report
COPY posix /home/posix/
COPY cpp /home/cpp/
COPY openssl /home/openssl/
RUN cd cpp && make -f posix_openssl.mk openssl/pubnub_sync

ARG SUB_KEY
ARG PUB_KEY
ARG SEC_KEY
ARG MOCK_SERVER_DOCKER
ENV PAM_SUB_KEY=${SUB_KEY}
ENV PAM_PUB_KEY=${PUB_KEY}
ENV PAM_SEC_KEY=${SEC_KEY}


COPY sdk-specifications/features /home/features/
COPY features/step_definitions /home/features/step_definitions/

RUN if [ -z "$MOCK_SERVER_DOCKER" ]; then \
        g++ -std=c++11 -g -o BoostSteps.o -c features/step_definitions/BoostSteps.cpp \
        -Icucumber-cpp/include -Icucumber-cpp/build/src/ -Iposix -Icore -I. -Icpp \
        -D PUBNUB_CRYPTO_API=1 -D PUBNUB_USE_SSL=0; \
    else \
        g++ -std=c++11 -g -o BoostSteps.o -c features/step_definitions/BoostSteps.cpp \
        -Icucumber-cpp/include -Icucumber-cpp/build/src/ -Iposix -Icore -I. -Icpp \
        -D PUBNUB_CRYPTO_API=1 -D PUBNUB_USE_SSL=0 -D MOCK_SERVER_DOCKER; \
    fi
    

RUN g++ -o steps BoostSteps.o cpp/pubnub_sync.a cucumber-cpp/build/src/libcucumber-cpp.a \
        -Lboost -lboost_unit_test_framework -lpthread -lboost_regex \
        -lboost_thread -lboost_program_options -lboost_filesystem \
        -lssl -lcrypto -D PUBNUB_USE_SSL=0

COPY run_contract_tests.py .

ENTRYPOINT [ "python3", "/home/run_contract_tests.py", "/home/features/access/revoke-token.feature", "mock_server"]
