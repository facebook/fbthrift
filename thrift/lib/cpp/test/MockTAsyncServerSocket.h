/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
#ifndef THRIFT_TEST_MOCKTASYNCSERVERSOCKET_H_
#define THRIFT_TEST_MOCKTASYNCSERVERSOCKET_H_ 1

#include <gmock/gmock.h>

#include <thrift/lib/cpp/async/TAsyncServerSocket.h>
#include <thrift/lib/cpp/async/TEventBase.h>

namespace apache {
namespace thrift {

namespace test {

class MockTAsyncServerSocket :
  public apache::thrift::async::TAsyncServerSocket {
public:
  typedef std::unique_ptr<MockTAsyncServerSocket, Destructor> UniquePtr;

  // We explicitly do not mock destroy(), since the base class implementation
  // in TDelayedDestruction is what actually deletes the object.
  //MOCK_METHOD0(destroy,
  //             void());
  MOCK_METHOD1(bind,
               void(const apache::thrift::transport::TSocketAddress& address));
  MOCK_METHOD1(bind,
               void(uint16_t port));
  MOCK_METHOD1(listen,
               void(int backlog));
  MOCK_METHOD0(startAccepting,
               void());
  MOCK_METHOD3(addAcceptCallback,
               void(AcceptCallback *callback,
                    apache::thrift::async::TEventBase *eventBase,
                    uint32_t maxAtOnce));
};

}}} // apache::thrift::test

#endif // THRIFT_TEST_MOCKTASYNCSERVERSOCKET_H_
