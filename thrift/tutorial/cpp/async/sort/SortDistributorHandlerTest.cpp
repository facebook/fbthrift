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

#include <random>
#include <folly/gen/Base.h>
#include <thrift/lib/cpp2/util/ScopedServerInterfaceThread.h>

#include <thrift/tutorial/cpp/async/sort/SortServerHandler.h>
#include <thrift/tutorial/cpp/async/sort/SortDistributorHandler.h>

#include <gtest/gtest.h>

using namespace std;
using namespace folly;
using namespace apache::thrift;
using namespace apache::thrift::tutorial::sort;

class SortDistributorHandlerTest : public testing::Test {};

TEST_F(SortDistributorHandlerTest, example) {
  auto backend_runners = gen::range<size_t>(0, 4)
    | gen::map([](size_t) {
        auto handler = make_shared<SortServerHandler>();
        return make_unique<ScopedServerInterfaceThread>(handler);
      })
    | gen::as<vector>();

  auto backends = gen::from(backend_runners)
    | gen::map([](const unique_ptr<ScopedServerInterfaceThread>& runner) {
        return runner->getAddress();
      })
    | gen::as<vector>();

  auto handler = make_shared<SortDistributorHandler>(move(backends));
  ScopedServerInterfaceThread runner(handler);

  EventBase eb;
  auto client = runner.newClient<SorterAsyncClient>(eb);

  default_random_engine rng;
  uniform_int_distribution<int> dist(0, 1<<20);
  auto input = gen::range<size_t>(0, 1<<12)
    | gen::map([&](size_t) { return dist(rng); })
    | gen::as<vector>();

  auto expected = gen::from(input) | gen::order | gen::as<vector>();
  auto actual = client->future_sort(input).waitVia(&eb).get();
  EXPECT_EQ(expected, actual);
}
