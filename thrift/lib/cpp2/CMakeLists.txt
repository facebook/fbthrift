# Set the cpp2 directory
set(LIB_CPP2_HOME ${CMAKE_CURRENT_SOURCE_DIR})
set(THRIFT_BINARY ${THRIFT_BUILD}/thrift/compiler/thrift1)
set(THRIFT_TEMPLATES_DIR ${THRIFT_HOME}/thrift/compiler/generate/templates)

set(SASL_HEADERS
  gen-cpp2/SaslAuthService.h
  gen-cpp2/SaslAuthService.tcc
  gen-cpp2/SaslAuthService_custom_protocol.h
  gen-cpp2/Sasl_constants.h
  gen-cpp2/Sasl_data.h
  gen-cpp2/Sasl_types.h
  gen-cpp2/Sasl_types.tcc
  gen-cpp2/Sasl_types_custom_protocol.h
)
set(SASL_SOURCES
  gen-cpp2/SaslAuthService.cpp
  gen-cpp2/SaslAuthService_client.cpp
  gen-cpp2/SaslAuthService_processmap_binary.cpp
  gen-cpp2/SaslAuthService_processmap_compact.cpp
  gen-cpp2/Sasl_constants.cpp
  gen-cpp2/Sasl_data.cpp
  gen-cpp2/Sasl_types.cpp
)

add_custom_command(
  OUTPUT ${SASL_HEADERS} ${SASL_SOURCES}
  COMMAND ${THRIFT_BINARY}
  ARGS --gen mstch_cpp2:include_prefix=${LIB_CPP2_HOME} --templates ${THRIFT_TEMPLATES_DIR} -o ${LIB_CPP2_HOME} ${LIB_CPP2_HOME}/Sasl.thrift
  DEPENDS thrift1
  COMMENT "Generating Sasl files"
)
add_custom_target(
  lib-sasl ALL
  DEPENDS ${SASL_HEADERS} ${SASL_SOURCES}
)

add_library(
  thriftasync

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
)
add_dependencies(
  thriftasync

  thrift
  lib-sasl
)
target_link_libraries(
  thriftasync

  ${FOLLY_LIBRARY}
  ${WANGLE_LIBRARIES}
)

 add_library(
   thriftfrozen2

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
   thriftfrozen2

   lib-thrift
 )
 target_link_libraries(
   thriftfrozen2

   ${FOLLY_LIBRARY}
 )

 add_library(
   thriftprotocol

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
   thriftprotocol

   lib-thrift
   thrift
 )
 target_link_libraries(
   thriftprotocol

   ${FOLLY_LIBRARY}
   ${WANGLE_LIBRARIES}
 )

add_library(
  thriftsecurity

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
  thriftsecurity

  ${WANGLE_LIBRARIES}
  ${FOLLY_LIBRARY}
  ${OPENSSL_LIBRARIES}
)

add_library(
  thrifttransport

  transport/core/ThriftProcessor.h
  transport/core/TransportRoutingHandler.h
  transport/core/ThriftChannelIf.h
  transport/core/FunctionInfo.h

  transport/core/ThriftProcessor.cpp
)
target_link_libraries(
  thrifttransport

  thrift
  thriftasync
)

add_library(
  thriftserver

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
  thriftserver

  thrifttransport
  ${WANGLE_LIBRARIES}
  ${FOLLY_LIBRARY}
)

add_library(
  thriftutil

  util/Frozen2ViewHelpers.h
  util/MapCopy.h
  util/ScopedServerInterfaceThread.h
  util/ScopedServerInterfaceThread-inl.h

  util/ScopedServerInterfaceThread.cpp
)
target_link_libraries(
  thriftutil

  ${FOLLY_LIBRARY}
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

  thrift
  thriftasync
  thriftfrozen2
  thriftprotocol
  thriftsecurity
  thrifttransport
  thriftserver
  thriftutil
)
