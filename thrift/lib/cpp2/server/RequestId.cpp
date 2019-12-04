/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <thrift/lib/cpp2/server/RequestId.h>
#include <atomic>

namespace apache {
namespace thrift {

// Reserve some high bits for future use. Currently the maximum id supported
// is 10^52, so thrift servers theoretically can generate unique request id
// for ~12 years, assuming the QPS is ~10 million.
const uint64_t RequestId::maxVal = 0x000fffffffffffff;

namespace {
std::atomic<uint64_t> nextId{0};
}

RequestId RequestId::gen() {
  return RequestId(nextId++ & maxVal);
}

} // namespace thrift
} // namespace apache
