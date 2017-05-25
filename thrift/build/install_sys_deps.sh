#!/bin/bash
set -ex

apt-get update || true
apt-get install sudo || true
sudo apt-get -y install git software-properties-common python-software-properties
sudo add-apt-repository -y ppa:ubuntu-toolchain-r/test
sudo apt-get update -qq
sudo apt-get install -qq g++-5
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-5 90
sudo apt-get install -y libdouble-conversion-dev libssl-dev make zip git \
  autoconf libtool libboost-all-dev libevent-dev flex bison \
  libgoogle-glog-dev scons libkrb5-dev libsnappy-dev libsasl2-dev \
  libnuma-dev git cmake pkg-config
