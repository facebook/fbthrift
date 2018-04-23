# Set the cpp2 directory
set(LIB_CPP2_HOME ${CMAKE_CURRENT_SOURCE_DIR})
set(THRIFT_TEMPLATES_DIR ${THRIFT_HOME}/thrift/compiler/generate/templates)


#######
# CMAKE variables only have local/subdirectory scope
# So even though this is defined in ../thrift/CMakeLists.txt as a variable
# it's not accessible here and silently defaults to ""
#
# Using global properties gets around this; probably better to restructure
# the directories/files to avoid this type of hack
#
get_property(RpcMetadata-cpp2-SOURCES GLOBAL PROPERTY RpcMetadata-cpp2-SOURCES)
get_property(frozen-cpp2-SOURCES GLOBAL PROPERTY frozen-cpp2-SOURCES)

bypass_source_check("${frozen-cpp2-SOURCES}")
add_library(
  thriftfrozen2

  frozen/Frozen.cpp
  frozen/FrozenUtil.cpp
  frozen/schema/MemorySchema.cpp
  protocol/Frozen2Protocol.cpp
  ${frozen-cpp2-SOURCES}
)
add_dependencies(thriftfrozen2 frozen-cpp2-target)
target_link_libraries(
  thriftfrozen2
  PUBLIC
    Folly::folly
    ${GLOG_LIBRARIES}
    ${GFLAGS_LIBRARIES}
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
  protocol/SimpleJSONProtocol.cpp
  protocol/VirtualProtocol.cpp
)
target_link_libraries(
  thriftprotocol
  PUBLIC
    thrift
    Folly::folly
    wangle::wangle
)

thrift_generate(
  "Sasl" #file_name
  "SaslAuthService" #services
  "cpp2" #language
  "" #options
  "${LIB_CPP2_HOME}" #file_path
  "${LIB_CPP2_HOME}" #output_path
  "thrift/lib/cpp2" #include_prefix
)

bypass_source_check("${RpcMetadata-cpp2-SOURCES}")
bypass_source_check("${Sasl-cpp2-SOURCES}")
add_library(
  thriftcpp2

  FrozenTApplicationException.cpp
  GeneratedCodeHelper.cpp
  async/AsyncClient.cpp
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
  server/peeking/TLSHelper.cpp
  transport/core/ThriftProcessor.cpp
  util/ScopedServerInterfaceThread.cpp
  ${RpcMetadata-cpp2-SOURCES}
  ${Sasl-cpp2-SOURCES}
)
add_dependencies(thriftcpp2 Sasl-cpp2-target)
target_link_libraries(
  thriftcpp2
  PUBLIC
    security
    thrift
    thriftfrozen2
    thriftprotocol
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
  install(DIRECTORY ${dir} DESTINATION ${INCLUDE_INSTALL_DIR}/thrift/lib/cpp2
          FILES_MATCHING PATTERN "*.h")
  install(DIRECTORY ${dir} DESTINATION ${INCLUDE_INSTALL_DIR}/thrift/lib/cpp2
          FILES_MATCHING PATTERN "*.tcc")
endforeach()

install(
  TARGETS
    thriftfrozen2
    thriftprotocol
    thriftcpp2
  EXPORT fbthrift-exports
  DESTINATION ${LIB_INSTALL_DIR}
)
