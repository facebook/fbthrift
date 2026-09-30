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

#include <folly/ThreadLocal.h>
#include <folly/hash/Checksum.h>
#include <thrift/lib/cpp2/transport/rocket/ChecksumGenerator.h>

namespace apache::thrift::rocket::detail {

namespace {

template <typename Function>
void forEachRange(folly::IOBuf& buffer, Function function) {
  for (auto range : buffer) {
    function(range);
  }
}

template <typename Function>
void forEachRange(IOBufChain& chain, Function function) {
  for (const auto& buffer : chain) {
    function(folly::ByteRange{buffer.data(), buffer.length()});
  }
}

size_t countElements(const folly::IOBuf& buffer) {
  return buffer.countChainElements();
}

size_t countElements(const IOBufChain& chain) {
  return chain.chainElements();
}

template <typename Buffer>
int64_t xxh3(XXH3_state_t* state, Buffer& buffer, int64_t salt) {
  if (XXH3_64bits_reset_withSeed(state, salt) == XXH_ERROR) {
    throw std::runtime_error("XXH64_reset failed");
  }

  forEachRange(buffer, [&](folly::ByteRange bytes) {
    if (XXH3_64bits_update(state, bytes.data(), bytes.size()) == XXH_ERROR) {
      throw std::runtime_error("XXH64_update failed");
    }
  });

  auto result = XXH3_64bits_digest(state);
  if (result == XXH_ERROR) {
    throw std::runtime_error("XXH64_digest failed");
  }
  return result;
}

template <typename Buffer>
int64_t crc32c(Buffer& buffer, int64_t salt) {
  auto checksum = static_cast<uint32_t>(salt);
  size_t count = 0;
  forEachRange(buffer, [&](folly::ByteRange bytes) {
    ++count;
    checksum = folly::crc32c(bytes.data(), bytes.size(), checksum);
  });
  FOLLY_SAFE_DCHECK(count == countElements(buffer));
  return checksum;
}

} // namespace

struct Destory {
  void operator()(XXH3_state_t* state) {
    if (state != nullptr) {
      XXH3_freeState(state);
    }
  }
};

using XXH3StatePtr = std::unique_ptr<XXH3_state_t, Destory>;

static folly::ThreadLocal<XXH3StatePtr> xxh3State_ =
    folly::ThreadLocal<XXH3StatePtr>(
        []() { return XXH3StatePtr(XXH3_createState(), Destory{}); });

XXH3_state_t* XXH3Logic::getXXH3State() {
  return xxh3State_->get();
}

int64_t XXH3Logic::xxh3(folly::IOBuf& buffer, int64_t salt) {
  return detail::xxh3(getXXH3State(), buffer, salt);
}

int64_t XXH3Logic::xxh3(IOBufChain& buffer, int64_t salt) {
  return detail::xxh3(getXXH3State(), buffer, salt);
}

int64_t CRC32Logic::crc32c(folly::IOBuf& buffer, int64_t salt) {
  return detail::crc32c(buffer, salt);
}

int64_t CRC32Logic::crc32c(IOBufChain& buffer, int64_t salt) {
  return detail::crc32c(buffer, salt);
}

} // namespace apache::thrift::rocket::detail
