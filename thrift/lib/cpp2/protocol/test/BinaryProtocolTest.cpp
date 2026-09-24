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

#include <gtest/gtest.h>
#include <folly/CPortability.h>

#include <array>
#include <cstring>
#include <memory>
#include <string_view>
#include <tuple>
#include <type_traits>

#include <thrift/lib/cpp2/IOBufChain.h>
#include <thrift/lib/cpp2/protocol/BinaryProtocol.h>
#include <thrift/lib/cpp2/protocol/CompactProtocol.h>
#include <thrift/lib/cpp2/protocol/Cpp2Ops.h>
#include <thrift/lib/cpp2/protocol/detail/index.h>
#include <thrift/lib/cpp2/protocol/detail/protocol_methods.h>

using namespace apache::thrift;
using namespace apache::thrift::protocol;

namespace {

class BinaryProtocolTest : public testing::Test {};

static_assert(apache::thrift::detail::hasIndexSupport<BinaryProtocolReader>);
static_assert(
    !apache::thrift::detail::hasIndexSupport<BinaryProtocolChainReader>);

using IOBufChainProtocolMethods = detail::pm::protocol_methods<
    type_class::binary,
    IOBufChain,
    type::cpp_type<IOBufChain, type::binary_t>>;

IOBufChain chainOf(std::initializer_list<std::string_view> parts) {
  IOBufChain chain;
  for (const auto part : parts) {
    chain.append(folly::IOBuf::copyBuffer(part));
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
  explicit TestAllocation(size_t size)
      : data(std::make_unique<uint8_t[]>(size)) {}

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

std::unique_ptr<folly::IOBuf> makeManagedSlice(
    const std::shared_ptr<TestAllocation>& allocation,
    size_t offset,
    std::string_view contents) {
  std::memcpy(
      allocation->data.get() + offset, contents.data(), contents.size());
  auto lease = std::make_unique<TestLease>();
  lease->allocation = allocation;
  auto buffer = folly::IOBuf::takeOwnership(
      allocation->data.get() + offset,
      contents.size(),
      contents.size(),
      releaseTestLease,
      lease.get(),
      false);
  buffer->markExternallySharedOne();
  std::ignore = lease.release();
  return buffer;
}

template <typename Writer, typename Reader>
void expectIOBufChainProtocolMethodsRoundTrip() {
  const auto input = chainOf({"abc", "defg", "hijkl"});

  folly::IOBufQueue queue;
  Writer writer{SHARE_EXTERNAL_BUFFER};
  writer.setOutput(&queue);
  const auto serializedSize =
      IOBufChainProtocolMethods::template serializedSize<false>(writer, input);
  const auto expectedSize = input.chainLength() +
      (std::is_same_v<Writer, BinaryProtocolWriter> ? 4 : 1);
  if constexpr (std::is_same_v<Writer, BinaryProtocolWriter>) {
    EXPECT_EQ(expectedSize, serializedSize);
  } else {
    EXPECT_GE(serializedSize, expectedSize);
  }
  EXPECT_EQ(expectedSize, IOBufChainProtocolMethods::write(writer, input));

  Reader reader{SHARE_EXTERNAL_BUFFER};
  auto serialized = queue.move();
  reader.setInput(serialized.get());
  IOBufChain output;
  IOBufChainProtocolMethods::read(reader, output);

  EXPECT_EQ(chainString(input), chainString(output));
}

template <typename Writer>
void expectIOBufChainUsesSinglePackBudget() {
  constexpr size_t kPartSize = folly::IOBufQueue::kMaxPackCopy / 2;
  const std::string part(kPartSize, 'x');
  const auto input = chainOf({part, part, part, part});
  const auto packable = chainOf({part});

  Writer writer{SHARE_EXTERNAL_BUFFER};
  const auto estimatedPrefixSize = writer.serializedSizeI32();
  EXPECT_EQ(
      estimatedPrefixSize + packable.chainLength(),
      writer.serializedSizeZCBinary(packable));
  EXPECT_EQ(estimatedPrefixSize, writer.serializedSizeZCBinary(input));

  folly::IOBufQueue queue;
  const auto allocationSize = writer.serializedSizeBinary(input);
  queue.preallocate(allocationSize, allocationSize);
  writer.setOutput(&queue);
  const auto written = writer.writeBinary(input);

  auto output = queue.move();
  ASSERT_NE(nullptr, output);
  EXPECT_EQ(written, output->computeChainDataLength());
  const auto actualPrefixSize = written - input.chainLength();
  EXPECT_EQ(
      actualPrefixSize + folly::IOBufQueue::kMaxPackCopy, output->length());
}

TEST_F(BinaryProtocolTest, readInvalidBool) {
  uint8_t data[] = {0, 1, 2};
  auto buf = folly::IOBuf::wrapBufferAsValue(folly::range(data));

  BinaryProtocolReader inprot;
  bool value{};
  inprot.setInput(&buf);
  inprot.readBool(value);
  EXPECT_EQ(false, value) << "sanity check";
  inprot.readBool(value);
  EXPECT_EQ(true, value) << "sanity check";
  EXPECT_THROW(inprot.readBool(value), TProtocolException);
}

FOLLY_DISABLE_UNDEFINED_BEHAVIOR_SANITIZER("undefined")
bool makeInvalidBool() {
  // NOLINTNEXTLINE(modernize-raw-string-literal)
  return *reinterpret_cast<const volatile bool*>("\x42");
}

void testWriteInvalidBool() {
  auto w = BinaryProtocolWriter();
  auto q = folly::IOBufQueue();
  w.setOutput(&q);
  // writeBool should either fail CHECK or write a valid bool.

  w.writeBool(makeInvalidBool());
  auto s = std::string();
  q.appendToString(s);
  // Die on success.

  CHECK(s != std::string(1, '\0')) << "invalid bool value";
}

TEST_F(BinaryProtocolTest, writeInvalidBool) {
  EXPECT_DEATH({ testWriteInvalidBool(); }, "invalid bool value");
}

TEST_F(BinaryProtocolTest, writeStringExactly2GB) {
  auto w = BinaryProtocolWriter();
  auto q = folly::IOBufQueue();
  w.setOutput(&q);
  std::string monster((uint32_t)1 << 31, 'x');
  EXPECT_THROW(w.writeString(monster), TProtocolException);
}

TEST_F(BinaryProtocolTest, writeStringExceeds2GB) {
  auto w = BinaryProtocolWriter();
  auto q = folly::IOBufQueue();
  w.setOutput(&q);
  std::string monster(((uint32_t)1 << 31) + 100, 'x');
  EXPECT_THROW(w.writeString(monster), TProtocolException);
}

TEST_F(BinaryProtocolTest, writeStringExactly4GB) {
  auto w = BinaryProtocolWriter();
  auto q = folly::IOBufQueue();
  w.setOutput(&q);
  std::string monster((uint64_t)1 << 32, 'x');
  EXPECT_THROW(w.writeString(monster), TProtocolException);
}

TEST_F(BinaryProtocolTest, IOBufChainProtocolMethodsRoundTrip) {
  expectIOBufChainProtocolMethodsRoundTrip<
      BinaryProtocolWriter,
      BinaryProtocolReader>();
}

TEST_F(BinaryProtocolTest, IOBufChainUsesSinglePackBudget) {
  expectIOBufChainUsesSinglePackBudget<BinaryProtocolWriter>();
}

TEST_F(BinaryProtocolTest, CompactIOBufChainProtocolMethodsRoundTrip) {
  expectIOBufChainProtocolMethodsRoundTrip<
      CompactProtocolWriter,
      CompactProtocolReader>();
}

TEST_F(BinaryProtocolTest, CompactIOBufChainUsesSinglePackBudget) {
  expectIOBufChainUsesSinglePackBudget<CompactProtocolWriter>();
}

TEST_F(BinaryProtocolTest, IOBufChainCpp2OpsRoundTrip) {
  static_assert(Cpp2Ops<IOBufChain>::thriftType() == TType::T_STRING);
  const auto input = chainOf({"abc", "defg", "hijkl"});

  folly::IOBufQueue queue;
  BinaryProtocolWriter writer{SHARE_EXTERNAL_BUFFER};
  writer.setOutput(&queue);
  const auto expectedSize = input.chainLength() + 4;
  EXPECT_EQ(expectedSize, Cpp2Ops<IOBufChain>::serializedSize(&writer, &input));
  EXPECT_EQ(
      expectedSize, Cpp2Ops<IOBufChain>::serializedSizeZC(&writer, &input));
  EXPECT_EQ(expectedSize, Cpp2Ops<IOBufChain>::write(&writer, &input));

  IOBufChain serialized{queue.move()};
  BinaryProtocolChainReader reader{SHARE_EXTERNAL_BUFFER};
  reader.setInput(&serialized);
  IOBufChain output;
  Cpp2Ops<IOBufChain>::read(&reader, &output);

  EXPECT_EQ(chainString(input), chainString(output));
}

TEST_F(BinaryProtocolTest, IOBufChainStringTraitsCompareBytes) {
  const auto segmented = chainOf({"ab", "", "cd"});
  const auto sameBytes = chainOf({"a", "bcd"});
  const auto greater = chainOf({"ab", "ce"});

  EXPECT_TRUE(StringTraits<IOBufChain>::isEqual(segmented, sameBytes));
  EXPECT_FALSE(StringTraits<IOBufChain>::isLess(segmented, sameBytes));
  EXPECT_TRUE(StringTraits<IOBufChain>::isLess(segmented, greater));
  EXPECT_TRUE(
      StringTraits<IOBufChain>::isEmpty(
          StringTraits<IOBufChain>::fromStringLiteral("")));
}

TEST_F(BinaryProtocolTest, ChainReaderHandlesSplitFieldHeaderAndPrimitive) {
  const std::array<uint8_t, 8> wireBytes{
      static_cast<uint8_t>(TType::T_I32), 0, 7, 1, 2, 3, 4, 0};
  auto allocation = std::make_shared<TestAllocation>(wireBytes.size());
  IOBufChain wire;
  for (size_t i = 0; i < wireBytes.size(); ++i) {
    const auto byte = static_cast<char>(wireBytes[i]);
    wire.append(
        makeManagedSlice(allocation, i, std::string_view{&byte, sizeof(byte)}));
  }
  ASSERT_EQ(wireBytes.size(), wire.chainElements());

  BinaryProtocolChainReader reader;
  reader.setInput(&wire);
  std::string fieldName;
  TType fieldType;
  int16_t fieldId;
  reader.readFieldBegin(fieldName, fieldType, fieldId);
  EXPECT_EQ(TType::T_I32, fieldType);
  EXPECT_EQ(7, fieldId);
  int32_t value;
  reader.readI32(value);
  EXPECT_EQ(0x01020304, value);
  reader.readFieldBegin(fieldName, fieldType, fieldId);
  EXPECT_EQ(TType::T_STOP, fieldType);
}

TEST_F(BinaryProtocolTest, ChainReaderHandlesSplitFloatingPointVectors) {
  const std::array<uint8_t, 24> wireBytes{
      0x3f, 0x80, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x3f, 0xf0, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
  auto allocation = std::make_shared<TestAllocation>(wireBytes.size());
  IOBufChain wire;
  for (size_t i = 0; i < wireBytes.size(); ++i) {
    const auto byte = static_cast<char>(wireBytes[i]);
    wire.append(
        makeManagedSlice(allocation, i, std::string_view{&byte, sizeof(byte)}));
  }

  BinaryProtocolChainReader reader;
  reader.setInput(&wire);
  std::array<float, 2> floats{};
  std::array<double, 2> doubles{};
  reader.readArithmeticVector(floats.data(), floats.size());
  reader.readArithmeticVector(doubles.data(), doubles.size());

  EXPECT_EQ((std::array<float, 2>{1.0, -2.0}), floats);
  EXPECT_EQ((std::array<double, 2>{1.0, -2.0}), doubles);
}

TEST_F(BinaryProtocolTest, ChainReaderSharesManagedBinarySegments) {
  auto allocation = std::make_shared<TestAllocation>(32);
  IOBufChain wire;
  wire.append(makeManagedSlice(allocation, 0, std::string_view{"\0", 1}));
  wire.append(makeManagedSlice(allocation, 2, std::string_view{"\0", 1}));
  wire.append(makeManagedSlice(allocation, 4, std::string_view{"\0", 1}));
  wire.append(makeManagedSlice(allocation, 6, std::string_view{"\6", 1}));

  std::array<const uint8_t*, 3> payloadAddresses{};
  size_t payloadIndex = 0;
  for (const auto part : std::array<std::string_view, 3>{"ab", "cd", "ef"}) {
    auto buffer = makeManagedSlice(allocation, 8 + payloadIndex * 4, part);
    payloadAddresses[payloadIndex++] = buffer->data();
    wire.append(std::move(buffer));
  }
  ASSERT_EQ(7, wire.chainElements());

  BinaryProtocolChainReader reader;
  reader.setInput(&wire);
  IOBufChain output;
  reader.readBinary(output);

  EXPECT_EQ("abcdef", chainString(output));
  ASSERT_EQ(payloadAddresses.size(), output.chainElements());
  payloadIndex = 0;
  for (const auto& buffer : output) {
    EXPECT_EQ(payloadAddresses[payloadIndex++], buffer.data());
  }
  EXPECT_EQ(0, allocation->releases);

  wire = IOBufChain{};
  EXPECT_EQ(4, allocation->releases);
  output = IOBufChain{};
  EXPECT_EQ(7, allocation->releases);
}

TEST_F(BinaryProtocolTest, ChainReaderSharesManagedBinarySegmentsWithIOBuf) {
  auto allocation = std::make_shared<TestAllocation>(32);
  IOBufChain wire;
  wire.append(makeManagedSlice(allocation, 0, std::string_view{"\0", 1}));
  wire.append(makeManagedSlice(allocation, 2, std::string_view{"\0", 1}));
  wire.append(makeManagedSlice(allocation, 4, std::string_view{"\0", 1}));
  wire.append(makeManagedSlice(allocation, 6, std::string_view{"\6", 1}));

  std::array<const uint8_t*, 3> payloadAddresses{};
  size_t payloadIndex = 0;
  for (const auto part : std::array<std::string_view, 3>{"ab", "cd", "ef"}) {
    auto buffer = makeManagedSlice(allocation, 8 + payloadIndex * 4, part);
    payloadAddresses[payloadIndex++] = buffer->data();
    wire.append(std::move(buffer));
  }

  BinaryProtocolChainReader reader;
  reader.setInput(&wire);
  folly::IOBuf output;
  reader.readBinary(output);

  EXPECT_EQ(6, output.computeChainDataLength());
  ASSERT_EQ(payloadAddresses.size(), output.countChainElements());
  const auto* buffer = &output;
  for (const auto* address : payloadAddresses) {
    EXPECT_EQ(address, buffer->data());
    buffer = buffer->next();
  }
  EXPECT_EQ(&output, buffer);
  EXPECT_EQ(0, allocation->releases);

  wire = IOBufChain{};
  EXPECT_EQ(4, allocation->releases);
  output = folly::IOBuf{};
  EXPECT_EQ(7, allocation->releases);
}

} // namespace
