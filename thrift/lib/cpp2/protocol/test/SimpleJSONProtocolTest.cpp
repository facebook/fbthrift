/*
 * Copyright 2017 Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>

#include <thrift/lib/cpp/protocol/TSimpleJSONProtocol.h>
#include <thrift/lib/cpp/util/ThriftSerializer.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <thrift/lib/cpp2/protocol/SimpleJSONProtocol.h>
#include <thrift/lib/cpp2/protocol/test/gen-cpp/Module_types.h>
#include <thrift/lib/cpp2/protocol/test/gen-cpp2/Module_types_custom_protocol.h>

using namespace std;
using namespace folly;
using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;
using namespace apache::thrift::util;

namespace {

class SimpleJSONProtocolTest : public testing::Test {};

using S1 = ThriftSerializerSimpleJson<>;
using S2 = SimpleJSONSerializer;

template <typename T>
struct action_traits_impl;
template <typename C, typename A>
struct action_traits_impl<void (C::*)(A&) const> {
  using arg_type = A;
};
template <typename C, typename A>
struct action_traits_impl<void (C::*)(A&)> {
  using arg_type = A;
};
template <typename F>
using action_traits = action_traits_impl<decltype(&F::operator())>;
template <typename F>
using arg = typename action_traits<F>::arg_type;

template <typename F>
arg<F> returning(F&& f) {
  arg<F> ret;
  f(ret);
  return ret;
}

} // namespace

TEST_F(SimpleJSONProtocolTest, roundtrip_struct_with_empty_map_string_i64) {
  //  cpp2 -> str -> cpp2
  using type = cpp2::StructWithEmptyMap;
  const auto orig = type{};
  const auto serialized = S2::serialize<string>(orig);
  LOG(INFO) << serialized;
  type deserialized;
  const auto size = S2::deserialize(serialized, deserialized);
  EXPECT_EQ(serialized.size(), size);
  EXPECT_EQ(orig, deserialized);
}

TEST_F(
    SimpleJSONProtocolTest,
    super_roundtrip_struct_with_empty_map_string_i64) {
  //  cpp2 -> str -> cpp1 -> str -> cpp2
  using cpp1_type = cpp1::StructWithEmptyMap;
  using cpp2_type = cpp2::StructWithEmptyMap;
  S1 cpp1_serializer;
  const auto orig = cpp2_type{};
  const auto serialized_1 = S2::serialize<string>(orig);
  const auto deserialized_size_1 =
      returning([&](tuple<cpp1_type, uint32_t>& _) {
        get<1>(_) = cpp1_serializer.deserialize(serialized_1, &get<0>(_));
      });
  const auto deserialized_1 = get<0>(deserialized_size_1);
  const auto size_1 = get<1>(deserialized_size_1);
  EXPECT_EQ(serialized_1.size(), size_1);
  const auto serialized_2 = returning(
      [&](string& _) { cpp1_serializer.serialize(deserialized_1, &_); });
  EXPECT_EQ(serialized_1, serialized_2);
  const auto deserialized_size_2 =
      returning([&](tuple<cpp2_type, uint32_t>& _) {
        get<1>(_) = S2::deserialize(serialized_2, get<0>(_));
      });
  const auto deserialized_2 = get<0>(deserialized_size_2);
  const auto size_2 = get<1>(deserialized_size_2);
  EXPECT_EQ(serialized_2.size(), size_2);
  EXPECT_EQ(orig, deserialized_2);
}
