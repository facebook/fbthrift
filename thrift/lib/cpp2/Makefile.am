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

libthriftcpp2_la_SOURCES = async/HeaderClientChannel.cpp \
			   async/StubSaslClient.cpp \
			   async/StubSaslServer.cpp \
			   async/GssSaslClient.cpp \
			   async/GssSaslServer.cpp \
			   async/Cpp2Channel.cpp \
			   async/AsyncProcessor.cpp \
			   async/Stream.cpp \
			   async/InputStream.cpp \
			   async/OutputStream.cpp \
			   protocol/StreamSerializers.cpp \
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
			   ../cpp/async/TAsyncSocket.cpp \
			   ../cpp/async/TAsyncSSLSocket.cpp \
			   ../cpp/EventHandlerBase.cpp \
			   ../cpp/transport/THeader.cpp \
			   ../cpp/transport/THeaderTransport.cpp \
			   ../cpp/transport/TSocketAddress.cpp \
			   ../cpp/util/ScopedServerThread.cpp \
			   ../cpp/util/SocketRetriever.cpp \
			   ../cpp/async/TAsyncServerSocket.cpp \
			   ../cpp/async/HHWheelTimer.cpp \
			   ../cpp/util/VarintUtils.cpp \
			   ../cpp/async/TAsyncTimeoutSet.cpp \
			   ../cpp/async/TEventBaseManager.cpp \
			   ../cpp/util/FdUtils.cpp \
			   ../cpp/util/THttpParser.cpp \
			   ../cpp/util/kerberos/Krb5Util.cpp \
			   ../cpp/util/kerberos/Krb5CredentialsCacheManager.cpp


libthriftcpp2_la_LIBADD = ../cpp/libthrift.la libsaslstubs.a -lkrb5 -lgssapi_krb5
libthriftcpp2_la_CPPFLAGS = $(AM_CPPFLAGS) $(LIBEVENT_CPPFLAGS) -I../cpp -I$(top_builddir)/../../gperftools-2.0.99/src

THRIFT = $(top_builddir)/compiler/thrift1

gen-cpp/Sasl_types.h: Sasl.thrift
	$(THRIFT) --gen cpp:templates,cob_style -r $<

gen-cpp2/Sasl_types.h: Sasl.thrift
	PYTHONPATH=$(PYPATH) python $(top_builddir)/compiler/main.py --gen cpp2 $<

libsaslstubs_a_SOURCES = gen-cpp2/Sasl_constants.cpp \
			 gen-cpp2/Sasl_types.cpp \
			 gen-cpp2/SaslAuthService.cpp

libsaslstubs_a_CPPFLAGS = -fPIC $(AM_CPPFLAGS) $(LIBEVENT_CPPFLAGS) -I../cpp -I$(top_builddir)/../../gperftools-2.0.99/src

PYPATH = $(top_builddir)/compiler/py/build/lib.linux-x86_64-2.7

clean:
	rm -rf gen-cpp2 gen-cpp
