# Set the cpp2 directory
include_directories(${CMAKE_CURRENT_BINARY_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR}/gen-cpp2)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/gen-cpp2)
set(LIB_CPP2_HOME ${CMAKE_CURRENT_SOURCE_DIR})
set(THRIFT_BINARY ${THRIFT_HOME}/thrift/compiler/thrift-compiler)
set(THRIFT_TEMPLATES_DIR ${THRIFT_HOME}/thrift/compiler/generate/templates)

add_custom_command(
  OUTPUT gen-cpp2/Sasl_constants.h gen-cpp2/Sasl_types.h gen-cpp2/Sasl_data.h gen-cpp2/Sasl_types_custom_protocol.h gen-cpp2/Sasl_constants.cpp gen-cpp2/Sasl_data.cpp gen-cpp2/Sasl_types.cpp gen-cpp2/Sasl_types.tcc gen-cpp2/SaslAuthService_client.cpp gen-cpp2/SaslAuthService.cpp gen-cpp2/SaslAuthService_custom_protocol.h gen-cpp2/SaslAuthService.h gen-cpp2/SaslAuthService_processmap_binary.cpp gen-cpp2/SaslAuthService_processmap_compact.cpp gen-cpp2/SaslAuthService.tcc

  COMMAND ${THRIFT_BINARY}
  ARGS --gen mstch_cpp2:include_prefix=${THRIFT_HOME}/thrift/lib/cpp2 --templates ${THRIFT_TEMPLATES_DIR} ${LIB_CPP2_HOME}/Sasl.thrift
  COMMENT "Generating Sasl files"
)
add_custom_target(
  lib-sasl ALL
  DEPENDS gen-cpp2/Sasl_constants.h gen-cpp2/Sasl_types.h gen-cpp2/Sasl_data.h gen-cpp2/Sasl_types_custom_protocol.h gen-cpp2/Sasl_constants.cpp gen-cpp2/Sasl_data.cpp gen-cpp2/Sasl_types.cpp gen-cpp2/Sasl_types.tcc gen-cpp2/SaslAuthService_client.cpp gen-cpp2/SaslAuthService.cpp gen-cpp2/SaslAuthService_custom_protocol.h gen-cpp2/SaslAuthService.h gen-cpp2/SaslAuthService_processmap_binary.cpp gen-cpp2/SaslAuthService_processmap_compact.cpp gen-cpp2/SaslAuthService.tcc
)

add_library(
  async-cpp2

  async/AsyncProcessor.h
  async/ChannelCallbacks.h
  async/ClientChannel.h
  async/Cpp2Channel.h
  async/DuplexChannel.h
  async/FramingHandler.h
  async/FutureRequest.h
  async/GssSaslClient.h
  async/GssSaslServer.h
  async/HeaderChannel.h
  async/HeaderChannelTrait.h
  async/HeaderClientChannel.h
  async/HeaderServerChannel.h
  async/MessageChannel.h
  async/PcapLoggingHandler.h
  async/ProtectionHandler.h
  async/RequestChannel.h
  async/ResponseChannel.h
  async/SaslClient.h
  async/SaslEndpoint.h
  async/SaslNegotiationHandler.h
  async/SaslServer.h
  async/ServerChannel.h
  async/StubSaslClient.h
  async/StubSaslServer.h
  async/TAsyncTransportHandler.h

  async/AsyncProcessor.cpp
  async/Cpp2Channel.cpp
  async/DuplexChannel.cpp
  async/FramingHandler.cpp
  async/GssSaslClient.cpp
  async/GssSaslServer.cpp
  async/HeaderChannel.cpp
  async/HeaderChannelTrait.cpp
  async/HeaderClientChannel.cpp
  async/HeaderServerChannel.cpp
  async/PcapLoggingHandler.cpp
  async/ProtectionHandler.cpp
  async/RequestChannel.cpp
  async/ResponseChannel.cpp
  async/SaslEndpoint.cpp
  async/SaslNegotiationHandler.cpp
  async/StubSaslClient.cpp
  async/StubSaslServer.cpp

  gen-cpp2/Sasl_constants.h
  gen-cpp2/Sasl_types.h
  gen-cpp2/Sasl_data.h
  gen-cpp2/Sasl_types_custom_protocol.h
  gen-cpp2/Sasl_constants.cpp
  gen-cpp2/Sasl_data.cpp
  gen-cpp2/Sasl_types.cpp
  gen-cpp2/Sasl_types.tcc
  gen-cpp2/SaslAuthService_client.cpp
  gen-cpp2/SaslAuthService.cpp
  gen-cpp2/SaslAuthService_custom_protocol.h
  gen-cpp2/SaslAuthService.h
  gen-cpp2/SaslAuthService_processmap_binary.cpp
  gen-cpp2/SaslAuthService_processmap_compact.cpp
  gen-cpp2/SaslAuthService.tcc
)
add_dependencies(
  async-cpp2

  thrift-cpp
  lib-sasl
)
target_link_libraries(
  async-cpp2

  ${FOLLY_LIBRARIES}
  ${WANGLE_LIBRARIES}
)

