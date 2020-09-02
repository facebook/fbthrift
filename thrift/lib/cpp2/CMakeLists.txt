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

# Set the cpp2 directory
set(LIB_CPP2_HOME ${CMAKE_CURRENT_SOURCE_DIR})


#######
# CMAKE variables only have local/subdirectory scope
# So even though this is defined in ../thrift/CMakeLists.txt as a variable
# it's not accessible here and silently defaults to ""
#
# Using global properties gets around this; probably better to restructure
# the directories/files to avoid this type of hack
#
get_property(RpcMetadata-cpp2-SOURCES GLOBAL PROPERTY RpcMetadata-cpp2-SOURCES)
get_property(metadata-cpp2-SOURCES GLOBAL PROPERTY metadata-cpp2-SOURCES)
get_property(frozen-cpp2-SOURCES GLOBAL PROPERTY frozen-cpp2-SOURCES)

bypass_source_check("${metadata-cpp2-SOURCES}")
add_library(
  thriftmetadata

  ${metadata-cpp2-SOURCES}
)
add_dependencies(thriftmetadata metadata-cpp2-target)
target_link_libraries(
  thriftmetadata
  PUBLIC
    Folly::folly
)

bypass_source_check("${frozen-cpp2-SOURCES}")
add_library(
  thriftfrozen2

  frozen/Frozen.cpp
  frozen/FrozenUtil.cpp
  frozen/schema/MemorySchema.cpp
  ${frozen-cpp2-SOURCES}
)
add_dependencies(thriftfrozen2 frozen-cpp2-target thriftmetadata)
target_link_libraries(
  thriftfrozen2
  PUBLIC
    Folly::folly
    thriftmetadata
    ${GLOG_LIBRARIES}
    ${LIBGFLAGS_LIBRARY}
)

bypass_source_check("${RpcMetadata-cpp2-SOURCES}")
add_library(
  rpcmetadata

  ${RpcMetadata-cpp2-SOURCES}
)
add_dependencies(rpcmetadata RpcMetadata-cpp2-target)
target_link_libraries(
  rpcmetadata
  PUBLIC
    Folly::folly
)

add_library(
  thriftprotocol

  protocol/BinaryProtocol.cpp
  protocol/CompactProtocol.cpp
  protocol/CompactV1Protocol.cpp
  protocol/DebugProtocol.cpp
  protocol/JSONProtocolCommon.cpp
  protocol/JSONProtocol.cpp
  protocol/Serializer.cpp
  protocol/VirtualProtocol.cpp
)
target_link_libraries(
  thriftprotocol
  PUBLIC
    thrift
    Folly::folly
    wangle::wangle
)

add_library(
  thriftcpp2

  GeneratedCodeHelper.cpp
  async/AsyncClient.cpp
  async/AsyncProcessor.cpp
  async/ClientSinkBridge.cpp
  async/ClientStreamBridge.cpp
  async/Cpp2Channel.cpp
  async/DuplexChannel.cpp
  async/FramingHandler.cpp
  async/HeaderChannel.cpp
  async/HeaderChannelTrait.cpp
  async/HeaderClientChannel.cpp
  async/HeaderServerChannel.cpp
  async/RequestChannel.cpp
  async/ResponseChannel.cpp
  async/RocketClientChannel.cpp
  async/RpcTypes.cpp
  async/ServerGeneratorStream.cpp
  async/ServerSinkBridge.cpp
  gen/module_types_cpp.cpp
  security/extensions/ThriftParametersClientExtension.cpp
  security/extensions/ThriftParametersContext.cpp
  security/extensions/Types.cpp
  server/RequestDebugLog.cpp
  server/RequestsRegistry.cpp
  server/BaseThriftServer.cpp
  server/Cpp2ConnContext.cpp
  server/Cpp2Connection.cpp
  server/Cpp2Worker.cpp
  server/ServerInstrumentation.cpp
  server/ThriftServer.cpp
  server/peeking/TLSHelper.cpp
  transport/core/RpcMetadataUtil.cpp
  transport/core/ThriftProcessor.cpp
  transport/core/ThriftRequest.cpp
  transport/core/ThriftClient.cpp
  transport/core/ThriftClientCallback.cpp
  transport/rocket/PayloadUtils.cpp
  transport/rocket/Types.cpp
  transport/rocket/client/RequestContext.cpp
  transport/rocket/client/RequestContextQueue.cpp
  transport/rocket/client/RocketClient.cpp
  transport/rocket/client/RocketStreamServerCallback.cpp
  transport/rocket/framing/ErrorCode.cpp
  transport/rocket/framing/Frames.cpp
  transport/rocket/framing/Serializer.cpp
  transport/rocket/framing/Util.cpp
  transport/rocket/server/RocketRoutingHandler.cpp
  transport/rocket/server/RocketServerConnection.cpp
  transport/rocket/server/RocketServerFrameContext.cpp
  transport/rocket/server/RocketSinkClientCallback.cpp
  transport/rocket/server/RocketStreamClientCallback.cpp
  transport/rocket/server/RocketThriftRequests.cpp
  transport/rocket/server/ThriftRocketServerHandler.cpp
  util/Checksum.cpp
  util/ScopedServerInterfaceThread.cpp
  util/ScopedServerThread.cpp
)
add_dependencies(
  thriftcpp2
  rpcmetadata
  thriftmetadata
)
target_link_libraries(
  thriftcpp2
  PUBLIC
    thrift
    rpcmetadata
    thriftmetadata
    thriftfrozen2
    thriftprotocol
)

# Install includes
set(THRIFT2_HEADER_DIRS
  async
  frozen
  protocol
  server
  transport
  util
)
install(
  DIRECTORY ${THRIFT2_HEADER_DIRS}
  DESTINATION ${INCLUDE_INSTALL_DIR}/thrift/lib/cpp2
  FILES_MATCHING PATTERN "*.h" PATTERN "*.tcc"
)

install(
  TARGETS
    rpcmetadata
    thriftmetadata
    thriftfrozen2
    thriftprotocol
    thriftcpp2
  EXPORT fbthrift-exports
  DESTINATION ${LIB_INSTALL_DIR}
)
