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

#include <thrift/lib/cpp2/async/Sink.h>
#include <thrift/lib/cpp2/async/tests/util/gen-cpp2/TestSinkService.h>

namespace testutil {
namespace testservice {

class TestSinkService : public TestSinkServiceSvIf {
 public:
  apache::thrift::SinkConsumer<int32_t, bool> range(int32_t from, int32_t to)
      override;

  apache::thrift::SinkConsumer<int32_t, bool> rangeThrow(
      int32_t from,
      int32_t to) override;

  apache::thrift::SinkConsumer<int32_t, bool> rangeFinalResponseThrow(
      int32_t from,
      int32_t to) override;

  apache::thrift::SinkConsumer<int32_t, int32_t>
  rangeEarlyResponse(int32_t from, int32_t to, int32_t early) override;

  apache::thrift::SinkConsumer<int32_t, bool> unSubscribedSink() override;

  bool isSinkUnSubscribed() override;

 private:
  bool sinkUnsubscribed_{false};
};

} // namespace testservice
} // namespace testutil
