#!/bin/sh

# remove --fbcode_dir and --install_dir
shift 2

PB_PATH=$FBCODE_DIR/$EXT_PROJECT_protobuf
LD_LIBRARY_PATH=$PB_PATH/lib $PB_PATH/bin/protoc --cpp_out=$INSTALL_DIR "$@"
