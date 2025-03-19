/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 * Copyright (c) 2025 NVIDIA CORPORATION & AFFILIATES. All rights reserved.
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

#include <folly/CPortability.h>
#include <folly/portability/GTest.h>

#include <thrift/lib/cpp2/protocol/BinaryProtocol.h>
#include <thrift/lib/cpp2/protocol/detail/protocol_methods.h>
#include <random>
#include <algorithm>

using namespace apache::thrift;
using namespace apache::thrift::protocol;

namespace {

class BinaryProtocolTest : public testing::Test {};

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

template <typename T>
class FundamentalTypeListBinaryProtocolTest : public BinaryProtocolTest {
};

using FundamentalTypes = ::testing::Types<int64_t, int32_t, int16_t, int8_t, float_t, double_t>;
TYPED_TEST_SUITE(FundamentalTypeListBinaryProtocolTest, FundamentalTypes);

TYPED_TEST(FundamentalTypeListBinaryProtocolTest, readBigListFixedInt) {
  for(int randomInit = 0; randomInit <= 1; ++randomInit) {
    for(int i = 1; i < 256; ++i) {
      auto w = BinaryProtocolWriter();
      auto q = folly::IOBufQueue();
      w.setOutput(&q);
      std::vector<TypeParam> intList(i);
      // Specify the engine and distribution.
      if (randomInit) {
        std::mt19937 mersenne_engine (1337);  // Generates random integers
        if constexpr (std::is_floating_point_v<TypeParam>) {
          std::uniform_real_distribution<TypeParam> dist {};
          std::generate(intList.begin(), intList.end(), [&]() {
              return dist(mersenne_engine);
          });
        } else {
          std::uniform_int_distribution<TypeParam> dist {};
          std::generate(intList.begin(), intList.end(), [&]() {
              return dist(mersenne_engine);
          });
        }

      } else {
        TypeParam t = 0;
        std::generate_n(intList.begin(), intList.size(), [&]() {
          return ++t;
        });
      }
      using prot_method_integral = ::apache::thrift::detail::pm::protocol_methods<::apache::thrift::type_class::list<::apache::thrift::type_class::integral>, ::std::vector<TypeParam>>;
      using prot_method_float = ::apache::thrift::detail::pm::protocol_methods<::apache::thrift::type_class::list<::apache::thrift::type_class::floating_point>, ::std::vector<TypeParam>>;

      if constexpr (std::is_floating_point_v<TypeParam>) {
        prot_method_float::write(w, intList);
      } else {
        prot_method_integral::write(w, intList);
      }

      auto r = BinaryProtocolReader();
      r.setInput(q.front());
      std::vector<TypeParam> outList;
      outList.resize(intList.size());
      if constexpr (std::is_floating_point_v<TypeParam>) {
        prot_method_float::read(r, outList);
      } else {
        prot_method_integral::read(r, outList);
      }
      ASSERT_EQ(intList, outList);
    }
  }
}

} // namespace
