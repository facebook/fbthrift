# Set the cpp2 directory
set(LIB_CPP2_HOME ${CMAKE_CURRENT_SOURCE_DIR})
set(THRIFT_TEMPLATES_DIR ${THRIFT_HOME}/thrift/compiler/generate/templates)

set_source_files_properties(
  ${FROZEN_CPP_SOURCES}
  PROPERTIES GENERATED TRUE
)
add_library(frozen-cpp ${FROZEN_CPP_SOURCES})
add_dependencies(frozen-cpp frozen-cpp-target)
target_link_libraries(
  frozen-cpp
  thrift
)

add_library(
  thriftfrozen2

  frozen/Frozen.cpp
  frozen/FrozenUtil.cpp
  frozen/schema/MemorySchema.cpp
)
target_link_libraries(
  thriftfrozen2

  frozen-cpp
  ${FOLLY_LIBRARY}
  ${DOUBLE_CONVERSION_LIBRARY}
)

add_library(
  thriftprotocol

  ${LIB_HOME}/cpp/Thrift.cpp
  ${LIB_HOME}/cpp/concurrency/Util.cpp
  ${LIB_HOME}/cpp/transport/TTransportException.cpp
  ${LIB_HOME}/cpp/util/VarintUtils.cpp
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
target_link_libraries(
  thriftprotocol

  ${FOLLY_LIBRARY}
  ${WANGLE_LIBRARIES}
  ${DOUBLE_CONVERSION_LIBRARY}
)
install(TARGETS thriftprotocol DESTINATION lib)

set(SASL_CPP2_HEADERS
  ${LIB_CPP2_HOME}/gen-cpp2/SaslAuthService.h
  ${LIB_CPP2_HOME}/gen-cpp2/SaslAuthService.tcc
  ${LIB_CPP2_HOME}/gen-cpp2/SaslAuthService_custom_protocol.h
  ${LIB_CPP2_HOME}/gen-cpp2/Sasl_constants.h
  ${LIB_CPP2_HOME}/gen-cpp2/Sasl_data.h
  ${LIB_CPP2_HOME}/gen-cpp2/Sasl_types.h
  ${LIB_CPP2_HOME}/gen-cpp2/Sasl_types.tcc
  ${LIB_CPP2_HOME}/gen-cpp2/Sasl_types_custom_protocol.h
)
set(SASL_CPP2_SOURCES
  ${LIB_CPP2_HOME}/gen-cpp2/SaslAuthService.cpp
  ${LIB_CPP2_HOME}/gen-cpp2/SaslAuthService_client.cpp
  ${LIB_CPP2_HOME}/gen-cpp2/SaslAuthService_processmap_binary.cpp
  ${LIB_CPP2_HOME}/gen-cpp2/SaslAuthService_processmap_compact.cpp
  ${LIB_CPP2_HOME}/gen-cpp2/Sasl_constants.cpp
  ${LIB_CPP2_HOME}/gen-cpp2/Sasl_data.cpp
  ${LIB_CPP2_HOME}/gen-cpp2/Sasl_types.cpp
)
add_custom_command(
  OUTPUT ${SASL_CPP2_HEADERS} ${SASL_CPP2_SOURCES}
  COMMAND ${THRIFT_BINARY}
  ARGS --gen mstch_cpp2:include_prefix=${LIB_CPP2_HOME} --templates ${THRIFT_TEMPLATES_DIR} -o ${LIB_CPP2_HOME} ${LIB_CPP2_HOME}/Sasl.thrift
  DEPENDS thrift1
  COMMENT "Generating Sasl files"
)
add_custom_target(
  sasl-cpp2-target ALL
  DEPENDS ${SASL_CPP2_HEADERS} ${SASL_CPP2_SOURCES}
)
set_source_files_properties(
  ${SASL_CPP2_SOURCES}
  PROPERTIES GENERATED TRUE
)

add_library(
  thriftcpp2

  FrozenTApplicationException.cpp
  GeneratedCodeHelper.cpp
  Version.cpp
  async/AsyncProcessor.cpp
  async/Cpp2Channel.cpp
  async/DuplexChannel.cpp
  async/FramingHandler.cpp
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
  async/GssSaslClient.cpp
  async/GssSaslServer.cpp
  security/KerberosSASLHandshakeClient.cpp
  security/KerberosSASLHandshakeServer.cpp
  security/KerberosSASLHandshakeUtils.cpp
  security/KerberosSASLThreadManager.cpp
  security/SecurityKillSwitch.cpp
  security/SecurityKillSwitchPoller.cpp
  server/BaseThriftServer.cpp
  server/Cpp2Connection.cpp
  server/Cpp2Worker.cpp
  server/ThriftServer.cpp
  server/peeking/HTTPHelper.cpp
  server/peeking/TLSHelper.cpp
  transport/core/ThriftProcessor.cpp
  util/ScopedServerInterfaceThread.cpp
  ${LIB_HOME}/cpp/EventHandlerBase.cpp
  ${LIB_HOME}/cpp/transport/THeaderTransport.cpp
  ${LIB_HOME}/cpp/util/ScopedServerThread.cpp
  ${LIB_HOME}/cpp/util/SocketRetriever.cpp
  ${LIB_HOME}/cpp/util/VarintUtils.cpp
  ${LIB_HOME}/cpp/util/FdUtils.cpp
  ${LIB_HOME}/cpp/util/THttpParser.cpp
  ${LIB_HOME}/cpp/util/PausableTimer.cpp
  ${LIB_HOME}/cpp/util/kerberos/Krb5Util.cpp
  ${LIB_HOME}/cpp/util/kerberos/Krb5OlderVersionStubs.cpp
  ${LIB_HOME}/cpp/util/kerberos/Krb5CredentialsCacheManager.cpp
  ${LIB_HOME}/cpp/util/kerberos/Krb5CCacheStore.cpp
  ${LIB_HOME}/cpp/util/kerberos/Krb5Tgts.cpp
  ${LIB_HOME}/cpp/util/kerberos/FBKrb5GetCreds.cpp
  ${SASL_CPP2_SOURCES}
)
add_dependencies(thriftcpp2 sasl-cpp2-target)
target_link_libraries(
  thriftcpp2

  thrift
  thriftfrozen2
  thriftprotocol
  ${DOUBLE_CONVERSION_LIBRARY}
  ${FOLLY_LIBRARY}
  ${GFLAGS_LIBRARIES}
  ${GLOG_LIBRARIES}
  ${KRB5_LIBRARIES}
  ${WANGLE_LIBRARIES}
  ${ZSTD_LIBRARIES}
  ${OPENSSL_LIBRARIES}
  ${Boost_LIBRARIES}
)
install(TARGETS thriftcpp2 DESTINATION lib)

# Example to test linking
set(CHATROOM_CPP2_HEADERS
  ${LIB_CPP2_HOME}/example/if/gen-cpp2/ChatRoomService.h
  ${LIB_CPP2_HOME}/example/if/gen-cpp2/ChatRoomService.tcc
  ${LIB_CPP2_HOME}/example/if/gen-cpp2/ChatRoomService_custom_protocol.h
  ${LIB_CPP2_HOME}/example/if/gen-cpp2/ChatRoomService_constants.h
  ${LIB_CPP2_HOME}/example/if/gen-cpp2/ChatRoomService_data.h
  ${LIB_CPP2_HOME}/example/if/gen-cpp2/ChatRoomService_types.h
  ${LIB_CPP2_HOME}/example/if/gen-cpp2/ChatRoomService_types.tcc
  ${LIB_CPP2_HOME}/example/if/gen-cpp2/ChatRoomService_types_custom_protocol.h
)
set(CHATROOM_CPP2_SOURCES
  ${LIB_CPP2_HOME}/example/if/gen-cpp2/ChatRoomService.cpp
  ${LIB_CPP2_HOME}/example/if/gen-cpp2/ChatRoomService_client.cpp
  ${LIB_CPP2_HOME}/example/if/gen-cpp2/ChatRoomService_processmap_binary.cpp
  ${LIB_CPP2_HOME}/example/if/gen-cpp2/ChatRoomService_processmap_compact.cpp
  ${LIB_CPP2_HOME}/example/if/gen-cpp2/ChatRoomService_constants.cpp
  ${LIB_CPP2_HOME}/example/if/gen-cpp2/ChatRoomService_data.cpp
  ${LIB_CPP2_HOME}/example/if/gen-cpp2/ChatRoomService_types.cpp
)
add_custom_command(
  OUTPUT ${CHATROOM_CPP2_HEADERS} ${CHATROOM_CPP2_SOURCES}
  COMMAND ${THRIFT_BINARY}
  ARGS --gen mstch_cpp2:include_prefix=${LIB_CPP2_HOME}/example/if --templates ${THRIFT_TEMPLATES_DIR} -o ${LIB_CPP2_HOME}/example/if ${LIB_CPP2_HOME}/example/if/ChatRoomService.thrift
  DEPENDS thrift1
  COMMENT "Generating Chatroom files"
)
add_custom_target(
  chatroom-cpp2-target ALL
  DEPENDS ${CHATROOM_CPP2_HEADERS} ${CHATROOM_CPP2_SOURCES}
)
set_source_files_properties(
  ${CHATROOM_CPP2_SOURCES}
  PROPERTIES GENERATED TRUE
)
add_library(
  chatroom-cpp2
  ${CHATROOM_CPP2_SOURCES}
)
add_dependencies(chatroom-cpp2 chatroom-cpp2-target)
target_link_libraries(
  chatroom-cpp2
  thrift
  thriftprotocol
  thriftcpp2
  ${FOLLY_LIBRARY}
  ${WANGLE_LIBRARIES}
  ${ZSTD_LIBRARIES}
  ${OPENSSL_LIBRARIES}
)

#PROJECT(ChatRoomServer CXX)
#add_executable(
#  chatroomserver
#
#  example/ChatRoomService.h
#  example/ChatRoomService.cpp
#  example/ChatRoomServiceMain.cpp
#)
#target_link_libraries(
#  chatroomserver
#  chatroom-cpp2
#)
#install(
#  TARGETS chatroomserver
#  RUNTIME DESTINATION bin
#  LIBRARY DESTINATION lib
#)

# Install includes
set(THRIFT2_HEADER_DIRS
  async
  frozen
  gen-cpp2
  protocol
  security
  server
  transport
  util
)
foreach(dir ${THRIFT2_HEADER_DIRS})
  install(DIRECTORY ${dir} DESTINATION include/thrift/lib/cpp2
          FILES_MATCHING PATTERN "*.h")
  install(DIRECTORY ${dir} DESTINATION include/thrift/lib/cpp2
          FILES_MATCHING PATTERN "*.tcc")
endforeach()
