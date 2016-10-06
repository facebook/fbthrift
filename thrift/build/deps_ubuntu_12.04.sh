#!/bin/bash

# Installs fbthrift's dependencies to /usr/local on a clean Ubuntu 12.04 x64
# system.  Primarily intended for Travis CI, since most engineers don't run
# distributions this stale.
#
# WARNING: Uses 'sudo' to upgrade your system with impunity:
#  - Adds several PPAs for missing/outdated dependencies
#  - Installs several from-source dependencies in /usr/local
#
# Library sources & build files end up in fbthrift/thrift/deps.

. "$(dirname "$0")/deps_common.sh"

sudo apt-get install git cmake

# Folly sets up a bunch of other dependencies, so install it right after git.
install_folly ubuntu_12.04  # needs git
install_mstch ubuntu_12.04  # needs git, cmake
install_zstd ubuntu_12.04   # needs git
install_wangle ubuntu_12.04

# Uses PPAs set up by folly.  TODO: According to the fbthrift docs,
# pkg-config is missing.  However, things seem to build fine...
sudo apt-get install -y libboost-python1.54-dev libsasl2-dev python-dev \
   libkrb5-dev libnuma-dev

# Install all the automake packages:
#  - bison: The base system version is too old, does not make 'thrify.hh'
#  - flex: Just in case -- the base system version probably does suffice...
#  - automake: default 1.11.1 has bad bison support, does not make 'thrifty.hh'
#  - autoconf, libtool: newer so as to be compatible with the new automake
for url in \
    http://ftp.gnu.org/gnu/bison/bison-3.0.4.tar.gz \
    http://downloads.sourceforge.net/project/flex/flex-2.5.37.tar.gz \
    http://ftp.gnu.org/gnu/autoconf/autoconf-2.69.tar.gz \
    http://ftp.gnu.org/gnu/automake/automake-1.15.tar.gz \
    http://ftp.gnu.org/gnu/libtool/libtool-2.4.6.tar.gz \
    ; do
  pkg=$(basename "$url")
  wget "$url" -O "$pkg"
  tar xzf "$pkg"
  pushd "${pkg%.tar.gz}"
  ./configure
  make
  sudo make install
  popd
done
