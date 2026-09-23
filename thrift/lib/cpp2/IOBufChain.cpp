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

#include <thrift/lib/cpp2/IOBufChain.h>

#include <new>
#include <stdexcept>
#include <utility>

#include <folly/io/Cursor.h>
#include <folly/lang/CheckedMath.h>
#include <folly/lang/Exception.h>

namespace apache::thrift {
namespace {

std::strong_ordering compareChainData(
    const IOBufChain& left, const IOBufChain& right) noexcept {
  auto leftIt = left.begin();
  auto rightIt = right.begin();
  const auto leftEnd = left.end();
  const auto rightEnd = right.end();
  size_t leftOffset = 0;
  size_t rightOffset = 0;

  while (leftIt != leftEnd && rightIt != rightEnd) {
    const auto length = std::min(
        leftIt->length() - leftOffset, rightIt->length() - rightOffset);
    if (length != 0) {
      const auto comparison =
          folly::ByteRange{leftIt->data() + leftOffset, length}.compare(
              folly::ByteRange{rightIt->data() + rightOffset, length}) <=> 0;
      if (comparison != std::strong_ordering::equal) {
        return comparison;
      }
      leftOffset += length;
      rightOffset += length;
    }
    if (leftOffset == leftIt->length()) {
      ++leftIt;
      leftOffset = 0;
    }
    if (rightOffset == rightIt->length()) {
      ++rightIt;
      rightOffset = 0;
    }
  }
  return left.chainLength() <=> right.chainLength();
}

} // namespace

struct alignas(folly::IOBuf) IOBufChain::Block {
  static Block* create(size_t capacity);
  static void destroy(Block* block) noexcept;
  static void destroyChain(Block* block) noexcept;

  explicit Block(size_t slotCapacity) noexcept : capacity{slotCapacity} {}

  ~Block() { clear(); }

  Block(const Block&) = delete;
  Block& operator=(const Block&) = delete;
  Block(Block&&) = delete;
  Block& operator=(Block&&) = delete;

  folly::IOBuf* rawSlot(size_t index) noexcept;
  const folly::IOBuf* rawSlot(size_t index) const noexcept;
  folly::IOBuf& at(size_t index) noexcept;
  const folly::IOBuf& at(size_t index) const noexcept;

  bool empty() const noexcept { return beginIndex == endIndex; }
  bool full() const noexcept { return endIndex == capacity; }
  void pop(folly::IOBuf& result) noexcept;
  void clear() noexcept;

