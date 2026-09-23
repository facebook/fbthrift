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

#include <array>
#include <compare>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <memory>
#include <new>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include <folly/portability/GTest.h>

using apache::thrift::IOBufChain;
using folly::IOBuf;
using std::string;
using std::unique_ptr;

namespace {

void releaseTrackedBuffer(void* data, void* userData) noexcept {
  delete[] static_cast<uint8_t*>(data);
  ++*static_cast<size_t*>(userData);
}

unique_ptr<IOBuf> trackedBuffer(size_t& destructionCount) {
  return IOBuf::takeOwnership(
      new uint8_t[1], 1, releaseTrackedBuffer, &destructionCount);
}

struct BufferReleaseTracker {
  size_t count{0};
  void* data{nullptr};
};

void recordBufferRelease(void* data, void* userData) noexcept {
  auto& tracker = *static_cast<BufferReleaseTracker*>(userData);
  ++tracker.count;
  tracker.data = data;
}

unique_ptr<IOBuf> makeOwnedBuffer(
    void* data, size_t size, BufferReleaseTracker& tracker) {
  auto buffer = IOBuf::takeOwnership(
      data, size, size, recordBufferRelease, &tracker, false);
  buffer->markExternallySharedOne();
  return buffer;
}

struct TestAllocation {
  explicit TestAllocation(string contents)
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

IOBufChain chainOf(std::initializer_list<string> parts) {
  IOBufChain chain;
  for (const auto& p : parts) {
    chain.append(IOBuf::copyBuffer(p));
  }
  return chain;
}

unique_ptr<IOBuf> intrusiveChainOf(std::initializer_list<string> parts) {
  unique_ptr<IOBuf> result;
  for (const auto& part : parts) {
    auto buffer = IOBuf::copyBuffer(part);
    if (result) {
      result->appendToChain(std::move(buffer));
    } else {
      result = std::move(buffer);
    }
  }
  return result;
}

unique_ptr<IOBuf> trackedBuffers(size_t& destructionCount, size_t elements) {
  auto result = trackedBuffer(destructionCount);
  for (size_t i = 1; i < elements; ++i) {
    result->appendToChain(trackedBuffer(destructionCount));
  }
  return result;
}

void checkConsistency(IOBufChain&& chain, const string& expected) {
  const auto bytes = chain.chainLength();
  const auto elements = chain.chainElements();
  auto buffers = std::move(chain).moveToIOBuf();
  EXPECT_EQ(elements == 0, buffers == nullptr);
  if (!buffers) {
    EXPECT_EQ(0, bytes);
    EXPECT_TRUE(expected.empty());
    return;
  }
  EXPECT_EQ(bytes, buffers->computeChainDataLength());
  EXPECT_EQ(elements, buffers->countChainElements());
  EXPECT_EQ(expected, buffers->toString());
}

string chainString(const IOBufChain& chain) {
  string result;
  result.reserve(chain.chainLength());
  for (const auto& buffer : chain) {
    result.append(
        reinterpret_cast<const char*>(buffer.data()), buffer.length());
  }
  return result;
}

void checkConsistency(const IOBufChain& chain) {
  size_t bytes = 0;
  size_t elements = 0;
  for (const auto& buffer : chain) {
    EXPECT_FALSE(buffer.isChained());
    bytes += buffer.length();
    ++elements;
  }
  EXPECT_EQ(bytes, chain.chainLength());
  EXPECT_EQ(elements, chain.chainElements());
  EXPECT_EQ(elements == 0, chain.empty());
}

} // namespace

TEST(IOBufChain, Empty) {
  IOBufChain chain;
  EXPECT_TRUE(chain.empty());
  EXPECT_EQ(0, chain.chainLength());
  EXPECT_EQ(0, chain.chainElements());
  EXPECT_EQ(nullptr, chain.pop());
  checkConsistency(std::move(chain), "");
}

TEST(IOBufChain, FromIOBufNull) {
  IOBufChain chain{nullptr};
  EXPECT_TRUE(chain.empty());
  checkConsistency(std::move(chain), "");
}

TEST(IOBufChain, ComputeChainInfo) {
  IOBuf head{IOBuf::COPY_BUFFER, "abc"};
  head.appendToChain(IOBuf::copyBuffer(""));
  head.appendToChain(IOBuf::copyBuffer("de"));
  const auto info = IOBufChain::computeChainInfo(head);
  EXPECT_EQ(3, info.elements);
  EXPECT_EQ(5, info.dataLength);
}

TEST(IOBufChain, FromIOBufChain) {
  auto head = IOBuf::copyBuffer("Hello");
  head->appendToChain(IOBuf::copyBuffer(", "));
  head->appendToChain(IOBuf::copyBuffer("World"));
  IOBufChain chain{std::move(head)};
  EXPECT_EQ(12, chain.chainLength());
  EXPECT_EQ(3, chain.chainElements());
  checkConsistency(std::move(chain), "Hello, World");
}

TEST(IOBufChain, AppendBuf) {
  IOBufChain chain;
  chain.append(IOBuf::copyBuffer("Hello"));
  EXPECT_EQ(5, chain.chainLength());
  EXPECT_EQ(1, chain.chainElements());
  chain.append(IOBuf::copyBuffer(", World"));
  EXPECT_EQ(12, chain.chainLength());
  EXPECT_EQ(2, chain.chainElements());
  checkConsistency(std::move(chain), "Hello, World");
}

TEST(IOBufChain, AppendOwnedBufferMovesMetadataIntoBlock) {
  std::array<uint8_t, 8> data{};
  BufferReleaseTracker tracker;

  {
    auto buffer = makeOwnedBuffer(data.data(), data.size(), tracker);
    IOBufChain chain;
    chain.append(std::move(buffer));

    ASSERT_FALSE(chain.empty());
    const auto& stored = *chain.begin();
    EXPECT_EQ(stored.data(), data.data());
    EXPECT_EQ(stored.length(), data.size());
    EXPECT_TRUE(stored.isSharedOne());
    EXPECT_EQ(chain.chainElements(), 1);
    EXPECT_EQ(chain.chainLength(), data.size());
    EXPECT_EQ(tracker.count, 0);
  }

  EXPECT_EQ(tracker.count, 1);
  EXPECT_EQ(tracker.data, data.data());
}

TEST(IOBufChain, PoppedBufferRetainsOwnership) {
  std::array<uint8_t, 8> data{};
  BufferReleaseTracker tracker;
  std::unique_ptr<IOBuf> popped;

  {
    auto buffer = makeOwnedBuffer(data.data(), data.size(), tracker);
    IOBufChain chain;
    chain.append(std::move(buffer));
    popped = chain.pop();
    EXPECT_TRUE(chain.empty());
  }

  EXPECT_EQ(tracker.count, 0);
  popped.reset();
  EXPECT_EQ(tracker.count, 1);
}

TEST(IOBufChain, CopiedBufferRetainsOwnership) {
  std::array<uint8_t, 8> data{};
  BufferReleaseTracker tracker;
  std::optional<IOBufChain> copy;

  {
    auto buffer = makeOwnedBuffer(data.data(), data.size(), tracker);
    IOBufChain chain;
    chain.append(std::move(buffer));
    copy.emplace(chain);
  }

  EXPECT_EQ(tracker.count, 0);
  copy.reset();
  EXPECT_EQ(tracker.count, 1);
}

TEST(IOBufChain, AdjacentBuffersRemainSeparate) {
  std::array<uint8_t, 16> data{};
  BufferReleaseTracker firstTracker;
  BufferReleaseTracker secondTracker;

  {
    auto first = makeOwnedBuffer(data.data(), 8, firstTracker);
    auto second = makeOwnedBuffer(data.data() + 8, 8, secondTracker);

    IOBufChain chain;
    chain.append(std::move(first));
    chain.append(std::move(second));
    EXPECT_EQ(chain.chainElements(), 2);
    EXPECT_EQ(chain.chainLength(), data.size());
    EXPECT_EQ(chain.begin()->data(), data.data());
  }

  EXPECT_EQ(firstTracker.count, 1);
  EXPECT_EQ(secondTracker.count, 1);
}

TEST(IOBufChain, AppendNullBufIsNoop) {
  IOBufChain chain = chainOf({"abc"});
  chain.append(unique_ptr<IOBuf>());
  EXPECT_EQ(3, chain.chainLength());
  EXPECT_EQ(1, chain.chainElements());
  checkConsistency(std::move(chain), "abc");
}

TEST(IOBufChain, AppendChain) {
  IOBufChain a = chainOf({"Hello", ", "});
  IOBufChain b = chainOf({"World", "!"});
  a.append(std::move(b));
  EXPECT_EQ(4, a.chainElements());
  EXPECT_EQ(13, a.chainLength());
  // NOLINTNEXTLINE(bugprone-use-after-move)
  EXPECT_TRUE(b.empty());
  // NOLINTNEXTLINE(bugprone-use-after-move)
  EXPECT_EQ(0, b.chainElements());
  checkConsistency(std::move(a), "Hello, World!");
}

TEST(IOBufChain, AppendChainIntoEmpty) {
  IOBufChain a;
  IOBufChain b = chainOf({"x", "y"});
  a.append(std::move(b));
  EXPECT_EQ(2, a.chainElements());
  // NOLINTNEXTLINE(bugprone-use-after-move)
  EXPECT_TRUE(b.empty());
  // NOLINTNEXTLINE(bugprone-use-after-move)
  EXPECT_EQ(0, b.chainElements());
  checkConsistency(std::move(a), "xy");
}

TEST(IOBufChain, AppendChainedIOBuf) {
  auto buffers = IOBuf::copyBuffer("abcde");
  buffers->appendToChain(IOBuf::copyBuffer("fg"));

  IOBufChain chain = chainOf({"start"});
  chain.append(std::move(buffers));
  EXPECT_EQ(12, chain.chainLength());
  EXPECT_EQ(3, chain.chainElements());
  checkConsistency(std::move(chain), "startabcdefg");
}

TEST(IOBufChain, Pop) {
  IOBufChain chain = chainOf({"aa", "bbb", "c"});
  EXPECT_EQ(3, chain.chainElements());
  EXPECT_EQ(6, chain.chainLength());

  auto first = chain.pop();
  ASSERT_NE(nullptr, first);
  EXPECT_EQ("aa", first->toString());
  EXPECT_EQ(2, chain.chainElements());
  EXPECT_EQ(4, chain.chainLength());
  EXPECT_EQ("bbb", chain.pop()->toString());
  EXPECT_EQ("c", chain.pop()->toString());
  EXPECT_TRUE(chain.empty());
  EXPECT_EQ(nullptr, chain.pop());
  checkConsistency(std::move(chain), "");
}

TEST(IOBufChain, MoveToIOBuf) {
  IOBufChain chain = chainOf({"Hello", "World"});
  auto raw = std::move(chain).moveToIOBuf();
  ASSERT_NE(nullptr, raw);
  EXPECT_EQ(10, raw->computeChainDataLength());
  EXPECT_EQ(2, raw->countChainElements());
  // NOLINTNEXTLINE(bugprone-use-after-move)
  EXPECT_TRUE(chain.empty());
  // NOLINTNEXTLINE(bugprone-use-after-move)
  EXPECT_EQ(0, chain.chainElements());
}

TEST(IOBufChain, MoveConstruct) {
  IOBufChain a = chainOf({"abc", "de"});
  IOBufChain b(std::move(a));
  EXPECT_EQ(2, b.chainElements());
  EXPECT_EQ(5, b.chainLength());
  // NOLINTNEXTLINE(bugprone-use-after-move)
  EXPECT_TRUE(a.empty());
  // NOLINTNEXTLINE(bugprone-use-after-move)
  EXPECT_EQ(0, a.chainElements());
  checkConsistency(std::move(b), "abcde");
}

TEST(IOBufChain, MoveAssign) {
  IOBufChain a = chainOf({"abc", "de"});
  IOBufChain b = chainOf({"zzzz"});
  b = std::move(a);
  EXPECT_EQ(2, b.chainElements());
  // NOLINTNEXTLINE(bugprone-use-after-move)
  EXPECT_TRUE(a.empty());
  // NOLINTNEXTLINE(bugprone-use-after-move)
  EXPECT_EQ(0, a.chainElements());
  checkConsistency(std::move(b), "abcde");
}

TEST(IOBufChain, Clear) {
  IOBufChain chain = chainOf({"abc", "de"});
  chain.clear();
  checkConsistency(std::move(chain), "");
}

TEST(IOBufChain, ZeroLengthBuffersAreCounted) {
  IOBufChain chain = chainOf({"a", "", "b"});
  EXPECT_EQ(3, chain.chainElements());
  EXPECT_EQ(2, chain.chainLength());
  EXPECT_FALSE(chain.empty());
  checkConsistency(std::move(chain), "ab");
}

TEST(IOBufChain, SingleEmptyBufferIsNotEmpty) {
  IOBufChain chain = chainOf({""});
  EXPECT_FALSE(chain.empty());
  EXPECT_EQ(1, chain.chainElements());
  EXPECT_EQ(0, chain.chainLength());
  checkConsistency(std::move(chain), "");
}

TEST(IOBufChain, PopEmptyBuffer) {
  IOBufChain chain = chainOf({"", "x"});
  auto first = chain.pop();
  ASSERT_NE(nullptr, first);
  EXPECT_EQ(0, first->length());
  EXPECT_EQ(1, chain.chainElements());
  checkConsistency(std::move(chain), "x");
}

TEST(IOBufChain, MoveToIOBufEmpty) {
  IOBufChain chain;
  EXPECT_EQ(nullptr, std::move(chain).moveToIOBuf());
  // NOLINTNEXTLINE(bugprone-use-after-move)
  EXPECT_TRUE(chain.empty());
}

TEST(IOBufChain, AppendAfterDrainToEmpty) {
  IOBufChain chain = chainOf({"ab"});
  chain.pop();
  ASSERT_TRUE(chain.empty());
  chain.append(IOBuf::copyBuffer("cd"));
  EXPECT_EQ(1, chain.chainElements());
  EXPECT_EQ(2, chain.chainLength());
  checkConsistency(std::move(chain), "cd");
}

TEST(IOBufChain, AppendEmptyChainIsNoop) {
  IOBufChain a = chainOf({"x"});
  IOBufChain empty;
  a.append(std::move(empty));
  EXPECT_EQ(1, a.chainElements());
  checkConsistency(std::move(a), "x");
}

TEST(IOBufChain, AppendSelfIsNoop) {
  IOBufChain chain = chainOf({"abc", "de"});
  chain.append(std::move(chain));
  // NOLINTNEXTLINE(bugprone-use-after-move)
  EXPECT_EQ(5, chain.chainLength());
  // NOLINTNEXTLINE(bugprone-use-after-move)
  EXPECT_EQ(2, chain.chainElements());
  // NOLINTNEXTLINE(bugprone-use-after-move)
  checkConsistency(std::move(chain), "abcde");
}

TEST(IOBufChain, AppendPastFullBlockPreservesContents) {
  IOBufChain chain{intrusiveChainOf({"a", "b"})};

  chain.append(IOBuf::copyBuffer("c"));

  EXPECT_EQ(3, chain.chainElements());
  checkConsistency(std::move(chain), "abc");
}

TEST(IOBufChain, DestroysOwnersAcrossBlockBoundaries) {
  size_t destructionCount = 0;
  {
    IOBufChain chain{trackedBuffers(destructionCount, 2)};
    chain.append(trackedBuffer(destructionCount));

    auto popped = chain.pop();
    EXPECT_EQ(0, destructionCount);
    popped.reset();
    EXPECT_EQ(1, destructionCount);

    chain.clear();
    EXPECT_EQ(3, destructionCount);
  }
  {
    IOBufChain chain{trackedBuffers(destructionCount, 2)};
    chain.append(trackedBuffer(destructionCount));
  }
  EXPECT_EQ(6, destructionCount);
}

TEST(IOBufChain, AppendPreservesPayload) {
  auto buffer = IOBuf::copyBuffer("payload");
  const auto* originalData = buffer->data();

  IOBufChain chain;
  chain.append(std::move(buffer));
  auto appended = chain.pop();

  ASSERT_NE(nullptr, appended);
  EXPECT_EQ(originalData, appended->data());
  EXPECT_EQ("payload", appended->toString());
}

TEST(IOBufChain, AppendPartiallyConsumedChain) {
  IOBufChain destination = chainOf({"drop", "left"});
  EXPECT_EQ("drop", destination.pop()->toString());
  IOBufChain source = chainOf({"drop", "middle", "right"});
  EXPECT_EQ("drop", source.pop()->toString());

  destination.append(std::move(source));

  EXPECT_EQ(3, destination.chainElements());
  EXPECT_EQ(15, destination.chainLength());
  // NOLINTNEXTLINE(bugprone-use-after-move)
  EXPECT_TRUE(source.empty());
  checkConsistency(std::move(destination), "leftmiddleright");
}

TEST(IOBufChain, AppendPartiallyConsumedMultiBlockChain) {
  IOBufChain source{intrusiveChainOf({"a", "b"})};
  source.append(IOBuf::copyBuffer("c"));
  EXPECT_EQ("a", source.pop()->toString());

  IOBufChain destination = chainOf({"prefix"});
  destination.append(std::move(source));

  EXPECT_EQ(3, destination.chainElements());
  // NOLINTNEXTLINE(bugprone-use-after-move)
  EXPECT_TRUE(source.empty());
  checkConsistency(std::move(destination), "prefixbc");
}

TEST(IOBufChain, MoveToIOBufAfterPop) {
  auto buffers = IOBuf::copyBuffer("drop");
  std::vector<const uint8_t*> payloads;
  auto first = IOBuf::copyBuffer("a");
  payloads.push_back(first->data());
  buffers->appendToChain(std::move(first));
  IOBufChain chain{std::move(buffers)};
  auto last = IOBuf::copyBuffer("b");
  payloads.push_back(last->data());
  chain.append(std::move(last));
  EXPECT_EQ("drop", chain.pop()->toString());

  auto raw = std::move(chain).moveToIOBuf();

  ASSERT_NE(nullptr, raw);
  EXPECT_EQ("ab", raw->toString());
  EXPECT_EQ(payloads.size(), raw->countChainElements());
  const auto* current = raw.get();
  for (const auto* payload : payloads) {
    EXPECT_EQ(payload, current->data());
    current = current->next();
  }
  EXPECT_EQ(raw.get(), current);
  // NOLINTNEXTLINE(bugprone-use-after-move)
  EXPECT_TRUE(chain.empty());
}

TEST(IOBufChain, MovePartiallyConsumedChain) {
  IOBufChain source = chainOf({"drop", "abc", "de"});
  EXPECT_EQ("drop", source.pop()->toString());

  IOBufChain intermediate(std::move(source));
  IOBufChain destination = chainOf({"old"});
  destination = std::move(intermediate);

  EXPECT_EQ(2, destination.chainElements());
  EXPECT_EQ(5, destination.chainLength());
  // NOLINTNEXTLINE(bugprone-use-after-move)
  EXPECT_TRUE(source.empty());
  // NOLINTNEXTLINE(bugprone-use-after-move)
  EXPECT_TRUE(intermediate.empty());
  checkConsistency(std::move(destination), "abcde");
}

TEST(IOBufChain, PopThenAppendPreservesOrder) {
  IOBufChain chain = chainOf({"a", "bb", "ccc"});
  EXPECT_EQ("a", chain.pop()->toString());

  chain.append(IOBuf::copyBuffer("dddd"));

  EXPECT_EQ(3, chain.chainElements());
  EXPECT_EQ(9, chain.chainLength());
  checkConsistency(std::move(chain), "bbcccdddd");
}

TEST(IOBufChain, PopAppendCyclePreservesOrderWhenTailIsFull) {
  IOBufChain chain{intrusiveChainOf({"a", "b"})};
  EXPECT_EQ("a", chain.pop()->toString());

  chain.append(IOBuf::copyBuffer("c"));

  EXPECT_EQ(2, chain.chainElements());
  EXPECT_EQ(2, chain.chainLength());
  checkConsistency(std::move(chain), "bc");
}

TEST(IOBufChain, IterateAcrossSplicedBlocks) {
  IOBufChain chain;
  string expected;
  for (size_t block = 0; block < 32; ++block) {
    const auto value = static_cast<char>('a' + block % 26);
    expected.push_back(value);
    IOBufChain oneBlock;
    oneBlock.append(IOBuf::copyBuffer(string(1, value)));
    chain.append(std::move(oneBlock));
  }

  string iterated;
  for (const auto& buffer : chain) {
    iterated.append(
        reinterpret_cast<const char*>(buffer.data()), buffer.length());
  }
  EXPECT_EQ(expected, iterated);

  for (size_t i = 0; i < 16; ++i) {
    EXPECT_EQ(expected.substr(i, 1), chain.pop()->toString());
  }
  EXPECT_EQ(expected.substr(16), chainString(chain));
  checkConsistency(chain);
}

TEST(IOBufChain, CopySharesPayloadButNotMetadata) {
  IOBufChain original = chainOf({"abc", "de"});

  IOBufChain copy = original;

  EXPECT_EQ("abcde", chainString(copy));
  ASSERT_EQ(original.chainElements(), copy.chainElements());
  auto originalIt = original.begin();
  auto copyIt = copy.begin();
  for (; originalIt != original.end(); ++originalIt, ++copyIt) {
    EXPECT_NE(std::addressof(*originalIt), std::addressof(*copyIt));
    EXPECT_EQ(originalIt->data(), copyIt->data());
  }
  copy.trimStart(1);
  EXPECT_EQ("abcde", chainString(original));
  EXPECT_EQ("bcde", chainString(copy));
}

TEST(IOBufChain, AppendDoesNotRelocateExistingMetadata) {
  IOBufChain chain{intrusiveChainOf({"a", "b"})};
  const auto* first = std::addressof(*chain.begin());

  chain.append(IOBuf::copyBuffer("c"));

  EXPECT_EQ(first, std::addressof(*chain.begin()));
  checkConsistency(std::move(chain), "abc");
}

TEST(IOBufChain, CopyAssignmentReplacesContents) {
  IOBufChain original = chainOf({"abc", "de"});
  IOBufChain copy = chainOf({"old"});

  copy = original;

  EXPECT_EQ("abcde", chainString(copy));
  EXPECT_EQ(original.chainElements(), copy.chainElements());
  copy.clear();
  EXPECT_EQ("abcde", chainString(original));
}

TEST(IOBufChain, ComparisonIgnoresSegmentBoundaries) {
  const IOBufChain split = chainOf({"ab", "", "c"});
  const IOBufChain joined = chainOf({"abc"});
  const IOBufChain later = chainOf({"abd"});
  const IOBufChain prefix = chainOf({"ab"});

  EXPECT_EQ(split, joined);
  EXPECT_NE(split, prefix);
  EXPECT_EQ(std::strong_ordering::less, split <=> later);
  EXPECT_EQ(std::strong_ordering::greater, split <=> prefix);
}

TEST(IOBufChain, AdjacentBuffersRemainSeparateAndRetainEveryOwner) {
  auto allocation = std::make_shared<TestAllocation>("abcdefgh");
  auto first = makeSlice(allocation, 0, 4);
  first->appendToChain(makeSlice(allocation, 4, 4));

  IOBufChain chain{std::move(first)};

  EXPECT_EQ(2, chain.chainElements());
  EXPECT_EQ("abcdefgh", chainString(chain));
  EXPECT_EQ(allocation->data.get(), chain.begin()->data());
  EXPECT_EQ(0, allocation->releases);

  auto clone = chain.begin()->cloneOne();
  chain.clear();
  EXPECT_EQ(1, allocation->releases);
  clone.reset();
  EXPECT_EQ(2, allocation->releases);
}

TEST(IOBufChain, AdjacentBuffersRemainSeparateAtStorageBlockBoundary) {
  auto allocation = std::make_shared<TestAllocation>("abcdef");
  auto first = makeSlice(allocation, 0, 2);
  first->appendToChain(makeSlice(allocation, 2, 2));
  IOBufChain chain{std::move(first)};

  chain.append(makeSlice(allocation, 4, 2));

  EXPECT_EQ(3, chain.chainElements());
  EXPECT_EQ(2, std::next(chain.begin(), 2)->length());
  checkConsistency(chain);

  chain.clear();
  EXPECT_EQ(3, allocation->releases);
}

TEST(IOBufChain, NonAdjacentBuffersRemainSeparate) {
  auto allocation = std::make_shared<TestAllocation>("abcdefghi");
  auto first = makeSlice(allocation, 0, 4);
  first->appendToChain(makeSlice(allocation, 5, 4));

  IOBufChain chain{std::move(first)};

  EXPECT_EQ(2, chain.chainElements());
  EXPECT_EQ("abcdfghi", chainString(chain));
  chain.clear();
  EXPECT_EQ(2, allocation->releases);
}

TEST(IOBufChain, AppendPreservesAdjacentChainBoundary) {
  auto allocation = std::make_shared<TestAllocation>("abcdefgh");
  IOBufChain first{makeSlice(allocation, 0, 4)};
  IOBufChain second{makeSlice(allocation, 4, 4)};

  first.append(std::move(second));

  EXPECT_EQ(2, first.chainElements());
  EXPECT_EQ("abcdefgh", chainString(first));
}

TEST(IOBufChain, SplitAndTrimStartAcrossBlocks) {
  auto allocation = std::make_shared<TestAllocation>(string(12, 'x'));
  auto first = makeSlice(allocation, 0, 2);
  first->appendToChain(makeSlice(allocation, 3, 2));
  IOBufChain chain{std::move(first)};
  chain.append(makeSlice(allocation, 6, 2));
  chain.append(makeSlice(allocation, 9, 2));

  auto prefix = chain.splitAt(5);
  EXPECT_EQ(5, prefix.chainLength());
  EXPECT_EQ(3, chain.chainLength());
  EXPECT_EQ(3, prefix.chainElements());
  EXPECT_EQ(2, chain.chainElements());

  prefix.trimStart(3);
  EXPECT_EQ(2, prefix.chainLength());
  EXPECT_EQ(3, chain.chainLength());
  checkConsistency(prefix);
  checkConsistency(chain);
}

TEST(IOBufChain, SplitAtBoundariesAndRejectsOutOfRange) {
  IOBufChain chain = chainOf({"ab", "cde", "f"});

  auto emptyPrefix = chain.splitAt(0);
  EXPECT_TRUE(emptyPrefix.empty());
  EXPECT_EQ("abcdef", chainString(chain));

  auto prefix = chain.splitAt(3);
  EXPECT_EQ("abc", chainString(prefix));
  EXPECT_EQ("def", chainString(chain));

  auto remainder = chain.splitAt(chain.chainLength());
  EXPECT_EQ("def", chainString(remainder));
  EXPECT_TRUE(chain.empty());
  EXPECT_THROW(chain.splitAt(1), std::out_of_range);
}

TEST(IOBufChain, TrimStartHandlesBoundariesAndRejectsOutOfRange) {
  IOBufChain chain = chainOf({"ab", "cde"});

  chain.trimStart(0);
  EXPECT_EQ("abcde", chainString(chain));

  EXPECT_THROW(chain.trimStart(chain.chainLength() + 1), std::out_of_range);
  EXPECT_EQ("abcde", chainString(chain));

  chain.trimStart(chain.chainLength());
  EXPECT_TRUE(chain.empty());
}

TEST(IOBufChain, SplitBufferSharesOwnership) {
  std::array<uint8_t, 8> data{};
  BufferReleaseTracker tracker;
  IOBufChain chain;
  chain.append(makeOwnedBuffer(data.data(), data.size(), tracker));
  ASSERT_EQ(1, chain.chainElements());

  auto prefix = chain.splitAt(4);
  prefix.clear();

  EXPECT_EQ(0, tracker.count);
  chain.clear();
  EXPECT_EQ(1, tracker.count);
}
