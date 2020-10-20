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

#include <folly/Demangle.h>
#include <folly/portability/GTest.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>
#include <thrift/lib/cpp2/reflection/populator.h>
#include <thrift/test/testset/gen-cpp2/gen_struct_all_fatal_types.h>
#include <thrift/test/testset/gen-cpp2/gen_struct_all_for_each_field.h>

using namespace apache::thrift;

TEST(SerializationRoundTripTest, TestAll) {
  auto func = [](auto&&, auto&& obj1, auto&& obj2) {
    std::mt19937 rng;
    populator::populate(obj1.ensure(), {}, rng);
    auto data = CompactSerializer::serialize<std::string>(*obj1);
    CompactSerializer::deserialize(data, obj2.ensure());
    EXPECT_EQ(obj1, obj2) << folly::demangle(typeid(*obj1).name());
  };
  {
    test::struct_all foo, bar;
    for_each_field(foo, bar, func);
  }
  {
    test::union_all foo, bar;
    for_each_field(foo, bar, func);
  }
}
