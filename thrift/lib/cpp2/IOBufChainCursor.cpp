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

#include <thrift/lib/cpp2/IOBufChainCursor.h>

#include <algorithm>
#include <cstring>
#include <stdexcept>

namespace apache::thrift::io {

IOBufChainCursor::IOBufChainCursor(
    const apache::thrift::IOBufChain& chain) noexcept {
  reset(&chain);
}

void IOBufChainCursor::reset(const apache::thrift::IOBufChain* chain) noexcept {
  chain_ = chain;
  current_ =
      chain ? chain->begin() : apache::thrift::IOBufChain::const_iterator{};
  offset_ = 0;
  position_ = 0;
  advancePastEmptyBuffers();
}

folly::ByteRange IOBufChainCursor::peekBytes() noexcept {
  advancePastEmptyBuffers();
  return {data(), length()};
}

size_t IOBufChainCursor::pullAtMostSlow(void* destination, size_t length) {
  const auto toPull = std::min(length, totalLength() - position_);
  pullSlow(destination, toPull);
  return toPull;
}

void IOBufChainCursor::pullSlow(void* destination, size_t length) {
  if (!canAdvance(length)) {
    throw std::out_of_range("IOBufChain cursor read exceeds chain length");
  }

  auto* output = static_cast<uint8_t*>(destination);
  while (length > 0) {
    const auto& buffer = *current_;
    const auto available = buffer.length() - offset_;
    const auto toCopy = std::min(length, available);
    std::memcpy(output, buffer.data() + offset_, toCopy);
    output += toCopy;
    length -= toCopy;
    offset_ += toCopy;
    position_ += toCopy;
    advancePastEmptyBuffers();
  }
}

void IOBufChainCursor::skipSlow(size_t length) {
  if (!canAdvance(length)) {
    throw std::out_of_range("IOBufChain cursor skip exceeds chain length");
  }

  while (length > 0) {
    const auto& buffer = *current_;
    const auto available = buffer.length() - offset_;
    const auto toSkip = std::min(length, available);
    length -= toSkip;
    offset_ += toSkip;
    position_ += toSkip;
    advancePastEmptyBuffers();
  }
}

std::string IOBufChainCursor::readFixedString(size_t length) {
  std::string result(length, '\0');
  pull(result.data(), length);
  return result;
}

void IOBufChainCursor::clone(
    apache::thrift::IOBufChain& destination,
    size_t length,
    folly::io::CloneOwnership ownership) {
  if (!canAdvance(length)) {
    throw std::out_of_range("IOBufChain cursor clone exceeds chain length");
  }

  auto probe = *this;
  size_t resultElements = 0;
  size_t probeRemaining = length;
  while (probeRemaining > 0) {
    const auto toSkip = std::min(probeRemaining, probe.length());
    probe.skip(toSkip);
    probeRemaining -= toSkip;
    ++resultElements;
  }

  apache::thrift::IOBufChain result{resultElements};
  size_t remaining = length;
  while (remaining > 0) {
    advancePastEmptyBuffers();
    const auto& source = *current_;
    const auto toClone = std::min(remaining, source.length() - offset_);
    result.appendClonedRange(
        source, offset_, toClone, ownership, resultElements);
    skip(toClone);
    remaining -= toClone;
  }
  destination = std::move(result);
}

void IOBufChainCursor::clone(
    std::unique_ptr<folly::IOBuf>& destination,
    size_t length,
    folly::io::CloneOwnership ownership) {
  apache::thrift::IOBufChain result;
  clone(result, length, ownership);
  destination = std::move(result).moveToIOBuf();
  if (!destination) {
    destination = std::make_unique<folly::IOBuf>();
  }
}

void IOBufChainCursor::clone(
    folly::IOBuf& destination,
    size_t length,
    folly::io::CloneOwnership ownership) {
  std::unique_ptr<folly::IOBuf> result;
  clone(result, length, ownership);
  destination = std::move(*result);
}

size_t IOBufChainCursor::operator-(
    const IOBufChainCursor& other) const noexcept {
  DCHECK_EQ(chain_, other.chain_);
  DCHECK_GE(position_, other.position_);
  return position_ - other.position_;
}

void IOBufChainCursor::advancePastEmptyBuffers() noexcept {
  while (chain_ && current_ != chain_->end()) {
    if (offset_ < current_->length()) {
      return;
    }
    ++current_;
    offset_ = 0;
  }
}

} // namespace apache::thrift::io
