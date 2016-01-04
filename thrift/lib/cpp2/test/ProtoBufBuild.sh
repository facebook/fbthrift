#!/bin/sh

set -e

# remove --install_dir
shift 1

protoc --cpp_out="$INSTALL_DIR" "$@"

# Fix up include path
sed -i \
  's|ProtoBufBenchData.pb.h|thrift/lib/cpp2/test/ProtoBufBenchData.pb.h|g' \
  "$INSTALL_DIR"/ProtoBufBenchData.pb.cc
