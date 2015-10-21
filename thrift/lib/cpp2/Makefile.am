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
#
# Contains some contributions under the Thrift Software License.
# Please see doc/old-thrift-license.txt in the Thrift distribution for
# details.

SUBDIRS = . test

lib_LTLIBRARIES = libthriftcpp2.la
noinst_LIBRARIES = libsaslstubs.a

gen-cpp2/Sasl_constants.cpp: gen-cpp2/Sasl_types.h
gen-cpp2/Sasl_types.cpp: gen-cpp2/Sasl_types.h
gen-cpp2/SaslAuthService.cpp: gen-cpp2/Sasl_types.h
gen-cpp2/SaslAuthService_client.cpp: gen-cpp2/Sasl_types.h

thrift2includedir = $(includedir)/thrift/lib/cpp2

thrift2include_HEADERS = \
	Thrift.h \
	GeneratedCodeHelper.h \
	ServiceIncludes.h \
	CloneableIOBuf.h \
	TestServer.h

thrift2include_asyncdir = $(thrift2includedir)/async

thrift2include_async_HEADERS = \
	async/AsyncProcessor.h \
	async/ChannelCallbacks.h \
	async/Cpp2Channel.h \
	async/ProtectionHandler.h \
	async/FramingHandler.h \
	async/HeaderCapableChannel.h \
	async/HeaderChannel.h \
	async/DuplexChannel.h \
	async/FutureRequest.h \
	async/GssSaslClient.h \
	async/GssSaslServer.h \
	async/HeaderClientChannel.h \
	async/HeaderServerChannel.h \
	async/MessageChannel.h \
	async/RequestChannel.h \
	async/ResponseChannel.h \
	async/SaslClient.h \
	async/SaslEndpoint.h \
	async/SaslServer.h \
	async/StubSaslClient.h \
	async/StubSaslServer.h \
	async/TAsyncTransportHandler.h

thrift2include_fataldir = $(thrift2includedir)/fatal

thrift2include_fatal_HEADERS = \
	fatal/folly_dynamic-inl.h \
	fatal/folly_dynamic.h \
	fatal/reflect_category-inl.h \
	fatal/reflect_category.h \
	fatal/reflection-inl.h \
	fatal/reflection.h


thrift2include_serverdir = $(thrift2includedir)/server

thrift2include_server_HEADERS = \
	server/Cpp2ConnContext.h \
	server/Cpp2Connection.h \
	server/Cpp2Worker.h \
	server/ThriftServer.h

thrift2include_utildir = $(thrift2includedir)/util

thrift2include_util_HEADERS = \
	util/ScopedServerInterfaceThread.h \
	util/ScopedServerInterfaceThread-inl.h

thrift2include_securitydir = $(thrift2includedir)/security

thrift2include_security_HEADERS = \
	security/KerberosSASLHandshakeClient.h \
	security/KerberosSASLHandshakeServer.h \
	security/KerberosSASLHandshakeUtils.h \
	security/KerberosSASLThreadManager.h \
	security/SecurityKillSwitch.h \
	security/SecurityLogger.h

thrift2include_protocoldir = $(thrift2includedir)/protocol

thrift2include_protocol_HEADERS = \
	protocol/Protocol.h \
	protocol/ProtocolReaderWithRefill.h \
	protocol/BinaryProtocol.h \
	protocol/BinaryProtocol.tcc \
	protocol/CompactProtocol.h \
	protocol/CompactProtocol.tcc \
	protocol/SimpleJSONProtocol.h \
	protocol/SimpleJSONProtocol.tcc \
	protocol/DebugProtocol.h \
	protocol/MessageSerializer.h \
	protocol/Serializer.h \
	protocol/VirtualProtocol.h

