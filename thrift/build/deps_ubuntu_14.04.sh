#!/usr/bin/env bash

. "$(dirname "$0")/deps_common.sh"

sudo apt-get install -y libdouble-conversion-dev libssl-dev make zip git \
  autoconf libtool g++ libboost-all-dev libevent-dev flex bison \
  libgoogle-glog-dev scons libkrb5-dev libsnappy-dev libsasl2-dev \
  libnuma-dev git

install_folly ubuntu_14.04  # needs git
