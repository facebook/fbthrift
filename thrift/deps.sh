#!/usr/bin/env bash

sudo apt-get install -yq libdouble-conversion-dev libssl-dev make zip git autoconf libtool g++ libboost-all-dev libevent-dev flex bison libgoogle-glog-dev scons libkrb5-dev libsnappy-dev libsasl2-dev libnuma-dev

# Change directory to location of this script
cd "$(dirname ${0})"
git clone https://github.com/facebook/folly
cd folly/folly
git fetch
git checkout v0.47.0

autoreconf --install
./configure
make -j8
cd ../..

autoreconf --install
CPPFLAGS=" -I`pwd`/folly/" LDFLAGS="-L`pwd`/folly/folly/.libs/" ./configure
make -j8
