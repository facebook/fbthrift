#!/usr/bin/env bash

. "$(dirname "$0")/deps_common.sh"

sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
sudo apt-get update -qq
sudo apt-get install -qq g++-5
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-5 90
sudo apt-get install -y libdouble-conversion-dev libssl-dev make zip git \
  autoconf libtool libboost-all-dev libevent-dev flex bison \
  libgoogle-glog-dev scons libkrb5-dev libsnappy-dev libsasl2-dev \
  libnuma-dev git cmake pkg-config

install_folly ubuntu_14.04  # needs git
install_mstch ubuntu_14.04  # needs git, cmake
install_zstd ubuntu_14.04   # needs git
install_wangle ubuntu_14.04 # needs git, cmake
