/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
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

#include <thrift/lib/cpp2/util/Checksum.h>

#include <folly/hash/Checksum.h>
#include <thrift/lib/cpp2/IOBufChain.h>

namespace apache::thrift::checksum {

namespace {

template <typename Function>
void forEachRange(const folly::IOBuf& payload, Function function) {
  for (auto range : payload) {
    function(range);
  }
}

template <typename Function>
void forEachRange(const IOBufChain& payload, Function function) {
  for (const auto& buffer : payload) {
    function(folly::ByteRange{buffer.data(), buffer.length()});
  }
}

template <typename Buffer>
uint32_t crc32cImpl(const Buffer& payload, size_t skipOffset) {
  uint32_t checksum = ~0U;
  forEachRange(payload, [&](folly::ByteRange range) {
    if (skipOffset >= range.size()) {
      skipOffset -= range.size();
      return;
    }
    checksum = folly::crc32c(
        range.data() + skipOffset, range.size() - skipOffset, checksum);
    skipOffset = 0;
  });
  return checksum;
}

} // namespace

uint32_t crc32c(const folly::IOBuf& payload, size_t skipOffset) {
  return crc32cImpl(payload, skipOffset);
}

uint32_t crc32c(const IOBufChain& payload, size_t skipOffset) {
  return crc32cImpl(payload, skipOffset);
}

} // namespace apache::thrift::checksum
