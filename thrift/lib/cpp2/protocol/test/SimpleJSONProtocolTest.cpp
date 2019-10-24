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

#include <folly/portability/GTest.h>

#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <thrift/lib/cpp2/protocol/SimpleJSONProtocol.h>
#include <thrift/lib/cpp2/protocol/test/gen-cpp2/Module_types_custom_protocol.h>

using namespace std;
using namespace folly;
using namespace apache::thrift;
using namespace apache::thrift::test;

namespace {

class SimpleJSONProtocolTest : public testing::Test {};

using S = SimpleJSONSerializer;

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
  using type = StructWithEmptyMap;
  const auto orig = type{};
  const auto serialized = S::serialize<string>(orig);
  LOG(INFO) << serialized;
  type deserialized;
  const auto size = S::deserialize(serialized, deserialized);
  EXPECT_EQ(serialized.size(), size);
  EXPECT_EQ(orig, deserialized);
}