add_library(
  frozen-cpp2

  frozen/Frozen.h
  frozen/FrozenAssociative-inl.h
  frozen/FrozenBool-inl.h
  frozen/FrozenEnum-inl.h
  frozen/FrozenHashTable-inl.h
  frozen/FrozenIntegral-inl.h
  frozen/FrozenMacros.h
  frozen/FrozenOptional-inl.h
  frozen/FrozenOrderedTable-inl.h
  frozen/FrozenPair-inl.h
  frozen/FrozenRange-inl.h
  frozen/FrozenString-inl.h
  frozen/FrozenTestUtil.h
  frozen/FrozenTrivial-inl.h
  frozen/FrozenUtil.h
  frozen/HintTypes.h
  frozen/Traits.h
  frozen/VectorAssociative.h
  frozen/schema/MemorySchema.h

  frozen/FrozenUtil.cpp
  frozen/schema/MemorySchema.cpp
)
add_dependencies(
  frozen-cpp2

  lib-thrift
)
target_link_libraries(
  frozen-cpp2

  ${FOLLY_LIBRARIES}
)

add_library(
  protocol-cpp2

  protocol/BinaryProtocol.h
  protocol/BinaryProtocol.tcc
  protocol/CompactProtocol.h
  protocol/CompactProtocol.tcc
  protocol/CompactV1Protocol.h
  protocol/CompactV1Protocol.tcc
  protocol/DebugProtocol.h
  protocol/Frozen2Protocol.h
  protocol/Frozen2Protocol.tcc
  protocol/JSONProtocolCommon.h
  protocol/JSONProtocolCommon.tcc
  protocol/JSONProtocol.h
  protocol/JSONProtocol.tcc
  protocol/MessageSerializer.h
  protocol/Protocol.h
  protocol/ProtocolReaderWithRefill.h
  protocol/Serializer.h
  protocol/SimpleJSONProtocol.h
  protocol/SimpleJSONProtocol.tcc
  protocol/VirtualProtocol.h

  protocol/BinaryProtocol.cpp
  protocol/CompactProtocol.cpp
  protocol/CompactV1Protocol.cpp
  protocol/DebugProtocol.cpp
  protocol/Frozen2Protocol.cpp
  protocol/JSONProtocolCommon.cpp
  protocol/JSONProtocol.cpp
  protocol/Serializer.cpp
  protocol/SimpleJSONProtocol.cpp
  protocol/VirtualProtocol.cpp
)
add_dependencies(
  protocol-cpp2

  thrift-cpp
)
target_link_libraries(
  protocol-cpp2

  ${FOLLY_LIBRARIES}
  ${WANGLE_LIBRARIES}
)

add_library(
  security-cpp2

  security/KerberosSASLHandshakeClient.h
  security/KerberosSASLHandshakeServer.h
  security/KerberosSASLHandshakeUtils.h
  security/KerberosSASLThreadManager.h
  security/SecurityKillSwitch.h
  security/SecurityKillSwitchPoller.h
  security/SecurityLogger.h

  security/KerberosSASLHandshakeClient.cpp
  security/KerberosSASLHandshakeServer.cpp
  security/KerberosSASLHandshakeUtils.cpp
  security/KerberosSASLThreadManager.cpp
  security/SecurityKillSwitch.cpp
  security/SecurityKillSwitchPoller.cpp
)
target_link_libraries(
  security-cpp2

  ${WANGLE_LIBRARIES}
  ${FOLLY_LIBRARIES}
  ${OPENSSL_LIBRARIES}
)

add_library(
  transport-cpp2

  transport/core/ThriftProcessor.h
  transport/core/TransportRoutingHandler.h
  transport/core/ThriftChannelIf.h
  transport/core/FunctionInfo.h

  transport/core/ThriftProcessor.cpp
)
target_link_libraries(
  transport-cpp2

  thrift-cpp
  async-cpp2
)

add_library(
  server-cpp2

  server/BaseThriftServer.h
  server/Cpp2ConnContext.h
  server/Cpp2Connection.h
  server/Cpp2Worker.h
  server/ThriftServer.h

  server/BaseThriftServer.cpp
  server/Cpp2Connection.cpp
  server/Cpp2Worker.cpp
  server/ThriftServer.cpp

  server/peeking/HTTPHelper.h
  server/peeking/PeekingManager.h
  server/peeking/TLSHelper.h

  server/peeking/HTTPHelper.cpp
  server/peeking/TLSHelper.cpp
)
target_link_libraries(
  server-cpp2

  transport-cpp2
  ${WANGLE_LIBRARIES}
  ${FOLLY_LIBRARIES}
)

add_library(
  util-cpp2

  util/Frozen2ViewHelpers.h
  util/MapCopy.h
  util/ScopedServerInterfaceThread.h
  util/ScopedServerInterfaceThread-inl.h

  util/ScopedServerInterfaceThread.cpp
)
target_link_libraries(
  util-cpp2

  ${FOLLY_LIBRARIES}
)

add_library(
  thriftcpp2

  CloneableIOBuf.h
  FrozenTApplicationException.h
  GeneratedCodeHelper.h
  GeneratedHeaderHelper.h
  GeneratedSerializationCodeHelper.h
  SerializationSwitch.h
  ServiceIncludes.h
  Thrift.h
  TypeClass.h

  FrozenTApplicationException.cpp
  GeneratedCodeHelper.cpp
  Version.cpp
)
target_link_libraries(
  thriftcpp2

  thrift-cpp
  async-cpp2
  frozen-cpp2
  protocol-cpp2
  security-cpp2
  transport-cpp2
  server-cpp2
  util-cpp2
)
