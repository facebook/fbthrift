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

lib_LTLIBRARIES = libthrift.la
pkgconfig_DATA = thrift.pc
libthrift_la_LDFLAGS = -version-info $(LT_VERSION) $(BOOST_LDFLAGS)

## We only build the extra libraries if we have the dependencies,
## but we install all of the headers unconditionally.
if AMX_HAVE_ZLIB
lib_LTLIBRARIES += libthriftz.la
pkgconfig_DATA += thrift-z.pc
endif

AM_CXXFLAGS = -Wall
#AM_CPPFLAGS = $(BOOST_CPPFLAGS) -I$(srcdir) -I$(srcdir)/../../..
AM_LDFLAGS = $(BOOST_LDFLAGS)

# Define the source files for the module

libthrift_la_SOURCES = EventHandlerBase.cpp \
                       Thrift.cpp \
                       TApplicationException.cpp \
                       VirtualProfiling.cpp \
                       ShutdownSocketSet.cpp \
                       concurrency/ThreadManager.cpp \
                       concurrency/TimerManager.cpp \
                       concurrency/Util.cpp \
                       concurrency/Codel.cpp \
                       protocol/TDebugProtocol.cpp \
                       protocol/TJSONProtocol.cpp \
                       protocol/TBase64Utils.cpp \
                       protocol/TMultiplexedProtocol.cpp \
                       ssl/SSLUtils.cpp \
                       transport/TTransportException.cpp \
                       transport/TFDTransport.cpp \
                       transport/TFileTransport.cpp \
                       transport/TSimpleFileTransport.cpp \
                       transport/THeaderTransport.cpp \
                       transport/THttpTransport.cpp \
                       transport/THttpClient.cpp \
                       transport/THttpServer.cpp \
                       transport/TSocket.cpp \
                       transport/TSSLSocket.cpp \
                       transport/TSocketAddress.cpp \
                       transport/TSocketPool.cpp \
                       transport/TServerSocket.cpp \
                       transport/TSSLServerSocket.cpp \
                       transport/TTransportUtils.cpp \
                       transport/TBufferTransports.cpp \
                       transport/THeader.cpp \
                       server/TServer.cpp \
                       processor/PeekProcessor.cpp \
                       util/FdUtils.cpp \
                       util/THttpParser.cpp \
                       util/SocketRetriever.cpp \
                       util/VarintUtils.cpp

libthrift_la_SOURCES += concurrency/Mutex.cpp \
                        concurrency/Monitor.cpp \
                        concurrency/PosixThreadFactory.cpp

libthriftz_la_SOURCES = transport/TZlibTransport.cpp


# Flags for the various libraries
libthriftz_la_CPPFLAGS  = $(AM_CPPFLAGS) $(ZLIB_CPPFLAGS)
libthriftz_la_CXXFLAGS  = $(AM_CXXFLAGS)
libthriftz_la_LDFLAGS   = -version-info $(LT_VERSION) $(BOOST_LDFLAGS)

include_thriftdir = $(includedir)/thrift/lib/cpp/
include_thrift_HEADERS = \
                         EventHandlerBase.h \
                         TDispatchProcessor.h \
                         Thrift.h \
			 thrift_config.h \
                         TReflectionLocal.h \
                         TProcessor.h \
                         TApplicationException.h \
                         TLogging.h \
			 ShutdownSocketSet.h

include_concurrencydir = $(include_thriftdir)/concurrency
include_concurrency_HEADERS = \
                         concurrency/Exception.h \
                         concurrency/Mutex.h \
                         concurrency/Monitor.h \
                         concurrency/PosixThreadFactory.h \
                         concurrency/Thread.h \
                         concurrency/ThreadManager.h \
                         concurrency/ThreadManager-impl.h \
                         concurrency/Codel.h \
                         concurrency/TimerManager.h \
                         concurrency/FunctionRunner.h \
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
                         protocol/TMultiplexedProtocol.h \
                         protocol/TProtocolDecorator.h \
                         protocol/TJSONProtocol.h \
                         protocol/TProtocolTap.h \
                         protocol/TProtocolException.h \
                         protocol/TVirtualProtocol.h \
                         protocol/TProtocol.h

