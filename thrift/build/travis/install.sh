#!/usr/bin/env bash
cd ./thrift || exit 2
autoreconf -ivf
./configure
make
sudo make install

