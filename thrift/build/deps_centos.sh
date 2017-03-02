#!/bin/bash

. "deps_common.sh"

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
    libevent-devel \
    libevent \
    flex \
    bison \
    krb5-devel \
    snappy-devel \
    libgsasl-devel \
    numactl-devel \
    numactl-libs \
    cmake

if [ ! -e "double-conversion" ]; then
  echo "Fetching double-conversion from git (yum failed)"
  git clone https://github.com/floitsch/double-conversion.git double-conversion
  cd double-conversion
  cmake . -DBUILD_SHARED_LIBS=ON
  sudo make install
  cd ..
fi

install_zstd centos
install_mstch centos
install_libsodium centos
install_libzmq centos
install_folly centos
install_fbthrift centos
