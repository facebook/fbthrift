/*
 * Copyright 2014 Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#ifndef THRIFT_TEST_MOCKTASYNCSSLSOCKET_H_
#define THRIFT_TEST_MOCKTASYNCSSLSOCKET_H_ 1

#include <gmock/gmock.h>

#include <thrift/lib/cpp/async/TAsyncSSLSocket.h>

namespace apache { namespace thrift { namespace test {

class MockTAsyncSSLSocket :
 public apache::thrift::async::TAsyncSSLSocket {
 public:
  MockTAsyncSSLSocket(
   const std::shared_ptr<transport::SSLContext>& ctx,
   async::TEventBase* base) :
    TAsyncSSLSocket(ctx, base) {
  }

  GMOCK_METHOD5_(, noexcept, ,
   connect,
   void(AsyncSocket::ConnectCallback*,
    const folly::SocketAddress&,
    int,
    const OptionMap&,
    const folly::SocketAddress&));
  MOCK_CONST_METHOD1(getLocalAddress, void(folly::SocketAddress*));
  MOCK_CONST_METHOD1(getPeerAddress, void(folly::SocketAddress*));
  MOCK_METHOD0(closeNow, void());
  MOCK_CONST_METHOD0(good, bool());
  MOCK_CONST_METHOD0(readable, bool());
  MOCK_CONST_METHOD0(hangup, bool());
  MOCK_CONST_METHOD2(
   getSelectedNextProtocol,
   void(const unsigned char**, unsigned*));
  MOCK_CONST_METHOD2(
   getSelectedNextProtocolNoThrow,
   bool(const unsigned char**, unsigned*));

  void sslConnect(
    TAsyncSSLSocket::HandshakeCallback* cb,
    uint64_t timeout,
    const apache::thrift::transport::SSLContext::SSLVerifyPeerEnum& verify)
      override {
    if (timeout > 0) {
      handshakeTimeout_.scheduleTimeout((uint32_t)timeout);
    }

    state_ = StateEnum::ESTABLISHED;
    sslState_ = STATE_CONNECTING;
    handshakeCallback_ = cb;

    sslConnectMockable(cb, timeout, verify);
  }
  MOCK_METHOD3(
   sslConnectMockable,
   void(TAsyncSSLSocket::HandshakeCallback*, uint64_t,
     const apache::thrift::transport::SSLContext::SSLVerifyPeerEnum&));
};

}}}

#endif
