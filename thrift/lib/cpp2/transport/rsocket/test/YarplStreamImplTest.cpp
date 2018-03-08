/*
 * Copyright 2018-present Facebook, Inc.
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

#include <thrift/lib/cpp2/transport/rsocket/YarplStreamImpl.h>

#include <gmock/gmock.h>

#include "yarpl/flowable/TestSubscriber.h"

namespace apache {
namespace thrift {
namespace {
/// Construct a pipeline with a test subscriber against the supplied
/// flowable.  Return the items that were sent to the subscriber.  If some
/// exception was sent, the exception is thrown.
template <typename T>
std::vector<T> run(
    std::shared_ptr<yarpl::flowable::Flowable<T>> flowable,
    int64_t requestCount = 100) {
  auto subscriber =
      std::make_shared<yarpl::flowable::TestSubscriber<T>>(requestCount);
  flowable->subscribe(subscriber);
  subscriber->awaitTerminalEvent(std::chrono::seconds(1));
  return std::move(subscriber->values());
}
} // namespace

TEST(YarplStreamImplTest, Basic) {
  auto flowable = yarpl::flowable::Flowable<>::justN({12, 34, 56, 98});
  auto stream = toStream(std::move(flowable));
  auto stream2 = std::move(stream).map<int>([](int x) { return x * 2; });
  auto flowable2 = toFlowable(std::move(stream2));

  EXPECT_EQ(run(flowable2), std::vector<int>({12 * 2, 34 * 2, 56 * 2, 98 * 2}));
}

} // namespace thrift
} // namespace apache
