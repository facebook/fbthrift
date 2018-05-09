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

#include <thrift/lib/cpp2/test/frozen2/support/client/gen-cpp2/WithSupportService.h>
#include <thrift/lib/cpp2/test/frozen2/support/server/gen-cpp2/NoSupportService.h>

#include <folly/io/async/EventBase.h>

#include <gtest/gtest.h>

using namespace ::apache::thrift;
using namespace ::apache::thrift::async;
using namespace ::apache::thrift::util;
using namespace ::apache::thrift::protocol;
using namespace ::test::frozen2;

class NoSupportServiceHandler : public NoSupportServiceSvIf {
 public:
  void stub() {}
};

TEST(Frozen2ServerTest, TestServerNoSupport) {
  folly::EventBase eb;
  TestThriftServerFactory<NoSupportServiceHandler> factory;
  ScopedServerThread sst(factory.create());
  auto socket = TAsyncSocket::newSocket(&eb, *sst.getAddress());
  auto channel = HeaderClientChannel::newChannel(socket);
  channel->setProtocolId(PROTOCOL_TYPES::T_FROZEN2_PROTOCOL);
  WithSupportServiceAsyncClient client(std::move(channel));

  try {
    client.sync_stub();
    FAIL() << "Server should send exception";
  } catch (TApplicationException& e) {
    ASSERT_EQ(
        e.getType(),
        TApplicationException::TApplicationExceptionType::INVALID_PROTOCOL);
  }
}
