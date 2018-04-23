get_property(reflection-cpp2-SOURCES GLOBAL PROPERTY reflection-cpp2-SOURCES)

add_library(
  thrift-core

  Thrift.cpp
)
target_include_directories(
  thrift-core
  INTERFACE
    $<INSTALL_INTERFACE:${INCLUDE_INSTALL_DIR}>
)
target_link_libraries(
  thrift-core
  PUBLIC
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
    ${GFLAGS_LIBRARIES}
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
    Folly::folly
    ${GLOG_LIBRARIES}
)

add_library(
  transport

  transport/TTransportException.cpp
  transport/TFDTransport.cpp
  transport/TSimpleFileTransport.cpp
  transport/THeaderTransport.cpp
  transport/THttpTransport.cpp
  transport/THttpClient.cpp
  transport/THttpServer.cpp
  transport/TSocket.cpp
  transport/TSSLSocket.cpp
  transport/TSocketOverHttpTunnel.cpp
  transport/TSocketPool.cpp
  transport/TServerSocket.cpp
  transport/TSSLServerSocket.cpp
  transport/TBufferTransports.cpp
  transport/THeader.cpp
  transport/TZlibTransport.cpp
  util/FdUtils.cpp
  util/PausableTimer.cpp
  util/ScopedServerThread.cpp
  util/SocketRetriever.cpp
  util/THttpParser.cpp
  util/VarintUtils.cpp
)
target_link_libraries(
  transport
  PUBLIC
    concurrency
    thrift-core
    Folly::folly
    ${OPENSSL_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${ZSTD_LIBRARIES}
)

add_library(
  async

  EventHandlerBase.cpp
  async/TAsyncSocketFactory.cpp
  async/TAsyncSSLSocketFactory.cpp
  async/TBinaryAsyncChannel.cpp
  async/TFramedAsyncChannel.cpp
  async/THeaderAsyncChannel.cpp
  async/THttpAsyncChannel.cpp
  async/TUnframedAsyncChannel.cpp
  async/TZlibAsyncChannel.cpp
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
  security

  util/kerberos/Krb5Util.cpp
  util/kerberos/Krb5OlderVersionStubs.cpp
  util/kerberos/Krb5CredentialsCacheManager.cpp
  util/kerberos/Krb5CCacheStore.cpp
  util/kerberos/Krb5Tgts.cpp
  util/kerberos/FBKrb5GetCreds.cpp
)
target_include_directories(
  security
  PUBLIC
    ${KRB5_INCLUDE_DIRS}
)
target_link_libraries(
  security
  PUBLIC
    ${KRB5_LIBRARIES}
    ${Boost_LIBRARIES}
    Folly::folly
    ${GFLAGS_LIBRARIES}
    ${GLOG_LIBRARIES}
)

add_library(
  server

  server/TServer.cpp
)
target_link_libraries(
  server
  PUBLIC
    Folly::folly
)

add_library(
  thrift

  VirtualProfiling.cpp
)
target_link_libraries(
  thrift
  PUBLIC
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
    security
    server
    thrift
  EXPORT fbthrift-exports
  DESTINATION ${LIB_INSTALL_DIR}
)
