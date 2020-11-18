/*
 * Copyright (c) Facebook, Inc. and its affiliates.
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

#include <fatal/type/slice.h>
#include <folly/lang/Pretty.h>
#include <folly/portability/GTest.h>
#include <thrift/conformance/cpp2/AnyRegistry.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <thrift/lib/cpp2/reflection/populator.h>
#include <thrift/lib/cpp2/reflection/reflection.h>
#include <thrift/test/testset/gen-cpp2/testset_fatal_types.h>

namespace apache::thrift::test {

namespace {

using conformance::Any;
using conformance::AnyRegistry;
using conformance::StandardProtocol;

template <typename T>
class SerializtionRoundTripTest : public testing::Test {
 private:
  std::mt19937 rng_;

 protected:
  T populated() {
    T result;
    populator::populate(result, {}, rng_);
    return result;
  }

  template <typename SerializerT>
  void testSerializer() {
    SCOPED_TRACE(folly::pretty_name<T>());
    T obj1 = this->populated();
    auto data = SerializerT::template serialize<std::string>(obj1);
    T obj2;
    SerializerT::template deserialize(data, obj2);
    EXPECT_EQ(obj1, obj2);
  }

  template <StandardProtocol P>
  void testAny() {
    SCOPED_TRACE(folly::pretty_name<T>());
    T obj1 = this->populated();
    Any any = AnyRegistry::generated().store<P>(obj1);
    T obj2 = AnyRegistry::generated().load<T>(any);
    EXPECT_EQ(obj1, obj2);
  }
};

TYPED_TEST_CASE_P(SerializtionRoundTripTest);

TYPED_TEST_P(SerializtionRoundTripTest, Compact) {
  this->template testSerializer<CompactSerializer>();
}

TYPED_TEST_P(SerializtionRoundTripTest, Binary) {
  this->template testSerializer<BinarySerializer>();
}

TYPED_TEST_P(SerializtionRoundTripTest, SimpleJson) {
  this->template testSerializer<SimpleJSONSerializer>();
}

TYPED_TEST_P(SerializtionRoundTripTest, Compact_Any) {
  this->template testAny<StandardProtocol::Compact>();
}

TYPED_TEST_P(SerializtionRoundTripTest, Binary_Any) {
  this->template testAny<StandardProtocol::Binary>();
}

TYPED_TEST_P(SerializtionRoundTripTest, SimpleJson_Any) {
  this->template testAny<StandardProtocol::SimpleJson>();
}

REGISTER_TYPED_TEST_CASE_P(
    SerializtionRoundTripTest,
    Compact,
    Binary,
    SimpleJson,
    Compact_Any,
    Binary_Any,
    SimpleJson_Any);

using testset_info = reflect_module<testset::testset_tags::module>;

template <typename Ts>
struct to_gtest_types;
template <typename... Ts>
struct to_gtest_types<fatal::list<Ts...>> {
  using type = testing::Types<fatal::first<Ts>...>;
};
// Unfortuantely, the version of testing::Types we are using only supports up to
// 50 types, so we have to batch.
constexpr size_t kBatchSize = 50;
template <typename Ts>
using to_gtest_types_t = typename to_gtest_types<Ts>::type;
#define INST_TEST_BATCH(Type, Batch)                           \
  using testset_##Type##Batch = to_gtest_types_t<fatal::slice< \
      testset_info::Type,                                      \
      Batch * kBatchSize,                                      \
      (Batch + 1) * kBatchSize>>;                              \
  INSTANTIATE_TYPED_TEST_CASE_P(                               \
      Type##Batch, SerializtionRoundTripTest, testset_##Type##Batch)
#define INST_TEST_LAST(Type, Batch)                                          \
  using testset_##Type##Batch =                                              \
      to_gtest_types_t<fatal::tail<testset_info::Type, Batch * kBatchSize>>; \
  INSTANTIATE_TYPED_TEST_CASE_P(                                             \
      Type##Batch, SerializtionRoundTripTest, testset_##Type##Batch)

INST_TEST_BATCH(structs, 0);
INST_TEST_BATCH(structs, 1);
INST_TEST_BATCH(structs, 2);
INST_TEST_BATCH(structs, 3);
INST_TEST_BATCH(structs, 5);
INST_TEST_BATCH(structs, 6);
INST_TEST_BATCH(structs, 7);
INST_TEST_LAST(structs, 8);

INST_TEST_BATCH(unions, 0);
INST_TEST_LAST(unions, 1);

} // namespace
} // namespace apache::thrift::test
