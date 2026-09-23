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

#include <array>
#include <cstring>
#include <iterator>
#include <memory>
#include <optional>
#include <stdexcept>
#include <string>

#include <folly/portability/GTest.h>

using apache::thrift::IOBufChain;
using apache::thrift::io::IOBufChainCursor;
using folly::IOBuf;
using folly::io::CloneOwnership;

namespace {

IOBufChain chainOf(std::initializer_list<std::string> parts) {
  IOBufChain chain;
  for (const auto& part : parts) {
    chain.append(IOBuf::copyBuffer(part));
  }
  return chain;
}

std::string chainString(const IOBufChain& chain) {
  std::string result;
  result.reserve(chain.chainLength());
  for (const auto& buffer : chain) {
    result.append(
        reinterpret_cast<const char*>(buffer.data()), buffer.length());
  }
  return result;
}

struct TestAllocation {
  explicit TestAllocation(std::string contents)
      : size(contents.size()), data(std::make_unique<uint8_t[]>(size)) {
    std::memcpy(data.get(), contents.data(), size);
  }

  size_t size;
  std::unique_ptr<uint8_t[]> data;
  size_t releases{0};
};

struct TestLease {
  std::shared_ptr<TestAllocation> allocation;
};

void releaseTestLease(void*, void* userData) noexcept {
  std::unique_ptr<TestLease> lease{static_cast<TestLease*>(userData)};
  ++lease->allocation->releases;
}

std::unique_ptr<IOBuf> makeSlice(
    const std::shared_ptr<TestAllocation>& allocation,
    size_t offset,
    size_t length) {
  auto lease = std::make_unique<TestLease>();
  lease->allocation = allocation;
  auto buffer = IOBuf::takeOwnership(
      allocation->data.get() + offset,
      length,
      length,
      releaseTestLease,
      lease.get(),
      false);
  buffer->markExternallySharedOne();
  std::ignore = lease.release();
  return buffer;
}

TEST(IOBufChainCursor, ReadsAcrossBuffers) {
  auto allocation = std::make_shared<TestAllocation>(
      std::string{"\x01_\x02\x03_\x04hello", 11});
  IOBufChain chain;
  chain.append(makeSlice(allocation, 0, 1));
  chain.append(makeSlice(allocation, 2, 2));
  chain.append(makeSlice(allocation, 5, 6));
  IOBufChainCursor cursor(chain);

  EXPECT_EQ(0x01020304, cursor.readBE<uint32_t>());
  EXPECT_EQ("hello", cursor.readFixedString(5));
  EXPECT_TRUE(cursor.isAtEnd());
  EXPECT_EQ(chain.chainLength(), cursor.getCurrentPosition());
  EXPECT_THROW(cursor.read<uint8_t>(), std::out_of_range);
}

TEST(IOBufChainCursor, ContiguousOperations) {
  IOBufChain chain = chainOf({"abcdef", "gh"});
  const auto second = std::next(chain.begin());
  IOBufChainCursor cursor(chain);

  EXPECT_EQ(0x6162, cursor.readBE<uint16_t>());

  std::array<char, 2> pulled{};
  cursor.pull(pulled.data(), pulled.size());
  EXPECT_EQ((std::array<char, 2>{'c', 'd'}), pulled);

  std::array<char, 2> pulledAtMost{};
  EXPECT_EQ(
      pulledAtMost.size(),
      cursor.pullAtMost(pulledAtMost.data(), pulledAtMost.size()));
  EXPECT_EQ((std::array<char, 2>{'e', 'f'}), pulledAtMost);
  EXPECT_EQ(second->data(), cursor.data());

  cursor.skip(1);
  EXPECT_EQ('h', cursor.read<char>());
  EXPECT_TRUE(cursor.isAtEnd());
}

TEST(IOBufChainCursor, PeekPullSkipAndReset) {
  IOBufChain chain = chainOf({"ab", "cde", "f"});
  IOBufChainCursor beginning(chain);
  IOBufChainCursor cursor(chain);

  const auto bytes = cursor.peekBytes();
  EXPECT_EQ("ab", std::string(bytes.begin(), bytes.end()));
  EXPECT_EQ(chain.begin()->data(), cursor.data());
  EXPECT_EQ(2, cursor.length());
  cursor.skipNoAdvance(1);
  EXPECT_EQ(1, cursor.length());
  EXPECT_EQ(1, cursor - beginning);

  std::array<char, 8> output{};
  EXPECT_EQ(5, cursor.pullAtMost(output.data(), output.size()));
  EXPECT_EQ("bcdef", std::string(output.data(), 5));
  EXPECT_TRUE(cursor.isAtEnd());
  EXPECT_FALSE(cursor.canAdvance(1));

  cursor.reset(&chain);
  cursor.skip(3);
  EXPECT_EQ('d', cursor.read<char>());
  cursor.reset();
  EXPECT_TRUE(cursor.isAtEnd());
  EXPECT_EQ(nullptr, cursor.data());
  EXPECT_EQ(0, cursor.length());
}

TEST(IOBufChainCursor, FailedOperationsDoNotAdvance) {
  IOBufChain chain = chainOf({"abc"});
  IOBufChainCursor cursor(chain);
  std::array<char, 4> output{};

  EXPECT_THROW(cursor.pull(output.data(), output.size()), std::out_of_range);
  EXPECT_EQ(0, cursor.getCurrentPosition());
  EXPECT_THROW(cursor.skip(4), std::out_of_range);
  EXPECT_EQ(0, cursor.getCurrentPosition());

  IOBufChain clone;
  EXPECT_THROW(cursor.clone(clone, 4), std::out_of_range);
  EXPECT_EQ(0, cursor.getCurrentPosition());
  EXPECT_TRUE(clone.empty());
}

TEST(IOBufChainCursor, CloneOverloadsPreserveRequestedRange) {
  IOBufChain source = chainOf({"ab", "cde", "f"});

  IOBufChainCursor chainCursor(source);
  chainCursor.skip(1);
  IOBufChain chainClone;
  chainCursor.clone(chainClone, 4);
  EXPECT_EQ("bcde", chainString(chainClone));

  IOBufChainCursor pointerCursor(source);
  pointerCursor.skip(2);
  std::unique_ptr<IOBuf> pointerClone;
  pointerCursor.clone(pointerClone, 3);
  ASSERT_NE(nullptr, pointerClone);
  EXPECT_EQ("cde", pointerClone->toString());

  IOBufChainCursor valueCursor(source);
  valueCursor.skip(5);
  IOBuf valueClone;
  valueCursor.clone(valueClone, 1);
  EXPECT_EQ("f", valueClone.toString());
}

TEST(IOBufChainCursor, SmallCloneCopiesOnlyRequestedData) {
  IOBufChain source;
  for (size_t i = 0; i < 100; ++i) {
    source.append(IOBuf::copyBuffer(std::string(1, 'a' + i % 26)));
  }
  IOBufChainCursor cursor(source);
  IOBufChain clone;

  cursor.clone(clone, 1);

  EXPECT_EQ(1, clone.chainElements());
  EXPECT_EQ("a", chainString(clone));
}

TEST(IOBufChainCursor, ManagedCloneCopiesUnmanagedData) {
  std::array<uint8_t, 4> storage{'a', 'b', 'c', 'd'};
  IOBufChain source{IOBuf::wrapBuffer(storage.data(), storage.size())};
  IOBufChainCursor cursor(source);
  IOBufChain clone;

  cursor.clone(clone, storage.size(), CloneOwnership::Managed);
  storage[0] = 'z';

  EXPECT_EQ("abcd", chainString(clone));
  EXPECT_EQ("zbcd", chainString(source));
}

TEST(IOBufChainCursor, CloneRetainsOnlyOverlappingOwners) {
  auto allocation = std::make_shared<TestAllocation>("abcdefgh");
  auto first = makeSlice(allocation, 0, 4);
  first->appendToChain(makeSlice(allocation, 4, 4));
  IOBufChain source{std::move(first)};

  IOBufChainCursor cursor(source);
  cursor.skip(4);
  IOBufChain clone;
  cursor.clone(clone, 4);

  EXPECT_EQ("efgh", chainString(clone));
  source.clear();
  EXPECT_EQ(1, allocation->releases);
  clone.clear();
  EXPECT_EQ(2, allocation->releases);
}

TEST(IOBufChainCursor, TraversesSplicedBlocksAfterPop) {
  auto allocation = std::make_shared<TestAllocation>(std::string(240, '_'));
  IOBufChain destination;
  IOBufChain source;
  std::string expected;
  for (size_t i = 0; i < 50; ++i) {
    allocation->data[i * 2] = static_cast<uint8_t>('a' + i % 26);
    allocation->data[120 + i * 2] = static_cast<uint8_t>('A' + i % 26);
    destination.append(makeSlice(allocation, i * 2, 1));
    source.append(makeSlice(allocation, 120 + i * 2, 1));
    if (i >= 10) {
      expected.push_back(static_cast<char>(allocation->data[i * 2]));
    }
  }
  for (size_t i = 10; i < 50; ++i) {
    expected.push_back(static_cast<char>(allocation->data[120 + i * 2]));
  }
  for (size_t i = 0; i < 10; ++i) {
    std::ignore = destination.pop();
    std::ignore = source.pop();
  }
  destination.append(std::move(source));

  IOBufChainCursor cursor(destination);

  EXPECT_EQ(expected, cursor.readFixedString(expected.size()));
  EXPECT_TRUE(cursor.isAtEnd());
}

} // namespace
