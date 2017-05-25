#!/bin/bash
set -ex
if [ -z "$1" ]
  then
    echo "First argument should be directory with local checkout of Thrift source repository"
    exit
fi
docker rm thrift_sys_container || true
docker run --name thrift_sys_container -v "$1":/fbthrift ubuntu:latest /bin/bash -c "/fbthrift/thrift/build/install_sys_deps.sh"
docker commit thrift_sys_container thrift_sys_image

docker rm thrift_lib_container || true
docker run --name thrift_lib_container -v "$1":/fbthrift thrift_sys_image /bin/bash -c "cd fbthrift/thrift/build; /fbthrift/thrift/build/install_lib_deps.sh"
docker commit thrift_lib_container thrift_lib_image
