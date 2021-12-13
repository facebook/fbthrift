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

#include <thrift/lib/cpp2/op/Hash.h>

#include <folly/portability/GTest.h>
#include <thrift/lib/cpp2/protocol/test/gen-cpp2/Module_types_custom_protocol.h>

namespace apache::thrift::op {
namespace {

TEST(TypedOpTest, HashType) {
  test::OneOfEach value;
  using Tag = type::struct_t<test::OneOfEach>;

  std::unordered_set<std::size_t> s;
  auto check_and_add = [&s](auto tag, const auto& v) {
    using Tag = decltype(tag);
    EXPECT_EQ(s.count(hash<Tag>(v)), 0);
    s.insert(hash<Tag>(v));
    EXPECT_EQ(s.count(hash<Tag>(v)), 1);
  };

  for (auto i = 0; i < 10; i++) {
    value.myI32_ref() = i + 100;
    check_and_add(Tag{}, value);
    value.myList_ref() = {std::to_string(i + 200)};
    check_and_add(Tag{}, value);
    value.mySet_ref() = {std::to_string(i + 300)};
    check_and_add(Tag{}, value);
    value.myMap_ref() = {{std::to_string(i + 400), 0}};
    check_and_add(Tag{}, value);

    check_and_add(type::i32_t{}, *value.myI32_ref());
    check_and_add(type::list<type::string_t>{}, *value.myList_ref());
    check_and_add(type::set<type::string_t>{}, *value.mySet_ref());
    check_and_add(type::map<type::string_t, type::i64_t>{}, *value.myMap_ref());
  }
}

TEST(TypedOpTest, HashDouble) {
  EXPECT_EQ(hash<type::double_t>(-0.0), hash<type::double_t>(+0.0));
  EXPECT_EQ(
      hash<type::double_t>(std::numeric_limits<double>::quiet_NaN()),
      hash<type::double_t>(std::numeric_limits<double>::quiet_NaN()));
}

} // namespace
} // namespace apache::thrift::op
