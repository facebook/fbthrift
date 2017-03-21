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

lib_LTLIBRARIES = libthriftprotocol.la

if WITH_CPP
lib_LTLIBRARIES += libthriftcpp2.la
noinst_LIBRARIES = libsaslstubs.a
endif

gen-cpp2/Sasl_constants.cpp: gen-cpp2/Sasl_types.h
gen-cpp2/Sasl_data.cpp: gen-cpp2/Sasl_data.h gen-cpp2/Sasl_types.h
gen-cpp2/Sasl_types.cpp: gen-cpp2/Sasl_types.h gen-cpp2/Sasl_data.h
gen-cpp2/SaslAuthService.cpp: gen-cpp2/Sasl_types.h
gen-cpp2/SaslAuthService_client.cpp: gen-cpp2/Sasl_types.h

thrift2includedir = $(includedir)/thrift/lib/cpp2

thrift2include_HEADERS = \
	Thrift.h \
	GeneratedCodeHelper.h \
	GeneratedHeaderHelper.h \
	ServiceIncludes.h \
	CloneableIOBuf.h

thrift2include_asyncdir = $(thrift2includedir)/async

thrift2include_async_HEADERS = \
	async/AsyncProcessor.h \
	async/ChannelCallbacks.h \
	async/Cpp2Channel.h \
	async/ProtectionHandler.h \
	async/FramingHandler.h \
	async/SaslNegotiationHandler.h \
	async/PcapLoggingHandler.h \
	async/HeaderChannel.h \
	async/HeaderChannelTrait.h \
	async/DuplexChannel.h \
	async/FutureRequest.h \
	async/GssSaslClient.h \
	async/GssSaslServer.h \
	async/HeaderClientChannel.h \
	async/HeaderServerChannel.h \
	async/HTTPClientChannel.h \
	async/MessageChannel.h \
	async/RequestChannel.h \
	async/ResponseChannel.h \
	async/SaslClient.h \
	async/SaslEndpoint.h \
	async/SaslServer.h \
	async/StubSaslClient.h \
	async/StubSaslServer.h \
	async/TAsyncTransportHandler.h \
	async/ClientChannel.h \
	async/ServerChannel.h

thrift2include_fataldir = $(thrift2includedir)/fatal

thrift2include_fatal_HEADERS = \
	fatal/container_traits.h \
	fatal/container_traits_folly.h \
	fatal/internal/debug-inl-post.h \
	fatal/internal/debug-inl-pre.h \
	fatal/debug.h \
	fatal/internal/folly_dynamic-inl-pre.h \
	fatal/internal/folly_dynamic-inl-post.h \
	fatal/folly_dynamic.h \
	fatal/internal/helpers-inl-pre.h \
	fatal/helpers.h \
	fatal/indenter.h \
	fatal/internal/legacy_reflection-inl-pre.h \
	fatal/internal/legacy_reflection-inl-post.h \
	fatal/legacy_reflection.h \
	fatal/internal/flatten_getters-inl-post.h \
	fatal/internal/flatten_getters-inl-pre.h \
	fatal/flatten_getters.h \
	fatal/internal/merge-inl-pre.h \
	fatal/internal/merge-inl-post.h \
	fatal/merge.h \
	fatal/internal/pretty_print-inl-pre.h \
	fatal/internal/pretty_print-inl-post.h \
	fatal/pretty_print.h \
	fatal/internal/reflection-inl-post.h \
	fatal/internal/reflection-inl-pre.h \
	fatal/reflection.h \
	fatal/testing.h \
	fatal/traits_test_helpers.h \
	fatal/internal/traits_test_helpers-inl.h \
	fatal/internal/test_helpers.h \
	fatal/variant.h \
	fatal/frozen1.h

thrift2include_fatal_SOURCES = \
	fatal/legacy_reflection.cpp

thrift2include_serverdir = $(thrift2includedir)/server

thrift2include_server_HEADERS = \
	server/BaseThriftServer.h \
	server/Cpp2ConnContext.h \
	server/Cpp2Connection.h \
	server/Cpp2Worker.h \
	server/SSLRejectingManager.h \
	server/ThriftServer.h \
	server/TLSHelper.h

thrift2include_utildir = $(thrift2includedir)/util

thrift2include_util_HEADERS = \
	util/MapCopy.h \
	util/ScopedServerInterfaceThread.h \
	util/ScopedServerInterfaceThread-inl.h

thrift2include_securitydir = $(thrift2includedir)/security

thrift2include_security_HEADERS = \
	security/KerberosSASLHandshakeClient.h \
	security/KerberosSASLHandshakeServer.h \
	security/KerberosSASLHandshakeUtils.h \
	security/KerberosSASLThreadManager.h \
	security/SecurityKillSwitch.h \
	security/SecurityKillSwitchPoller.h \
	security/SecurityLogger.h

thrift2include_protocoldir = $(thrift2includedir)/protocol

thrift2include_protocol_HEADERS = \
	protocol/Protocol.h \
	protocol/ProtocolReaderWithRefill.h \
	protocol/BinaryProtocol.h \
	protocol/BinaryProtocol.tcc \
	protocol/CompactProtocol.h \
	protocol/CompactProtocol.tcc \
	protocol/CompactV1Protocol.h \
	protocol/CompactV1Protocol.tcc \
	protocol/JSONProtocol.h \
	protocol/JSONProtocol.tcc \
	protocol/JSONProtocolCommon.h \
	protocol/JSONProtocolCommon.tcc \
	protocol/SimpleJSONProtocol.h \
	protocol/SimpleJSONProtocol.tcc \
	protocol/DebugProtocol.h \
	protocol/MessageSerializer.h \
	protocol/Serializer.h \
	protocol/VirtualProtocol.h

