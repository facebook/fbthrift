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

#pragma once

#include <gmock/gmock.h>
#include <atomic>
#include <thrift/lib/cpp2/transport/core/testutil/gen-cpp2/TestService.tcc>

namespace testutil {
namespace testservice {

class TestServiceMock : public TestServiceSvIf {
 public:
  using EmptyArgs = apache::thrift::ThriftPresult<false>;
  using EmptyResult = apache::thrift::ThriftPresult<true>;

  MOCK_METHOD2(sumTwoNumbers_, int32_t(int32_t, int32_t));
  MOCK_METHOD1(add_, int32_t(int32_t));
  MOCK_METHOD2(addAfterDelay_, void(int32_t, int32_t));

  int32_t sumTwoNumbers(int32_t x, int32_t y) override;

  int32_t add(int32_t x) override;

  void addAfterDelay(int32_t delayMs, int32_t x) override;

  void throwExpectedException(int32_t x) override;

  void throwUnexpectedException(int32_t x) override;

  void sleep(int32_t timeMs) override;

  void headers() override;

 protected:
  std::atomic<int32_t> sum{0};
};

} // namespace testservice
} // namespace testutil
