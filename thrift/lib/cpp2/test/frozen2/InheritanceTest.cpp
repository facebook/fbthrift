/*
 * Copyright 2004-present Facebook, Inc.
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

#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include <thrift/lib/cpp2/test/util/TestThriftServerFactory.h>
#include <thrift/lib/cpp2/util/ScopedServerThread.h>

#include <thrift/lib/cpp2/test/frozen2/inheritance/gen-cpp2/InheritanceNoSupportService.h>
#include <thrift/lib/cpp2/test/frozen2/inheritance/gen-cpp2/InheritanceWithSupportService.h>

#include <gtest/gtest.h>

using namespace ::apache::thrift;
using namespace ::apache::thrift::async;
using namespace ::apache::thrift::util;
using namespace ::apache::thrift::protocol;
using namespace ::test::frozen2;

template <typename T>
std::unique_ptr<T> makeClient(
    folly::EventBase* eb,
    const folly::SocketAddress* addr) {
  auto socket = TAsyncSocket::newSocket(eb, *addr);
  auto channel = HeaderClientChannel::newChannel(socket);
  channel->setProtocolId(PROTOCOL_TYPES::T_FROZEN2_PROTOCOL);
  return std::make_unique<T>(std::move(channel));
}

class InheritanceNoSupportServiceHandler
    : public InheritanceNoSupportServiceSvIf {
 public:
  void stub() override {}
};

class InheritanceWithSupportServiceHandler
    : public InheritanceWithSupportServiceSvIf {
 public:
  void stub() override {}
};

TEST(InheritanceTest, TestHasSupport) {
  folly::EventBase eb;
  TestThriftServerFactory<InheritanceNoSupportServiceHandler> factory;
  ScopedServerThread sst(factory.create());

  auto withSupportClient = makeClient<InheritanceWithSupportServiceAsyncClient>(
      &eb, sst.getAddress());
  try {
    withSupportClient->sync_stub();
    FAIL() << "Server should send exception";
  } catch (TApplicationException& e) {
    ASSERT_EQ(
        e.getType(),
        TApplicationException::TApplicationExceptionType::INVALID_PROTOCOL);
  }
}

TEST(InheritanceTest, TestNoSupport) {
  folly::EventBase eb;
  TestThriftServerFactory<InheritanceWithSupportServiceHandler> factory;
  ScopedServerThread sst(factory.create());

  auto wtihSupportClient = makeClient<InheritanceWithSupportServiceAsyncClient>(
      &eb, sst.getAddress());
  wtihSupportClient->sync_stub();
}
