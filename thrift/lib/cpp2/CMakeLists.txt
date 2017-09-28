# Set the cpp2 directory
set(LIB_CPP2_HOME ${CMAKE_CURRENT_SOURCE_DIR})
set(THRIFT_TEMPLATES_DIR ${THRIFT_HOME}/thrift/compiler/generate/templates)

bypass_source_check("${frozen_cpp_SOURCES}")
add_library(
  thriftfrozen2

  frozen/Frozen.cpp
  frozen/FrozenUtil.cpp
  frozen/schema/MemorySchema.cpp
  protocol/Frozen2Protocol.cpp
  ${frozen_cpp_SOURCES}
)
add_dependencies(thriftfrozen2 frozen-cpp-target)
target_link_libraries(
  thriftfrozen2

  ${DOUBLE_CONVERSION_LIBRARY}
  ${FOLLY_LIBRARY}
  ${GLOG_LIBRARIES}
  ${GFLAGS_LIBRARIES}
)
install(TARGETS thriftfrozen2 DESTINATION lib)

add_library(
  thriftprotocol

  protocol/BinaryProtocol.cpp
  protocol/CompactProtocol.cpp
  protocol/CompactV1Protocol.cpp
  protocol/DebugProtocol.cpp
  protocol/JSONProtocolCommon.cpp
  protocol/JSONProtocol.cpp
  protocol/Serializer.cpp
  protocol/SimpleJSONProtocol.cpp
  protocol/VirtualProtocol.cpp
)
target_link_libraries(
  thriftprotocol

  thrift
  ${FOLLY_LIBRARY}
  ${WANGLE_LIBRARIES}
  ${DOUBLE_CONVERSION_LIBRARY}
)
install(TARGETS thriftprotocol DESTINATION lib)

thrift_generate(
  "Sasl" #file_name
  "SaslAuthService" #services
  "cpp2" #language
  "" #options
  "${LIB_CPP2_HOME}" #file_path
  "${LIB_CPP2_HOME}" #output_path
)

bypass_source_check("${RpcMetadata-cpp2-SOURCES}")
bypass_source_check("${Sasl-cpp2-SOURCES}")
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
  ${RpcMetadata-cpp2-SOURCES}
  ${Sasl-cpp2-SOURCES}
)
add_dependencies(thriftcpp2 Sasl-cpp2-target)
target_link_libraries(
  thriftcpp2

  security
  thrift
  thriftfrozen2
  thriftprotocol
)
install(TARGETS thriftcpp2 DESTINATION lib)

# Example to test linking
PROJECT(ChatRoomServer CXX)
thrift_library(
  "chatroom" #file_name
  "ChatRoomService;Echo" #services
  "cpp2" #language
  "" #options
  "${LIB_CPP2_HOME}/example/if" #file_path
  "${LIB_CPP2_HOME}/example/if" #output_path
)
add_executable(
  chatroomserver

  example/EchoService.h
  example/EchoService.cpp
  example/ChatRoomService.h
  example/ChatRoomService.cpp
  example/ExampleServer.cpp
)
target_link_libraries(
  chatroomserver
  thriftcpp2
  chatroom-cpp2
  ${GFLAGS_LIBRARIES}
  ${GLOG_LIBRARIES}
)
install(
  TARGETS chatroomserver
  RUNTIME DESTINATION bin
  LIBRARY DESTINATION lib
)

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
