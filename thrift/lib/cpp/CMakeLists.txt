# Copyright (c) Facebook, Inc. and its affiliates.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

get_property(reflection-cpp2-SOURCES GLOBAL PROPERTY reflection-cpp2-SOURCES)

add_library(
  thrift-core

  Thrift.cpp
  ../cpp2/FieldRef.cpp
)
target_include_directories(
  thrift-core
  INTERFACE
    $<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}>
)
target_link_libraries(
  thrift-core
  PUBLIC
    fmt::fmt
    Folly::folly
)

add_library(
  concurrency

  concurrency/Mutex.cpp
  concurrency/Monitor.cpp
  concurrency/PosixThreadFactory.cpp
  concurrency/ThreadManager.cpp
  concurrency/TimerManager.cpp
  concurrency/Util.cpp
)
target_link_libraries(
  concurrency
  PUBLIC
    Folly::folly
    ${LIBGFLAGS_LIBRARY}
    ${GLOG_LIBRARIES}
)

bypass_source_check("${reflection-cpp2-SOURCES}")
add_library(
  protocol

  protocol/TDebugProtocol.cpp
  protocol/TJSONProtocol.cpp
  protocol/TBase64Utils.cpp
  protocol/TProtocolException.cpp
  protocol/TSimpleJSONProtocol.cpp
  ${reflection-cpp2-SOURCES}
)
add_dependencies(protocol reflection-cpp2-target)
target_link_libraries(
  protocol
  PUBLIC
    fmt::fmt
    Folly::folly
    ${GLOG_LIBRARIES}
    thrift-core
)

add_library(
  transport

  transport/TTransportException.cpp
  transport/TFDTransport.cpp
  transport/THttpTransport.cpp
  transport/THttpClient.cpp
  transport/THttpServer.cpp
  transport/TSocket.cpp
  transport/TBufferTransports.cpp
  transport/THeader.cpp
  transport/TZlibTransport.cpp
  util/PausableTimer.cpp
  util/THttpParser.cpp
  util/VarintUtils.cpp
)
add_dependencies(
  transport
  rpcmetadata
)
target_link_libraries(
  transport
  PUBLIC
    concurrency
    thrift-core
    rpcmetadata
    Folly::folly
    ${OPENSSL_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${ZSTD_LIBRARIES}
)

add_library(
  async

  ContextStack.cpp
  EventHandlerBase.cpp
  async/TBinaryAsyncChannel.cpp
  async/TFramedAsyncChannel.cpp
  async/THttpAsyncChannel.cpp
  async/TUnframedAsyncChannel.cpp
  async/TZlibAsyncChannel.cpp
  server/TServerObserver.cpp
)
target_link_libraries(
  async
  PUBLIC
    concurrency
    transport
    ${Boost_LIBRARIES}
    Folly::folly
    ${GLOG_LIBRARIES}
    ${OPENSSL_LIBRARIES}
)

add_library(
  thrift
  INTERFACE
)
target_link_libraries(
  thrift
  INTERFACE
    async
    concurrency
    protocol
    transport
    Folly::folly
    ${GLOG_LIBRARIES}
)

set(THRIFT1_HEADER_DIRS
  async
  concurrency
  protocol
  server
  transport
)
foreach(dir ${THRIFT1_HEADER_DIRS})
  install(DIRECTORY ${dir} DESTINATION ${INCLUDE_INSTALL_DIR}/thrift/lib/cpp
          FILES_MATCHING PATTERN "*.h")
  install(DIRECTORY ${dir} DESTINATION ${INCLUDE_INSTALL_DIR}/thrift/lib/cpp
          FILES_MATCHING PATTERN "*.tcc")
endforeach()

install(
  TARGETS
    thrift-core
    concurrency
    protocol
    transport
    async
    thrift
  EXPORT fbthrift-exports
  DESTINATION ${LIB_INSTALL_DIR}
)