  Block* next{nullptr};
  size_t beginIndex{0};
  size_t endIndex{0};
  size_t capacity{0};
};

IOBufChain::Block* IOBufChain::Block::create(size_t capacity) {
  if (capacity == 0) {
    folly::throw_exception<std::invalid_argument>(
        "IOBufChain block capacity is zero");
  }
  static_assert(alignof(Block) <= __STDCPP_DEFAULT_NEW_ALIGNMENT__);
  static_assert(sizeof(Block) % alignof(folly::IOBuf) == 0);
  size_t allocationSize;
  if (!folly::checked_muladd(
          &allocationSize, capacity, sizeof(folly::IOBuf), sizeof(Block))) {
    folly::throw_exception<std::bad_array_new_length>();
  }
  auto* allocation = ::operator new(allocationSize);
  return std::construct_at(static_cast<Block*>(allocation), capacity);
}

void IOBufChain::Block::destroy(Block* block) noexcept {
  std::destroy_at(block);
  ::operator delete(block);
}

void IOBufChain::Block::destroyChain(Block* block) noexcept {
  while (block) {
    auto* next = block->next;
    destroy(block);
    block = next;
  }
}

folly::IOBuf* IOBufChain::Block::rawSlot(size_t index) noexcept {
  DCHECK_LT(index, capacity);
  auto* storage = reinterpret_cast<std::byte*>(this + 1);
  return reinterpret_cast<folly::IOBuf*>(
      storage + index * sizeof(folly::IOBuf));
}

const folly::IOBuf* IOBufChain::Block::rawSlot(size_t index) const noexcept {
  DCHECK_LT(index, capacity);
  const auto* storage = reinterpret_cast<const std::byte*>(this + 1);
  return reinterpret_cast<const folly::IOBuf*>(
      storage + index * sizeof(folly::IOBuf));
}

folly::IOBuf& IOBufChain::Block::at(size_t index) noexcept {
  DCHECK_GE(index, beginIndex);
  DCHECK_LT(index, endIndex);
  return *rawSlot(index);
}

const folly::IOBuf& IOBufChain::Block::at(size_t index) const noexcept {
  DCHECK_GE(index, beginIndex);
  DCHECK_LT(index, endIndex);
  return *rawSlot(index);
}

void IOBufChain::Block::pop(folly::IOBuf& result) noexcept {
  DCHECK(!empty());
  result = std::move(at(beginIndex));
  std::destroy_at(std::addressof(at(beginIndex)));
  ++beginIndex;
}

void IOBufChain::Block::clear() noexcept {
  for (size_t i = beginIndex; i < endIndex; ++i) {
    std::destroy_at(std::addressof(at(i)));
  }
  beginIndex = 0;
  endIndex = 0;
}

IOBufChain::const_iterator::const_iterator(
    const Block* block, size_t slot) noexcept
    : block_(block), slot_(slot) {}

const folly::IOBuf& IOBufChain::const_iterator::dereference() const noexcept {
  return block_->at(slot_);
}

bool IOBufChain::const_iterator::equal(
    const const_iterator& other) const noexcept {
  return block_ == other.block_ && slot_ == other.slot_;
}

void IOBufChain::const_iterator::increment() noexcept {
  if (++slot_ != block_->endIndex) {
    return;
  }
  block_ = block_->next;
  slot_ = block_ ? block_->beginIndex : 0;
}

bool operator==(const IOBufChain& left, const IOBufChain& right) noexcept {
  return left.chainLength() == right.chainLength() &&
      compareChainData(left, right) == std::strong_ordering::equal;
}

std::strong_ordering operator<=>(
    const IOBufChain& left, const IOBufChain& right) noexcept {
  return compareChainData(left, right);
}

IOBufChain::ChainInfo IOBufChain::computeChainInfo(
    const folly::IOBuf& head) noexcept {
  ChainInfo info;
  for (const auto buf : head) {
    info.dataLength += buf.size();
    ++info.elements;
  }
  return info;
}

IOBufChain::IOBufChain(size_t initialBlockCapacity) {
  if (initialBlockCapacity != 0) {
    appendBlock(initialBlockCapacity);
  }
}

IOBufChain::IOBufChain(std::unique_ptr<folly::IOBuf> buf) : IOBufChain() {
  if (!buf) {
    return;
  }
  const auto elements = buf->countChainElements();
  append(std::move(buf), elements);
}

IOBufChain::IOBufChain(const IOBufChain& other) : IOBufChain(other.clone()) {}

IOBufChain& IOBufChain::operator=(const IOBufChain& other) {
  if (this != &other) {
    *this = other.clone();
  }
  return *this;
}
IOBufChain::IOBufChain(IOBufChain&& other) noexcept
    : head_(std::exchange(other.head_, nullptr)),
      tail_(std::exchange(other.tail_, nullptr)),
      byteLength_(std::exchange(other.byteLength_, 0)),
      elementCount_(std::exchange(other.elementCount_, 0)) {}

IOBufChain::~IOBufChain() {
  Block::destroyChain(head_);
}

IOBufChain& IOBufChain::operator=(IOBufChain&& other) noexcept {
  if (this == &other) {
    return *this;
  }
  Block::destroyChain(head_);
  head_ = std::exchange(other.head_, nullptr);
  tail_ = std::exchange(other.tail_, nullptr);
  byteLength_ = std::exchange(other.byteLength_, 0);
  elementCount_ = std::exchange(other.elementCount_, 0);
  return *this;
}

void IOBufChain::clear() noexcept {
  auto* const blocks = std::exchange(head_, nullptr);
  tail_ = nullptr;
  byteLength_ = 0;
  elementCount_ = 0;
  Block::destroyChain(blocks);
}

void IOBufChain::append(std::unique_ptr<folly::IOBuf> buf) {
  append(std::move(buf), kDefaultBlockCapacity);
}

void IOBufChain::append(
    std::unique_ptr<folly::IOBuf> buf, size_t nextBlockCapacity) {
  while (buf) {
    if (!tail_ || tail_->full()) {
      appendBlock(nextBlockCapacity);
    }
    auto current = std::move(buf);
    buf = current->pop();
    emplaceIOBuf(std::move(*current));
  }
}

void IOBufChain::append(IOBufChain&& other) {
  if (&other == this || other.empty()) {
    return;
  }
  if (empty()) {
    *this = std::move(other);
    return;
  }

  tail_->next = other.head_;
  tail_ = other.tail_;
  other.head_ = nullptr;
  other.tail_ = nullptr;
  byteLength_ += std::exchange(other.byteLength_, 0);
  elementCount_ += std::exchange(other.elementCount_, 0);
}

std::unique_ptr<folly::IOBuf> IOBufChain::pop() {
  if (empty()) {
    return nullptr;
  }

  auto result = std::make_unique<folly::IOBuf>();
  extractFront(*result);
  return result;
}

void IOBufChain::extractFront(folly::IOBuf& result) noexcept {
  DCHECK(!empty());
  head_->pop(result);
  byteLength_ -= result.length();

  if (--elementCount_ == 0) {
    DCHECK_EQ(head_, tail_);
    DCHECK_EQ(byteLength_, 0);
    Block::destroy(head_);
    head_ = nullptr;
    tail_ = nullptr;
    return;
  }

  if (head_->empty()) {
    auto* exhausted = head_;
    head_ = exhausted->next;
    Block::destroy(exhausted);
  }
}

IOBufChain IOBufChain::splitAt(size_t offset) {
  if (offset > byteLength_) {
    folly::throw_exception<std::out_of_range>(
        "IOBufChain split offset exceeds chain length");
  }
  if (offset == 0) {
    return {};
  }
  if (offset == byteLength_) {
    return IOBufChain{std::move(*this)};
  }

  size_t prefixElements = 0;
  size_t bytesToCount = offset;
  for (const auto& buffer : *this) {
    ++prefixElements;
    if (buffer.length() >= bytesToCount) {
      break;
    }
    bytesToCount -= buffer.length();
  }

  IOBufChain result{prefixElements};
  size_t remaining = offset;
  while (remaining > 0) {
    auto& first = head_->at(head_->beginIndex);
    const auto length = first.length();
    if (length > remaining) {
      result.appendClonedRange(
          first,
          0,
          remaining,
          folly::io::CloneOwnership::Shared,
          prefixElements);
      first.trimStart(remaining);
      byteLength_ -= remaining;
      return result;
    }
    folly::IOBuf prefix;
    extractFront(prefix);
    remaining -= length;
    result.emplaceIOBuf(std::move(prefix));
  }
  return result;
}

void IOBufChain::trimStart(size_t amount) {
  if (amount > byteLength_) {
    folly::throw_exception<std::out_of_range>(
        "IOBufChain trim exceeds chain length");
  }
  if (amount == 0) {
    return;
  }
  if (amount == byteLength_) {
    clear();
    return;
  }

  while (amount > 0) {
    auto& first = head_->at(head_->beginIndex);
    const auto length = first.length();
    if (length > amount) {
      first.trimStart(amount);
      byteLength_ -= amount;
      return;
    }
    folly::IOBuf removed;
    extractFront(removed);
    amount -= length;
  }
}

IOBufChain::const_iterator IOBufChain::begin() const noexcept {
  return empty() ? end() : const_iterator{head_, head_->beginIndex};
}

IOBufChain::const_iterator IOBufChain::end() const noexcept {
  return const_iterator{nullptr, 0};
}

IOBufChain IOBufChain::clone() const {
  if (empty()) {
    return {};
  }
  IOBufChain result{chainElements()};
  for (const auto* block = head_; block; block = block->next) {
    for (size_t i = block->beginIndex; i < block->endIndex; ++i) {
      result.emplaceIOBuf(block->at(i).cloneOneAsValue());
    }
  }
  return result;
}

std::unique_ptr<folly::IOBuf> IOBufChain::moveToIOBuf() && {
  std::unique_ptr<folly::IOBuf> result;
  while (auto node = pop()) {
    if (!result) {
      result = std::move(node);
    } else {
      result->appendToChain(std::move(node));
    }
  }
  return result;
}

void IOBufChain::emplaceIOBuf(folly::IOBuf&& buffer) {
  DCHECK(!buffer.isChained());
  const auto length = buffer.length();
  std::construct_at(tail_->rawSlot(tail_->endIndex), std::move(buffer));
  byteLength_ += length;
  ++elementCount_;
  ++tail_->endIndex;
}

void IOBufChain::appendClonedRange(
    const folly::IOBuf& source,
    size_t offset,
    size_t length,
    folly::io::CloneOwnership ownership,
    size_t nextBlockCapacity) {
  DCHECK_LE(offset, source.length());
  DCHECK_LE(length, source.length() - offset);
  if (!tail_ || tail_->full()) {
    appendBlock(nextBlockCapacity);
  }
  if (ownership == folly::io::CloneOwnership::Managed &&
      !source.isManagedOne()) {
    emplaceIOBuf(
        folly::IOBuf{
            folly::IOBuf::COPY_BUFFER, source.data() + offset, length});
    return;
  }
  auto clone = source.cloneOneAsValue();
  clone.trimStart(offset);
  clone.trimEnd(clone.length() - length);
  emplaceIOBuf(std::move(clone));
}

void IOBufChain::appendBlock(size_t capacity) {
  DCHECK_NE(capacity, 0);
  auto* block = Block::create(capacity);
  if (tail_) {
    tail_->next = block;
  } else {
    head_ = block;
  }
  tail_ = block;
}

} // namespace apache::thrift
