/*
 * Copyright 2015-present Facebook, Inc.
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
#include <thrift/lib/cpp2/test/gen-cpp2/TestService.h>
#include <thrift/lib/cpp2/util/ScopedServerInterfaceThread.h>

#include <gtest/gtest.h>

using namespace std;
using namespace std::chrono;
using namespace folly;
using namespace apache::thrift;
using namespace apache::thrift::test;
using namespace apache::thrift::transport;

class ThriftClientTest : public testing::Test {};

TEST_F(ThriftClientTest, FutureCapturesChannel) {
  class Handler : public TestServiceSvIf {
  public:
    Future<unique_ptr<string>> future_sendResponse(int64_t size) override {
      return makeFuture(make_unique<string>(to<string>(size)));
    }
  };

  auto handler = make_shared<Handler>();
  ScopedServerInterfaceThread runner(handler);

  EventBase eb;
  auto client = runner.newClient<TestServiceAsyncClient>(&eb);
  auto fut = client->future_sendResponse(12);
  // To prove that even if the client is gone, the channel is captured:
  client = nullptr;
  auto ret = fut.waitVia(&eb).getTry();

  EXPECT_TRUE(ret.hasValue());
  EXPECT_EQ("12", ret.value());
}

TEST_F(ThriftClientTest, SemiFutureCapturesChannel) {
  class Handler : public TestServiceSvIf {
   public:
    void sendResponse(std::string& _return, int64_t size) override {
      _return = to<string>(size);
    }
  };

  auto handler = make_shared<Handler>();
  ScopedServerInterfaceThread runner(handler);

  EventBase eb;
  auto client = runner.newClient<TestServiceAsyncClient>(&eb);
  auto fut = client->semifuture_sendResponse(15).via(&eb).waitVia(&eb);

  // To prove that even if the client is gone, the channel is captured:
  client = nullptr;

  EXPECT_EQ("15", fut.value());
}

TEST_F(ThriftClientTest, FutureCapturesChannelOneway) {
  //  Generated SvIf handler methods throw. We check Try::hasValue().
  //  So this is a sanity check that the method is oneway.
  auto handler = make_shared<TestServiceSvIf>();
  ScopedServerInterfaceThread runner(handler);

  EventBase eb;
  auto client = runner.newClient<TestServiceAsyncClient>(&eb);
  auto fut = client->future_noResponse(12);
  // To prove that even if the client is gone, the channel is captured:
  client = nullptr;
  auto ret = fut.waitVia(&eb).getTry();

  EXPECT_TRUE(ret.hasValue());
}

TEST_F(ThriftClientTest, SemiFutureCapturesChannelOneway) {
  // Ditto previous test but with the SemiFuture<T> methods
  auto handler = make_shared<TestServiceSvIf>();
  ScopedServerInterfaceThread runner(handler);

  EventBase eb;
  auto client = runner.newClient<TestServiceAsyncClient>(&eb);
  auto fut = client->semifuture_noResponse(12).via(&eb).waitVia(&eb);
  // To prove that even if the client is gone, the channel is captured:
  client = nullptr;

  EXPECT_TRUE(fut.hasValue());
}

TEST_F(ThriftClientTest, SyncRpcOptionsTimeout) {
  class DelayHandler : public TestServiceSvIf {
  public:
    DelayHandler(milliseconds delay) : delay_(delay) {}
    void async_eb_eventBaseAsync(
        unique_ptr<HandlerCallback<unique_ptr<string>>> cb) override {
      auto eb = cb->getEventBase();
      eb->runAfterDelay([cb = move(cb)] {
        cb->result("hello world");
      }, delay_.count());
    }
  private:
    milliseconds delay_;
  };

  //  rpcTimeout << handlerDelay << channelTimeout.
  constexpr auto handlerDelay = milliseconds(10);
  constexpr auto channelTimeout = duration_cast<milliseconds>(seconds(10));
  constexpr auto rpcTimeout = milliseconds(1);

  //  Handler has medium 10ms delay.
  auto handler = make_shared<DelayHandler>(handlerDelay);
  ScopedServerInterfaceThread runner(handler);

  EventBase eb;
  auto client = runner.newClient<TestServiceAsyncClient>(&eb);
  auto channel = dynamic_cast<HeaderClientChannel*>(client->getChannel());
  ASSERT_NE(nullptr, channel);
  channel->setTimeout(channelTimeout.count());

  auto start = steady_clock::now();
  try {
    RpcOptions options;
    options.setTimeout(rpcTimeout);
    std::string response;
    client->sync_eventBaseAsync(options, response);
    ADD_FAILURE() << "should have timed out";
  } catch (const TTransportException& e) {
    auto expected = TTransportException::TIMED_OUT;
    EXPECT_EQ(expected, e.getType());
  }

  auto dur = steady_clock::now() - start;
  EXPECT_EQ(channelTimeout.count(), channel->getTimeout());
  EXPECT_GE(dur, rpcTimeout);
  EXPECT_LT(dur, channelTimeout);
}
