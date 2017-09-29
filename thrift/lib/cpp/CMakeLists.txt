add_library(
  thrift-core

  Thrift.cpp
)
target_link_libraries(
  thrift-core

  ${FOLLY_LIBRARY}
)
install(TARGETS thrift-core DESTINATION lib)

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

  ${FOLLY_LIBRARY}
  ${GFLAGS_LIBRARIES}
  ${GLOG_LIBRARIES}
)
install(TARGETS concurrency DESTINATION lib)

bypass_source_check("${reflection-cpp-SOURCES}")
add_library(
  protocol

  protocol/TDebugProtocol.cpp
  protocol/TJSONProtocol.cpp
  protocol/TBase64Utils.cpp
  protocol/TProtocolException.cpp
  protocol/TSimpleJSONProtocol.cpp
  ${reflection-cpp-SOURCES}
)
add_dependencies(protocol reflection-cpp-target)
target_link_libraries(
  protocol

  ${DOUBLE_CONVERSION_LIBRARY}
  ${FOLLY_LIBRARY}
  ${GLOG_LIBRARIES}
)
install(TARGETS protocol DESTINATION lib)

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

  concurrency
  thrift-core
  ${FOLLY_LIBRARY}
  ${OPENSSL_LIBRARIES}
  ${ZLIB_LIBRARIES}
  ${ZSTD_LIBRARIES}
)
install(TARGETS transport DESTINATION lib)

add_library(
  async

  EventHandlerBase.cpp
  async/TAsyncSocketFactory.cpp
  async/TAsyncSSLSocketFactory.cpp
  async/TBinaryAsyncChannel.cpp
  async/TEventConnection.cpp
  async/TEventServer.cpp
  async/TEventTask.cpp
  async/TEventWorker.cpp
  async/TFramedAsyncChannel.cpp
  async/THeaderAsyncChannel.cpp
  async/THttpAsyncChannel.cpp
  async/TUnframedAsyncChannel.cpp
  async/TZlibAsyncChannel.cpp
)
target_link_libraries(
  async

  concurrency
  transport
  ${Boost_LIBRARIES}
  ${FOLLY_LIBRARY}
  ${GLOG_LIBRARIES}
  ${OPENSSL_LIBRARIES}
)
install(TARGETS async DESTINATION lib)

add_library(
  security

  util/kerberos/Krb5Util.cpp
  util/kerberos/Krb5OlderVersionStubs.cpp
  util/kerberos/Krb5CredentialsCacheManager.cpp
  util/kerberos/Krb5CCacheStore.cpp
  util/kerberos/Krb5Tgts.cpp
  util/kerberos/FBKrb5GetCreds.cpp
)
target_link_libraries(
  security

  ${KRB5_LIBRARIES}
  ${Boost_LIBRARIES}
  ${FOLLY_LIBRARY}
  ${GFLAGS_LIBRARIES}
  ${GLOG_LIBRARIES}
)
install(TARGETS security DESTINATION lib)

add_library(
  server

  server/TServer.cpp
)
target_link_libraries(
  server

  ${FOLLY_LIBRARY}
)
install(TARGETS server DESTINATION lib)

add_library(
  thrift

  VirtualProfiling.cpp
)
target_link_libraries(
  thrift

  async
  concurrency
  protocol
  transport
  ${FOLLY_LIBRARY}
  ${GLOG_LIBRARIES}
)
install(TARGETS thrift DESTINATION lib)

set(THRIFT1_HEADER_DIRS
  async
  concurrency
  protocol
  server
  transport
)
foreach(dir ${THRIFT1_HEADER_DIRS})
  install(DIRECTORY ${dir} DESTINATION include/thrift/lib/cpp
          FILES_MATCHING PATTERN "*.h")
  install(DIRECTORY ${dir} DESTINATION include/thrift/lib/cpp
          FILES_MATCHING PATTERN "*.tcc")
endforeach()
