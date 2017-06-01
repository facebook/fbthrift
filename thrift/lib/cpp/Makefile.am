#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements. See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership. The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License. You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied. See the License for the
# specific language governing permissions and limitations
# under the License.
#


SUBDIRS = .

pkgconfigdir = $(libdir)/pkgconfig

if WITH_CPP
lib_LTLIBRARIES = libthrift.la
pkgconfig_DATA = thrift.pc
libthrift_la_LDFLAGS = -version-info $(LT_VERSION) $(BOOST_LDFLAGS)

## We only build the extra libraries if we have the dependencies,
## but we install all of the headers unconditionally.
if AMX_HAVE_ZLIB
lib_LTLIBRARIES += libthriftz.la
pkgconfig_DATA += thrift-z.pc
endif
endif

AM_CXXFLAGS = -Wall
#AM_CPPFLAGS = $(BOOST_CPPFLAGS) -I$(srcdir) -I$(srcdir)/../../..
AM_LDFLAGS = $(BOOST_LDFLAGS)

# Define the source files for the module

libthrift_la_SOURCES = EventHandlerBase.cpp \
                       Thrift.cpp \
                       VirtualProfiling.cpp \
                       async/TAsyncSocketFactory.cpp \
                       async/TAsyncSSLSocketFactory.cpp \
                       async/TBinaryAsyncChannel.cpp \
                       async/TEventConnection.cpp \
                       async/TEventServer.cpp \
                       async/TEventTask.cpp \
                       async/TEventWorker.cpp \
                       async/TFramedAsyncChannel.cpp \
                       async/THeaderAsyncChannel.cpp \
                       async/THttpAsyncChannel.cpp \
                       async/TUnframedAsyncChannel.cpp \
                       concurrency/ThreadManager.cpp \
                       concurrency/TimerManager.cpp \
                       concurrency/Util.cpp \
                       protocol/TDebugProtocol.cpp \
                       protocol/TJSONProtocol.cpp \
                       protocol/TBase64Utils.cpp \
                       protocol/TProtocolException.cpp \
                       protocol/TSimpleJSONProtocol.cpp \
                       transport/TTransportException.cpp \
                       transport/TFDTransport.cpp \
                       transport/TSimpleFileTransport.cpp \
                       transport/THeaderTransport.cpp \
                       transport/THttpTransport.cpp \
                       transport/THttpClient.cpp \
                       transport/THttpServer.cpp \
                       transport/TSocket.cpp \
                       transport/TSSLSocket.cpp \
                       transport/TSocketOverHttpTunnel.cpp \
                       transport/TSocketPool.cpp \
                       transport/TServerSocket.cpp \
                       transport/TSSLServerSocket.cpp \
                       transport/TBufferTransports.cpp \
                       transport/THeader.cpp \
                       server/TServer.cpp \
                       util/FdUtils.cpp \
                       util/PausableTimer.cpp \
                       util/THttpParser.cpp \
                       util/SocketRetriever.cpp \
                       util/VarintUtils.cpp

libthrift_la_SOURCES += concurrency/Mutex.cpp \
                        concurrency/Monitor.cpp \
                        concurrency/PosixThreadFactory.cpp \
                        concurrency/ProfiledMutex.cpp

libthriftz_la_SOURCES = \
                       async/TZlibAsyncChannel.cpp \
                       transport/TZlibTransport.cpp


# Flags for the various libraries
libthrift_la_LIBADD     = $(FOLLY_LIBS) $(BOOST_SYSTEM_LIB) $(BOOST_THREAD_LIB)

libthriftz_la_CPPFLAGS  = $(AM_CPPFLAGS) $(ZLIB_CPPFLAGS)
libthriftz_la_CXXFLAGS  = $(AM_CXXFLAGS)
libthriftz_la_LDFLAGS   = -version-info $(LT_VERSION) $(BOOST_LDFLAGS)

include_thriftdir = $(includedir)/thrift/lib/cpp/
include_thrift_HEADERS = \
                         EventHandlerBase.h \
                         Reflection.h \
                         TDispatchProcessor.h \
                         Thrift.h \
                         thrift_config.h \
                         TReflectionLocal.h \
                         TProcessor.h \
                         TApplicationException.h \
                         TLogging.h

include_concurrencydir = $(include_thriftdir)/concurrency
include_concurrency_HEADERS = \
                         concurrency/Exception.h \
                         concurrency/FunctionRunner.h \
                         concurrency/Monitor.h \
                         concurrency/Mutex.h \
                         concurrency/PosixThreadFactory.h \
                         concurrency/ProfiledMutex.h \
                         concurrency/Thread.h \
                         concurrency/ThreadManager-impl.h \
			 concurrency/ThreadManager.tcc \
                         concurrency/ThreadManager.h \
                         concurrency/TimerManager.h \
                         concurrency/Util.h