libthriftprotocol_la_SOURCES = Version.cpp \
                               ../cpp/Thrift.cpp \
                               ../cpp/concurrency/Util.cpp \
                               ../cpp/transport/TTransportException.cpp \
                               ../cpp/util/VarintUtils.cpp \
                               protocol/BinaryProtocol.cpp \
                               protocol/CompactProtocol.cpp \
                               protocol/CompactV1Protocol.cpp \
                               protocol/DebugProtocol.cpp \
                               protocol/JSONProtocol.cpp \
                               protocol/JSONProtocolCommon.cpp \
                               protocol/Serializer.cpp \
                               protocol/SimpleJSONProtocol.cpp \
                               protocol/VirtualProtocol.cpp

libthriftprotocol_la_LIBADD = $(FOLLY_LIBS) $(BOOST_SYSTEM_LIB) -ldouble-conversion

libthriftprotocol_la_LDFLAGS = -no-undefined \
                               -version-info $(LT_VERSION) \
                               $(BOOST_LDFLAGS)

libthriftprotocol_la_CPPFLAGS = $(AM_CPPFLAGS) -I../cpp

libthriftcpp2_la_SOURCES = Version.cpp \
			   GeneratedCodeHelper.cpp \
			   async/HeaderChannel.cpp \
			   async/HeaderChannelTrait.cpp \
			   async/HeaderClientChannel.cpp \
			   async/HeaderServerChannel.cpp \
			   async/HTTPClientChannel.cpp \
			   async/StubSaslClient.cpp \
			   async/StubSaslServer.cpp \
			   async/SaslEndpoint.cpp \
			   async/GssSaslClient.cpp \
			   async/GssSaslServer.cpp \
			   async/Cpp2Channel.cpp \
			   async/ResponseChannel.cpp \
			   async/ProtectionHandler.cpp \
			   async/FramingHandler.cpp \
			   async/SaslNegotiationHandler.cpp \
			   async/PcapLoggingHandler.cpp \
			   async/AsyncProcessor.cpp \
			   async/DuplexChannel.cpp \
			   security/KerberosSASLHandshakeClient.cpp \
			   security/KerberosSASLHandshakeServer.cpp \
			   security/KerberosSASLHandshakeUtils.cpp \
			   security/KerberosSASLThreadManager.cpp \
			   security/SecurityKillSwitch.cpp \
			   security/SecurityKillSwitchPoller.cpp \
			   server/BaseThriftServer.cpp \
			   server/Cpp2Connection.cpp \
			   server/Cpp2Worker.cpp \
			   server/ThriftServer.cpp \
			   server/TLSHelper.cpp \
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
			   ../cpp/util/kerberos/FBKrb5GetCreds.cpp

if WITH_NUMA
libthriftcpp2_la_SOURCES += ../cpp/concurrency/NumaThreadManager.cpp
endif

libthriftcpp2_la_LIBADD = \
	../cpp/libthrift.la \
	libthriftprotocol.la \
	libsaslstubs.a \
	$(FOLLY_LIBS) \
	-lkrb5 \
	-lgssapi_krb5 \
	-ldouble-conversion \
	$(BOOST_SYSTEM_LIB) \
	$(BOOST_THREAD_LIB)

if WITH_NUMA
libthriftcpp2_la_LIBADD += -lnuma
endif

libthriftcpp2_la_LDFLAGS = -version-info $(LT_VERSION) $(BOOST_LDFLAGS)
libthriftcpp2_la_CPPFLAGS = $(AM_CPPFLAGS) $(LIBEVENT_CPPFLAGS) -I../cpp -I$(top_builddir)/../../gperftools-2.0.99/src

THRIFT = $(top_builddir)/compiler/thrift1

BUILT_SOURCES = gen-cpp2/Sasl_constants.cpp \
		gen-cpp2/Sasl_data.cpp \
		gen-cpp2/Sasl_types.cpp \
		gen-cpp2/SaslAuthService.cpp \
		gen-cpp2/SaslAuthService_client.cpp

gen-cpp2/Sasl_data.h gen-cpp2/Sasl_types.h: Sasl.thrift
	PYTHONPATH=$(PY_LOCAL_PATH) $(PYTHON) -mthrift_compiler.main --gen cpp2 $<

gen-cpp2/Sasl_constants.cpp: gen-cpp2/Sasl_types.h

gen-cpp2/Sasl_data.cpp: gen-cpp2/Sasl_data.h gen-cpp2/Sasl_types.h

gen-cpp2/Sasl_types.cpp: gen-cpp2/Sasl_data.h gen-cpp2/Sasl_types.h

gen-cpp2/SaslAuthService.cpp: gen-cpp2/Sasl_types.h

libsaslstubs_a_SOURCES = gen-cpp2/Sasl_constants.cpp \
			 gen-cpp2/Sasl_data.cpp \
			 gen-cpp2/Sasl_types.cpp \
			 gen-cpp2/SaslAuthService.cpp \
		         gen-cpp2/SaslAuthService_client.cpp

libsaslstubs_a_CPPFLAGS = -fPIC $(AM_CPPFLAGS) $(LIBEVENT_CPPFLAGS) -I../cpp -I$(top_builddir)/../../gperftools-2.0.99/src

clean-local:
	rm -rf gen-cpp2/Sasl*
