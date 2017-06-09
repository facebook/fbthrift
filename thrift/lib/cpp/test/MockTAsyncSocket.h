/*
 * Copyright 2014-present Facebook, Inc.
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
#ifndef THRIFT_TEST_MOCKTASYNCSOCKET_H_
#define THRIFT_TEST_MOCKTASYNCSOCKET_H_ 1


#include <gmock/gmock.h>

#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <folly/io/async/EventBase.h>

namespace apache {
namespace thrift {

namespace test {

class MockTAsyncSocket : public apache::thrift::async::TAsyncSocket {
 public:
  typedef std::unique_ptr<MockTAsyncSocket, Destructor> UniquePtr;

  explicit MockTAsyncSocket(folly::EventBase* base) : TAsyncSocket(base) {
    attachEventBase(base);
  }

  static MockTAsyncSocket::UniquePtr newSocket(folly::EventBase* base) {
    return MockTAsyncSocket::UniquePtr(new MockTAsyncSocket(base));
  }

  GMOCK_METHOD5_(,
                 noexcept,
                 ,
                 connectInternal,
                 void(AsyncSocket::ConnectCallback*,
                      const folly::SocketAddress&,
                      int,
                      const OptionMap&,
                      const folly::SocketAddress&));

  void connect(
      AsyncSocket::ConnectCallback* callback,
      const folly::SocketAddress& addr,
      int timeout = 0,
      const OptionMap& options = emptyOptionMap,
      const folly::SocketAddress& bindAddr = anyAddress()) noexcept override {
    connectInternal(callback, addr, timeout, options, bindAddr);
  }

  MOCK_CONST_METHOD1(getPeerAddress,
                     void(folly::SocketAddress*));
  MOCK_METHOD0(detachFd, int());
  MOCK_CONST_METHOD0(getFd, int());
  MOCK_METHOD0(closeNow, void());
  MOCK_CONST_METHOD0(good, bool());
  MOCK_CONST_METHOD0(readable, bool());
  MOCK_CONST_METHOD0(hangup, bool());
  MOCK_METHOD1(setReadCB, void(AsyncTransportWrapper::ReadCallback*));
  MOCK_METHOD3(
      writeChain,
      void(
          AsyncTransportWrapper::WriteCallback*,
          std::unique_ptr<folly::IOBuf>&& buf,
          apache::thrift::async::WriteFlags));
};

}}}

#endif
