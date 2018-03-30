/*
 * Copyright 2016 Facebook, Inc.
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

#include <thrift/lib/cpp/protocol/TCompactProtocol.h>
#include <thrift/lib/cpp/transport/TBufferTransports.h>
#include <thrift/lib/cpp/util/ThriftSerializer.h>
#include <thrift/lib/cpp2/protocol/CompactV1Protocol.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <thrift/lib/cpp2/protocol/test/gen-cpp/Module_types.h>
#include <thrift/lib/cpp2/protocol/test/gen-cpp2/Module_types_custom_protocol.h>

using namespace std;
using namespace folly;
using namespace apache::thrift;
using namespace apache::thrift::protocol;
using namespace apache::thrift::transport;

namespace {

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

using CompactV1Serializer =
    Serializer<CompactV1ProtocolReader, CompactV1ProtocolWriter>;

class CompactV1ProtocolTest : public testing::Test {};

} // namespace

TEST_F(CompactV1ProtocolTest, double_write_cpp1_read_cpp2) {
  util::ThriftSerializerCompactDeprecated<> cpp1_serializer;
  const auto orig = cpp1::OneOfEach{};
  const auto serialized =
      returning([&](string& _) { cpp1_serializer.serialize(orig, &_); });
  const auto deserialized_size =
      returning([&](tuple<cpp2::OneOfEach, uint32_t>& _) {
        get<1>(_) = CompactV1Serializer::deserialize(serialized, get<0>(_));
      });
  EXPECT_EQ(serialized.size(), get<1>(deserialized_size));
  EXPECT_EQ(orig.myDouble, get<0>(deserialized_size).myDouble);
}

TEST_F(CompactV1ProtocolTest, double_write_cpp2_read_cpp1) {
  util::ThriftSerializerCompactDeprecated<> cpp1_serializer;
  const auto orig = cpp2::OneOfEach{};
  const auto serialized = CompactV1Serializer::serialize<std::string>(orig);
  const auto deserialized_size =
      returning([&](tuple<cpp1::OneOfEach, uint32_t>& _) {
        get<1>(_) = cpp1_serializer.deserialize(serialized, &get<0>(_));
      });
  EXPECT_EQ(serialized.size(), get<1>(deserialized_size));
  EXPECT_EQ(orig.myDouble, get<0>(deserialized_size).myDouble);
}
