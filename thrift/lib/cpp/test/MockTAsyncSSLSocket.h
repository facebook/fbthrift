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

#include "thrift/lib/cpp/async/TAsyncSSLSocket.h"

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
   void(TAsyncSocket::ConnectCallback*,
    const apache::thrift::transport::TSocketAddress&,
    int,
    const OptionMap&,
    const apache::thrift::transport::TSocketAddress&));
  MOCK_CONST_METHOD1(
   getPeerAddress,
   void(apache::thrift::transport::TSocketAddress*));
  MOCK_METHOD0(closeNow, void());
  MOCK_CONST_METHOD0(good, bool());
  MOCK_CONST_METHOD0(readable, bool());
  MOCK_CONST_METHOD0(hangup, bool());
  MOCK_METHOD3(
   sslConnect,
   void(TAsyncSSLSocket::HandshakeCallback*, uint64_t, bool));
};

}}}

#endif
