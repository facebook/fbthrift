#!/bin/bash
# Helpers used by deps_*.sh, meant to be sourced.

set -ex

BUILD_DIR="$(readlink -f "$(dirname "$0")")"
mkdir -p "$BUILD_DIR/deps"
cd "$BUILD_DIR/deps"

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
  if [[ ! -e 'zstd' ]]; then
    git clone https://github.com/facebook/zstd.git ./zstd
  fi
  cd zstd
  sudo make install
  popd
}

install_folly() {
  pushd .
  if [[ ! -e "folly" ]]; then
    git clone https://github.com/facebook/folly
  fi
  cd folly/folly
  FOLLY_VERSION="$(cat "$BUILD_DIR"/FOLLY_VERSION)"  # on own line for set -e
  git checkout "$FOLLY_VERSION"
  # TODO: write folly dependency scripts for all supported platforms,
  # instead of having the fbthrift scripts pre-install its dependencies.
  if [[ -x "./build/deps_$1.sh" ]] ; then
    "./build/deps_$1.sh"
  fi
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
  WANGLE_VERSION="$(cat "$BUILD_DIR"/WANGLE_VERSION)"
  git checkout "$WANGLE_VERSION"
  cmake .
  make
  sudo make install
  popd
}
