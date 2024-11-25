FROM ubuntu:20.04 AS cucumber_cpp
ARG DEBIAN_FRONTEND=noninteractive

RUN apt update
RUN apt install -y build-essential libssl-dev pkg-config

ENV CMAKE_CXX_COMPILER=/usr/bin/g++
ENV PUBNUB_ORIGIN=h2.pubnubapi.com
ENV PUBNUB_USE_IPV6_CONNECTIVITY=1

COPY cpp cpp
COPY core core
COPY lib lib
COPY posix posix
COPY openssl openssl

WORKDIR /openssl
RUN USE_IPV6=1 make -f posix.mk clean pubnub_publish_via_post_sample

CMD ["./pubnub_publish_via_post_sample"]