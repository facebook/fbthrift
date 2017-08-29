add_library(
  concurrency

  thrift_config.h
  concurrency/Util.h
  concurrency/Exception.h
  concurrency/FunctionRunner.h
  concurrency/Monitor.h
  concurrency/Mutex.h
  concurrency/PosixThreadFactory.h
  concurrency/ProfiledMutex.h
  concurrency/Thread.h
  concurrency/ThreadManager-impl.h
  concurrency/ThreadManager.tcc
  concurrency/ThreadManager.h
  concurrency/TimerManager.h
  concurrency/Util.h

  concurrency/Mutex.cpp
  concurrency/Monitor.cpp
  concurrency/PosixThreadFactory.cpp
  concurrency/ProfiledMutex.cpp
  concurrency/ThreadManager.cpp
  concurrency/TimerManager.cpp
  concurrency/Util.cpp
)

add_library(
  protocol

  Reflection.h
  protocol/TProtocolTypes.h
  protocol/TBinaryProtocol.h
  protocol/TBinaryProtocol.tcc
  protocol/TCompactProtocol.h
  protocol/TCompactProtocol.tcc
  protocol/TDebugProtocol.h
  protocol/TBase64Utils.h
  protocol/TJSONProtocol.h
  protocol/TSimpleJSONProtocol.h
  protocol/TProtocolTap.h
  protocol/TProtocolException.h
  protocol/TVirtualProtocol.h
  protocol/TType.h
  protocol/TProtocol.h

  protocol/TDebugProtocol.cpp
  protocol/TJSONProtocol.cpp
  protocol/TBase64Utils.cpp
  protocol/TProtocolException.cpp
  protocol/TSimpleJSONProtocol.cpp
)
add_dependencies(
  protocol

  lib-thrift
)

add_library(
  transport

  transport/THeader.h
  transport/TFDTransport.h
  transport/TSimpleFileTransport.h
  transport/TServerSocket.h
  transport/TSSLServerSocket.h
  transport/TServerTransport.h
  transport/TRpcTransport.h
  transport/THttpTransport.h
  transport/THttpClient.h
  transport/THttpServer.h
  transport/TSocket.h
  transport/TSocketOverHttpTunnel.h
  transport/TSSLSocket.h
  transport/TSocketPool.h
  transport/TVirtualTransport.h
  transport/TTransport.h
  transport/TTransportException.h
  transport/TNullTransport.h
  transport/TBufferTransports.h
  transport/TShortReadTransport.h
  transport/TZlibTransport.h

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
)
target_link_libraries(
  transport

  ${ZSTD_LIBRARIES}
)

add_library(
  async

  async/SimpleCallback.h
  async/TAsyncChannel.h
  async/TAsyncDispatchProcessor.h
  async/TAsyncEventChannel.h
  async/TAsyncProcessor.h
  async/TAsyncSocketFactory.h
  async/TAsyncSocket.h
  async/TAsyncSSLSocketFactory.h
  async/TAsyncSSLSocket.h
  async/TAsyncTransport.h
  async/TBinaryAsyncChannel.h
  async/TEventConnection.h
  async/TEventFDWrapper.h
  async/TEventJobQueue.h
  async/TEventServer.h
  async/TEventTask.h
  async/TEventWorker.h
  async/TFramedAsyncChannel.h
  async/THeaderAsyncChannel.h
  async/THttpAsyncChannel.h
  async/TQueuingAsyncProcessor.h
  async/TStreamAsyncChannel.h
  async/TSyncToAsyncProcessor.h
  async/TUnframedAsyncChannel.h
  async/TZlibAsyncChannel.h

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

add_library(
  util

  util/BitwiseCast.h
  util/shared_ptr_util.h
  util/ThriftSerializer.h
  util/ThriftSerializer-inl.h
  util/THttpParser.h
  util/VarintUtils.h
  util/VarintUtils.tcc
  util/ScopedServerThread.h
  util/kerberos/Krb5CCacheStore.h
  util/kerberos/Krb5CredentialsCacheManager.h
  util/kerberos/FBKrb5GetCreds.h
  util/kerberos/Krb5OlderVersionStubs.h
  util/kerberos/Krb5Tgts.h
  util/kerberos/Krb5Util.h
  util/kerberos/Krb5CredentialsCacheManagerLogger.h

  util/FdUtils.cpp
  util/THttpParser.cpp
  util/SocketRetriever.cpp
  util/VarintUtils.cpp
)

add_library(
  server

  server/TServer.h
  server/TConnectionContext.h
  server/TServerObserver.h

  server/TServer.cpp
)

add_library(
  thrift-cpp

  DistinctTable.h
  EventHandlerBase.h
  Reflection.h
  TDispatchProcessor.h
  Thrift.h
  thrift_config.h
  TReflectionLocal.h
  TProcessor.h
  TApplicationException.h
  TLogging.h

  EventHandlerBase.cpp
  Thrift.cpp
  VirtualProfiling.cpp
)
target_link_libraries(
  thrift-cpp

  async
  concurrency
  protocol
  transport
  ${FOLLY_LIBRARIES}
  ${GLOG_LIBRARIES}
)
