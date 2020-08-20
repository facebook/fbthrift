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

#include <thrift/conformance/cpp2/AnySerializer.h>

namespace apache::thrift::conformance {

std::string AnySerializer::encode(any_ref value) const {
  folly::IOBufQueue queue(folly::IOBufQueue::cacheChainLength());
  // Allocate 16KB at a time; leave some room for the IOBuf overhead
  constexpr size_t kDesiredGrowth = (1 << 14) - 64;
  encode(value, folly::io::QueueAppender(&queue, kDesiredGrowth));
  std::string out;
  queue.appendToString(out);
  return out;
}

void AnySerializer::decode(
    const std::type_info& typeInfo,
    std::string_view data,
    any_ref value) const {
  folly::IOBuf buf(folly::IOBuf::WRAP_BUFFER, data.data(), data.size());
  folly::io::Cursor cursor{&buf};
  decode(typeInfo, cursor, value);
}

std::any AnySerializer::decode(
    const std::type_info& typeInfo,
    folly::io::Cursor& cursor) const {
  std::any result;
  decode(typeInfo, cursor, result);
  return result;
}

std::any AnySerializer::decode(
    const std::type_info& typeInfo,
    std::string_view data) const {
  folly::IOBuf buf(folly::IOBuf::WRAP_BUFFER, data.data(), data.size());
  folly::io::Cursor cursor{&buf};
  return decode(typeInfo, cursor);
}

void AnySerializer::checkType(
    const std::type_info& actual,
    const std::type_info& expected) {
  if (actual != expected) {
    folly::throw_exception(std::bad_any_cast());
  }
}

} // namespace apache::thrift::conformance
