/*
 * Copyright 2017-present Facebook, Inc.
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

#include <thrift/lib/cpp2/transport/rsocket/test/util/TestServiceMock.h>

#include <thrift/lib/cpp2/transport/rsocket/YarplStreamImpl.h>

namespace testutil {
namespace testservice {

using namespace apache::thrift;
using namespace yarpl::flowable;

class LeakDetector {
 public:
  class InternalClass {};

  LeakDetector()
      : internal_(std::make_shared<testing::StrictMock<InternalClass>>()) {
    ++instanceCount();
  }

  LeakDetector(const LeakDetector& oth) : internal_(oth.internal_) {
    ++instanceCount();
  }

  LeakDetector& operator=(const LeakDetector& oth) {
    internal_ = oth.internal_;
    return *this;
  }

  virtual ~LeakDetector() {
    --instanceCount();
  }

  std::shared_ptr<testing::StrictMock<InternalClass>> internal_;

  static int32_t getInstanceCount() {
    return instanceCount();
  }

 protected:
  static std::atomic_int& instanceCount() {
    static std::atomic_int instanceCount{0};
    return instanceCount;
  }
};

Stream<int32_t> TestServiceMock::range(int32_t from, int32_t to) {
  return toStream(
      Flowable<>::range(from, to)->map([](auto i) { return (int32_t)i; }),
      &executor_);
}

ResponseAndStream<int32_t, int32_t> TestServiceMock::leakCheck(
    int32_t from,
    int32_t to) {
  return {LeakDetector::getInstanceCount(),
          toStream(
              Flowable<>::range(from, to)->map(
                  [detector = LeakDetector()](auto i) { return (int32_t)i; }),
              &executor_)};
}

int32_t TestServiceMock::instanceCount() {
  return LeakDetector::getInstanceCount();
}

Stream<Message> TestServiceMock::returnNullptr() {
  return {};
}

ResponseAndStream<int, Message> TestServiceMock::throwError() {
  throw Error();
}

apache::thrift::ResponseAndStream<int32_t, int32_t>
TestServiceMock::sleepWithResponse(int32_t timeMs) {
  /* sleep override */
  std::this_thread::sleep_for(std::chrono::milliseconds(timeMs));
  return {1,
          toStream(
              Flowable<>::range(1, 1)->map([](auto i) { return (int32_t)i; }),
              &executor_)};
}

apache::thrift::Stream<int32_t> TestServiceMock::sleepWithoutResponse(
    int32_t timeMs) {
  return std::move(sleepWithResponse(timeMs).stream);
}

} // namespace testservice
} // namespace testutil
