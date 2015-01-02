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

thrift2includedir = $(includedir)/thrift/lib/cpp2

thrift2include_HEADERS = \
	Thrift.h \
	ServiceIncludes.h \
	CloneableIOBuf.h

thrift2include_asyncdir = $(thrift2includedir)/async

thrift2include_async_HEADERS = \
	async/AsyncProcessor.h \
	async/Cpp2Channel.h \
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
	async/StubSaslServer.h

thrift2include_serverdir = $(thrift2includedir)/server

thrift2include_server_HEADERS = \
	server/Cpp2ConnContext.h \
	server/Cpp2Connection.h \
	server/Cpp2Worker.h \
	server/ThriftServer.h

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
	protocol/BinaryProtocol.h \
	protocol/BinaryProtocol.tcc \
	protocol/CompactProtocol.h \
	protocol/CompactProtocol.tcc \
	protocol/DebugProtocol.h \
	protocol/MessageSerializer.h \
	protocol/Serializer.h \
	protocol/VirtualProtocol.h

libthriftcpp2_la_SOURCES = Version.cpp \
			   async/HeaderClientChannel.cpp \
			   async/StubSaslClient.cpp \
			   async/StubSaslServer.cpp \
			   async/SaslEndpoint.cpp \
			   async/GssSaslClient.cpp \
			   async/GssSaslServer.cpp \
			   async/Cpp2Channel.cpp \
			   async/AsyncProcessor.cpp \
			   async/DuplexChannel.cpp \
			   protocol/Serializer.cpp \
			   protocol/DebugProtocol.cpp \
			   security/KerberosSASLHandshakeClient.cpp \
			   security/KerberosSASLHandshakeServer.cpp \
			   security/KerberosSASLHandshakeUtils.cpp \
			   security/KerberosSASLThreadManager.cpp \
			   security/SecurityKillSwitch.cpp \
			   async/HeaderServerChannel.cpp \
			   server/Cpp2Connection.cpp \
			   server/Cpp2Worker.cpp \
			   server/ThriftServer.cpp \
			   ../cpp/async/TAsyncSignalHandler.cpp \
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
                           ../cpp/concurrency/NumaThreadManager.cpp


libthriftcpp2_la_LIBADD = ../cpp/libthrift.la libsaslstubs.a -lkrb5 -lgssapi_krb5 -lsnappy -lnuma -lboost_thread
libthriftcpp2_la_LDFLAGS = -version-info $(LT_VERSION) $(BOOST_LDFLAGS)
libthriftcpp2_la_CPPFLAGS = $(AM_CPPFLAGS) $(LIBEVENT_CPPFLAGS) -I../cpp -I$(top_builddir)/../../gperftools-2.0.99/src

THRIFT = $(top_builddir)/compiler/thrift1

BUILT_SOURCES = gen-cpp2/Sasl_constants.cpp \
		gen-cpp2/Sasl_types.cpp \
		gen-cpp2/SaslAuthService.cpp

gen-cpp2/Sasl_types.h: Sasl.thrift
	PYTHONPATH=$(PY_LOCAL_PATH) python -mthrift_compiler.main --gen cpp2 $<

gen-cpp2/Sasl_constants.cpp: gen-cpp2/Sasl_types.h

gen-cpp2/Sasl_types.cpp: gen-cpp2/Sasl_types.h

gen-cpp2/SaslAuthService.cpp: gen-cpp2/Sasl_types.h

libsaslstubs_a_SOURCES = gen-cpp2/Sasl_constants.cpp \
			 gen-cpp2/Sasl_types.cpp \
			 gen-cpp2/SaslAuthService.cpp

libsaslstubs_a_CPPFLAGS = -fPIC $(AM_CPPFLAGS) $(LIBEVENT_CPPFLAGS) -I../cpp -I$(top_builddir)/../../gperftools-2.0.99/src

clean-local:
	rm -rf gen-cpp2/Sasl*
