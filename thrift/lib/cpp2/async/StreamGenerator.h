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

#pragma once

#include <thrift/lib/cpp2/async/Stream.h>

namespace apache {
namespace thrift {

class StreamGenerator {
 public:
  template <
      typename Generator,
      typename Element =
          typename folly::invoke_result_t<std::decay_t<Generator>&>::value_type>
  static Stream<Element> create(
      folly::Executor::KeepAlive<folly::SequencedExecutor> executor,
      Generator&& generator);

 private:
  StreamGenerator() = default;
};

} // namespace thrift
} // namespace apache

#include <thrift/lib/cpp2/async/StreamGenerator-inl.h>