include_protocoldir = $(include_thriftdir)/protocol
include_protocol_HEADERS = \
                         protocol/TProtocolTypes.h \
                         protocol/TBinaryProtocol.h \
                         protocol/TBinaryProtocol.tcc \
                         protocol/TCompactProtocol.h \
                         protocol/TCompactProtocol.tcc \
                         protocol/TDebugProtocol.h \
                         protocol/TBase64Utils.h \
                         protocol/TJSONProtocol.h \
                         protocol/TSimpleJSONProtocol.h \
                         protocol/TProtocolTap.h \
                         protocol/TProtocolException.h \
                         protocol/TVirtualProtocol.h \
                         protocol/TType.h \
                         protocol/TProtocol.h


include_transportdir = $(include_thriftdir)/transport
include_transport_HEADERS = \
                         transport/THeader.h \
                         transport/TFDTransport.h \
                         transport/TSimpleFileTransport.h \
                         transport/TServerSocket.h \
                         transport/TSSLServerSocket.h \
                         transport/TServerTransport.h \
                         transport/TRpcTransport.h \
                         transport/THttpTransport.h \
                         transport/THttpClient.h \
                         transport/THttpServer.h \
                         transport/TSocket.h \
                         transport/TSocketOverHttpTunnel.h \
                         transport/TSSLSocket.h \
                         transport/TSocketPool.h \
                         transport/TVirtualTransport.h \
                         transport/TTransport.h \
                         transport/TTransportException.h \
                         transport/TNullTransport.h \
                         transport/TBufferTransports.h \
                         transport/TShortReadTransport.h \
                         transport/TZlibTransport.h

include_serverdir = $(include_thriftdir)/server
include_server_HEADERS = \
                         server/TServer.h \
                         server/TConnectionContext.h \
                         server/TServerObserver.h

include_asyncdir = $(include_thriftdir)/async
include_async_HEADERS = \
                     async/SimpleCallback.h \
                     async/TAsyncChannel.h \
                     async/TAsyncDispatchProcessor.h \
                     async/TAsyncEventChannel.h \
                     async/TAsyncProcessor.h \
                     async/TAsyncSocketFactory.h \
                     async/TAsyncSocket.h \
                     async/TAsyncSSLSocketFactory.h \
                     async/TAsyncSSLSocket.h \
                     async/TAsyncTransport.h \
                     async/TBinaryAsyncChannel.h \
                     async/TEventConnection.h \
                     async/TEventFDWrapper.h \
                     async/TEventJobQueue.h \
                     async/TEventServer.h \
                     async/TEventTask.h \
                     async/TEventWorker.h \
                     async/TFramedAsyncChannel.h \
                     async/THeaderAsyncChannel.h \
                     async/THttpAsyncChannel.h \
                     async/TQueuingAsyncProcessor.h \
                     async/TStreamAsyncChannel.h \
                     async/TSyncToAsyncProcessor.h \
                     async/TUnframedAsyncChannel.h \
                     async/TZlibAsyncChannel.h

include_utildir = $(include_thriftdir)/util
include_util_HEADERS = \
		util/BitwiseCast.h \
		util/shared_ptr_util.h \
		util/ThriftSerializer.h \
		util/ThriftSerializer-inl.h \
		util/THttpParser.h \
		util/VarintUtils.h \
		util/VarintUtils.tcc \
		util/ScopedServerThread.h

include_utilkerberosdir = $(include_utildir)/kerberos
include_utilkerberos_HEADERS = \
	util/kerberos/Krb5CCacheStore.h \
	util/kerberos/Krb5CredentialsCacheManager.h \
	util/kerberos/FBKrb5GetCreds.h \
	util/kerberos/Krb5OlderVersionStubs.h \
	util/kerberos/Krb5Tgts.h \
	util/kerberos/Krb5Util.h \
	util/kerberos/Krb5CredentialsCacheManagerLogger.h

include_testdir = $(include_thriftdir)/test
include_test_HEADERS = \
	test/MockTAsyncServerSocket.h \
	test/MockTAsyncSocketFactory.h \
	test/MockTAsyncSocket.h \
	test/MockTAsyncSSLSocket.h \
	test/MockTAsyncTransport.h

THRIFT = $(top_builddir)/compiler/cpp/thrift


EXTRA_DIST = \
             README \
             README.SSL \
             thrift-nb.pc.in \
             thrift.pc.in \
             thrift-z.pc.in