include_ssldir = $(include_thriftdir)/ssl
include_ssl_HEADERS = \
                         ssl/SSLUtils.h


include_transportdir = $(include_thriftdir)/transport
include_transport_HEADERS = \
                         transport/THeader.h \
                         transport/TFDTransport.h \
                         transport/TFileTransport.h \
                         transport/TSimpleFileTransport.h \
                         transport/TServerSocket.h \
                         transport/TSSLServerSocket.h \
                         transport/TServerTransport.h \
                         transport/TRpcTransport.h \
                         transport/THttpTransport.h \
                         transport/THttpClient.h \
                         transport/THttpServer.h \
                         transport/TSocket.h \
                         transport/TSSLSocket.h \
                         transport/TSocketPool.h \
                         transport/TVirtualTransport.h \
                         transport/TTransport.h \
                         transport/TTransportException.h \
                         transport/TTransportUtils.h \
                         transport/TBufferTransports.h \
                         transport/TShortReadTransport.h \
                         transport/TSocketAddress.h \
                         transport/TZlibTransport.h

include_serverdir = $(include_thriftdir)/server
include_server_HEADERS = \
                         server/TServer.h \
                         server/TConnectionContext.h \
                         server/TServerObserver.h

include_processordir = $(include_thriftdir)/processor
include_processor_HEADERS = \
                         processor/PeekProcessor.h \
                         processor/StatsProcessor.h \
                         processor/TMultiplexedProcessor.h

include_asyncdir = $(include_thriftdir)/async
include_async_HEADERS = \
                     async/HHWheelTimer.h \
                     async/Request.h \
                     async/TAsyncChannel.h \
                     async/TAsyncSignalHandler.h \
                     async/TAsyncDispatchProcessor.h \
                     async/TAsyncProcessor.h \
                     async/TAsyncSSLServerSocket.h \
                     async/TAsyncSSLSocket.h \
                     async/TAsyncSSLSocketFactory.h \
                     async/TAsyncServerSocket.h \
                     async/TAsyncSocket.h \
                     async/TAsyncSocketFactory.h \
                     async/TAsyncTimeout.h \
                     async/TAsyncTimeoutSet.h \
                     async/TAsyncTransport.h \
                     async/TAsyncUDPServerSocket.h \
                     async/TAsyncUDPSocket.h \
                     async/TDelayedDestruction.h \
                     async/TEventBase.h \
                     async/TEventBaseManager.h \
                     async/TEventConnection.h \
                     async/TEventFDWrapper.h \
                     async/TEventHandler.h \
                     async/TEventJobQueue.h \
                     async/TNotificationQueue.h \
                     async/TimeoutManager.h

include_utildir = $(include_thriftdir)/util
include_util_HEADERS = \
		util/BitwiseCast.h \
		util/shared_ptr_util.h \
		util/THttpParser.h \
		util/VarintUtils.h \
		util/VarintUtils.tcc \
		util/ScopedServerThread.h

include_utilkerberosdir = $(include_utildir)/kerberos
include_utilkerberos_HEADERS = \
	util/kerberos/Krb5CCacheStore.h \
	util/kerberos/Krb5CredentialsCacheManager.h \
	util/kerberos/Krb5OlderVersionStubs.h \
	util/kerberos/Krb5Tgts.h \
	util/kerberos/Krb5Util.h

include_testdir = $(include_thriftdir)/test
include_test_HEADERS = \
	test/MockTAsyncServerSocket.h
	test/MockTAsyncSocketFactory.h
	test/MockTAsyncSocket.h
	test/MockTAsyncSSLSocket.h
	test/MockTAsyncTransport.h
	test/MockTimeoutManager.h

THRIFT = $(top_builddir)/compiler/cpp/thrift


EXTRA_DIST = \
             README \
             README.SSL \
             thrift-nb.pc.in \
             thrift.pc.in \
             thrift-z.pc.in
