/*
 * Copyright 2015 Facebook, Inc.
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
using namespace folly;
using namespace apache::thrift;
using namespace apache::thrift::test::cpp2;

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
