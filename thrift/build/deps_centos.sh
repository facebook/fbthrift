#!/bin/bash

. "$(dirname "$0")/deps_common.sh"

sudo yum install -y \
    openssl-devel \
    openssl-libs \
    make \
    zip \
    git \
    autoconf \
    libtool \
    gcc-c++ \
    boost \
    boost-devel \
    libevent-devel \
    libevent \
    flex \
    bison \
    scons \
    krb5-devel \
    snappy-devel \
    libgsasl-devel \
    numactl-devel \
    numactl-libs \
    gflags-devel \
    glogs-devel \
    cmake

# no rpm for this?
if [ ! -e double-conversion ]; then
  echo "Fetching double-conversion from git (yum failed)"
  git clone https://github.com/floitsch/double-conversion.git double-conversion
  cd double-conversion
  cmake . -DBUILD_SHARED_LIBS=ON
  sudo make install
  cd ..
fi

install_folly centos  # needs git
install_mstch centos  # needs git, cmake
install_zstd centos   # needs git
install_wangle centos
