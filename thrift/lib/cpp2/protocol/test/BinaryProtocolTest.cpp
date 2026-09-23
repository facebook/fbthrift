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

#include <string_view>
#include <type_traits>

#include <thrift/lib/cpp2/IOBufChain.h>
#include <thrift/lib/cpp2/protocol/BinaryProtocol.h>
#include <thrift/lib/cpp2/protocol/CompactProtocol.h>
#include <thrift/lib/cpp2/protocol/Cpp2Ops.h>
#include <thrift/lib/cpp2/protocol/detail/protocol_methods.h>

using namespace apache::thrift;
using namespace apache::thrift::protocol;

namespace {

class BinaryProtocolTest : public testing::Test {};

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

} // namespace
