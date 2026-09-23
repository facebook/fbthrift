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

#pragma once

#include <compare>
#include <cstddef>
#include <iterator>
#include <memory>

#include <folly/detail/Iterators.h>
#include <folly/io/IOBuf.h>

namespace folly::io {
enum class CloneOwnership;
} // namespace folly::io

namespace apache::thrift {

namespace io {
class IOBufChainCursor;
} // namespace io

class IOBufChain {
 private:
  struct Block;

 public:
  class const_iterator : public folly::detail::IteratorFacade<
                             const_iterator,
                             const folly::IOBuf,
                             std::forward_iterator_tag> {
   public:
    const_iterator() noexcept = default;

   private:
    friend class IOBufChain;
    friend class folly::detail::IteratorFacade<
        const_iterator,
        const folly::IOBuf,
        std::forward_iterator_tag>;

    const_iterator(const Block* block, size_t slot) noexcept;
    const folly::IOBuf& dereference() const noexcept;
    bool equal(const const_iterator& other) const noexcept;
    void increment() noexcept;

    const Block* block_{nullptr};
    size_t slot_{0};
  };

  struct ChainInfo {
    size_t elements{0};
    size_t dataLength{0};
  };

  static ChainInfo computeChainInfo(const folly::IOBuf& head) noexcept;

  IOBufChain() = default;
  ~IOBufChain();

  explicit IOBufChain(std::unique_ptr<folly::IOBuf> buf);

  IOBufChain(IOBufChain&& other) noexcept;
  IOBufChain& operator=(IOBufChain&& other) noexcept;

  IOBufChain(const IOBufChain& other);
  IOBufChain& operator=(const IOBufChain& other);

  size_t chainLength() const noexcept { return byteLength_; }
  size_t chainElements() const noexcept { return elementCount_; }
  bool empty() const noexcept { return elementCount_ == 0; }
  void clear() noexcept;

  void append(std::unique_ptr<folly::IOBuf> buf);
  void append(IOBufChain&& other);

  std::unique_ptr<folly::IOBuf> pop();
  IOBufChain splitAt(size_t offset);
  void trimStart(size_t amount);

  const_iterator begin() const noexcept;
  const_iterator end() const noexcept;

  IOBufChain clone() const;
  std::unique_ptr<folly::IOBuf> moveToIOBuf() &&;

 private:
  friend class io::IOBufChainCursor;

  static constexpr size_t kDefaultBlockCapacity = 10;

  explicit IOBufChain(size_t initialBlockCapacity);
  void append(std::unique_ptr<folly::IOBuf> buf, size_t nextBlockCapacity);
  void appendClonedRange(
      const folly::IOBuf& source,
      size_t offset,
      size_t length,
      folly::io::CloneOwnership ownership,
      size_t nextBlockCapacity);
  void extractFront(folly::IOBuf& result) noexcept;
  void emplaceIOBuf(folly::IOBuf&& buffer);
  void appendBlock(size_t capacity);

  Block* head_{nullptr};
  Block* tail_{nullptr};
  size_t byteLength_{0};
  size_t elementCount_{0};
};

bool operator==(const IOBufChain& left, const IOBufChain& right) noexcept;
std::strong_ordering operator<=>(
    const IOBufChain& left, const IOBufChain& right) noexcept;

} // namespace apache::thrift
