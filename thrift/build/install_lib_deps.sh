#!/bin/bash
set -ex

install_mstch() {
  pushd .
  if [[ ! -e "mstch" ]]; then
    git clone https://github.com/no1msd/mstch
  fi
  cd mstch
  cmake .
  make
  sudo make install
  popd
}

install_zstd() {
  pushd .
  if [[ ! -e "zstd" ]]; then
    git clone https://github.com/facebook/zstd
  fi
  cd zstd
  make
  sudo make install
  popd
}

install_gtest() {
  pushd .
  if [[ ! -e "gtest" ]]; then
    GTEST_RELEASE_TAG="release-1.8.0"
    git clone -b $GTEST_RELEASE_TAG --single-branch --depth 1 \
      https://github.com/google/googletest gtest
  fi

  cd gtest/googletest
  cmake .
  make
  sudo make install

  cd ../googlemock
  cmake .
  make
  sudo make install

  popd
}

install_glog() {
  pushd .
  if [[ ! -e "glog" ]]; then
    git clone https://github.com/google/glog
  fi
  cd glog
  GLOG_RELEASE_HASH="b6a5e0524c28178985f0d228e9eaa43808dbec3c"
  git reset --hard $GLOG_RELEASE_HASH

  cmake .
  make
  sudo make install

  popd
}


install_folly() {
  pushd .
  if [[ ! -e "folly" ]]; then
    git clone https://github.com/facebook/folly
  fi
  cd folly/folly
  autoreconf -if
  ./configure
  make
  sudo make install
  sudo ldconfig
  popd
}

install_wangle() {
  pushd .
  if [[ ! -e "wangle" ]]; then
    git clone https://github.com/facebook/wangle
  fi
  cd wangle/wangle
  cmake .
  make
  sudo make install
  popd
}

mkdir -p deps
cd deps

install_gtest
install_glog
install_mstch
install_zstd
install_folly
install_wangle