libthriftcpp2_la_SOURCES = Version.cpp \
			   GeneratedCodeHelper.cpp \
			   async/HeaderCapableChannel.cpp \
			   async/HeaderChannel.cpp \
			   async/HeaderClientChannel.cpp \
			   async/HeaderServerChannel.cpp \
			   async/StubSaslClient.cpp \
			   async/StubSaslServer.cpp \
			   async/SaslEndpoint.cpp \
			   async/GssSaslClient.cpp \
			   async/GssSaslServer.cpp \
			   async/Cpp2Channel.cpp \
			   async/ResponseChannel.cpp \
			   async/ProtectionHandler.cpp \
			   async/FramingHandler.cpp \
			   async/AsyncProcessor.cpp \
			   async/DuplexChannel.cpp \
			   protocol/Serializer.cpp \
			   protocol/SimpleJSONProtocol.cpp \
			   protocol/DebugProtocol.cpp \
			   protocol/BinaryProtocol.cpp \
			   protocol/VirtualProtocol.cpp \
			   protocol/CompactProtocol.cpp \
			   security/KerberosSASLHandshakeClient.cpp \
			   security/KerberosSASLHandshakeServer.cpp \
			   security/KerberosSASLHandshakeUtils.cpp \
			   security/KerberosSASLThreadManager.cpp \
			   security/SecurityKillSwitch.cpp \
			   server/Cpp2Connection.cpp \
			   server/Cpp2Worker.cpp \
			   server/ThriftServer.cpp \
			   util/ScopedServerInterfaceThread.cpp \
			   ../cpp/EventHandlerBase.cpp \
			   ../cpp/transport/THeaderTransport.cpp \
			   ../cpp/util/ScopedServerThread.cpp \
			   ../cpp/util/SocketRetriever.cpp \
			   ../cpp/util/VarintUtils.cpp \
			   ../cpp/util/FdUtils.cpp \
			   ../cpp/util/THttpParser.cpp \
			   ../cpp/util/PausableTimer.cpp \
			   ../cpp/util/kerberos/Krb5Util.cpp \
			   ../cpp/util/kerberos/Krb5OlderVersionStubs.cpp \
			   ../cpp/util/kerberos/Krb5CredentialsCacheManager.cpp \
			   ../cpp/util/kerberos/Krb5CCacheStore.cpp \
			   ../cpp/util/kerberos/Krb5Tgts.cpp \
			   ../cpp/util/kerberos/FBKrb5GetCreds.cpp \
			   ../cpp/concurrency/NumaThreadManager.cpp


libthriftcpp2_la_LIBADD = \
	../cpp/libthrift.la \
	libsaslstubs.a \
	$(FOLLY_LIBS) \
	-lkrb5 \
	-lgssapi_krb5 \
	-lboost_system \
	-lboost_thread

libthriftcpp2_la_LDFLAGS = -version-info $(LT_VERSION) $(BOOST_LDFLAGS)
libthriftcpp2_la_CPPFLAGS = $(AM_CPPFLAGS) $(LIBEVENT_CPPFLAGS) -I../cpp -I$(top_builddir)/../../gperftools-2.0.99/src

THRIFT = $(top_builddir)/compiler/thrift1

BUILT_SOURCES = gen-cpp2/Sasl_constants.cpp \
		gen-cpp2/Sasl_types.cpp \
		gen-cpp2/SaslAuthService.cpp \
		gen-cpp2/SaslAuthService_client.cpp

gen-cpp2/Sasl_types.h: Sasl.thrift
	PYTHONPATH=$(PY_LOCAL_PATH) $(PYTHON) -mthrift_compiler.main --gen cpp2 $<

gen-cpp2/Sasl_constants.cpp: gen-cpp2/Sasl_types.h

gen-cpp2/Sasl_types.cpp: gen-cpp2/Sasl_types.h

gen-cpp2/SaslAuthService.cpp: gen-cpp2/Sasl_types.h

libsaslstubs_a_SOURCES = gen-cpp2/Sasl_constants.cpp \
			 gen-cpp2/Sasl_types.cpp \
			 gen-cpp2/SaslAuthService.cpp \
		         gen-cpp2/SaslAuthService_client.cpp

libsaslstubs_a_CPPFLAGS = -fPIC $(AM_CPPFLAGS) $(LIBEVENT_CPPFLAGS) -I../cpp -I$(top_builddir)/../../gperftools-2.0.99/src

clean-local:
	rm -rf gen-cpp2/Sasl*
