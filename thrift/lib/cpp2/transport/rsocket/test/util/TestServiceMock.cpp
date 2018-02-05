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

namespace testutil {
namespace testservice {
apache::thrift::Stream<int32_t> TestServiceMock::range(
    int32_t from,
    int32_t to) {
  return yarpl::flowable::Flowable<>::range(from, to)->map(
      [](auto i) { return (int32_t)i; });
}

apache::thrift::Stream<int32_t> TestServiceMock::prefixSumIOThread(
    apache::thrift::Stream<int32_t> input) {
  // TODO: Flow control

  // As we just return the input as output and as the input is part of the IO
  // thread, the map operation will be performed also in the IO thread.
  int j = 0;
  return input->map([j](auto i) mutable {
    j = j + i;
    return j;
  });
}

apache::thrift::Stream<Message> TestServiceMock::returnNullptr() {
  return nullptr;
}

apache::thrift::Stream<Message> TestServiceMock::throwException(
    apache::thrift::Stream<Message>) {
  throw std::runtime_error("random error");
}

} // namespace testservice
} // namespace testutil
