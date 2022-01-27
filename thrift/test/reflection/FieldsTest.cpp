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

#include <thrift/test/reflection/gen-cpp2/reflection_types.h>

#include <typeindex>

#include <boost/mp11.hpp>
#include <folly/portability/GTest.h>
#include <thrift/lib/cpp2/type/Tag.h>

using apache::thrift::detail::st::struct_private_access;

namespace apache::thrift::type {
static_assert(
    std::is_same_v<
        struct_private_access::fields<test_cpp2::cpp_reflection::struct3>,
        fields<
            field_t<2, i32_t>,
            field_t<1, string_t>,
            field_t<3, enum_t<::test_cpp2::cpp_reflection::enum1>>,
            field_t<4, enum_t<::test_cpp2::cpp_reflection::enum2>>,
            field_t<5, union_t<::test_cpp2::cpp_reflection::union1>>,
            field_t<6, union_t<::test_cpp2::cpp_reflection::union2>>,
            field_t<7, struct_t<::test_cpp2::cpp_reflection::struct1>>,
            field_t<8, union_t<::test_cpp2::cpp_reflection::union2>>,
            field_t<9, list<i32_t>>,
            field_t<10, list<string_t>>,
            field_t<11, list<string_t>>,
            field_t<12, list<struct_t<::test_cpp2::cpp_reflection::structA>>>,
            field_t<13, set<i32_t>>,
            field_t<14, set<string_t>>,
            field_t<15, set<string_t>>,
            field_t<16, set<struct_t<::test_cpp2::cpp_reflection::structB>>>,
            field_t<
                17,
                map<string_t, struct_t<::test_cpp2::cpp_reflection::structA>>>,
            field_t<
                18,
                map<string_t, struct_t<::test_cpp2::cpp_reflection::structB>>>,
            field_t<19, map<binary_t, binary_t>>>>);

struct ExtractFieldsInfo {
  std::vector<int> ids;
  std::vector<std::type_index> tags;

  template <int Id, class Tag>
  void operator()(field_t<Id, Tag>) {
    ids.push_back(Id);
    tags.emplace_back(typeid(Tag));
  }
};

TEST(Fields, for_each) {
  std::vector<int> expectedIds = {
      2, 1, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19};
  std::vector<std::type_index> expectedTags;
  expectedTags.emplace_back(typeid(i32_t));
  expectedTags.emplace_back(typeid(string_t));
  expectedTags.emplace_back(typeid(enum_t<::test_cpp2::cpp_reflection::enum1>));
  expectedTags.emplace_back(typeid(enum_t<::test_cpp2::cpp_reflection::enum2>));
  expectedTags.emplace_back(
      typeid(union_t<::test_cpp2::cpp_reflection::union1>));
  expectedTags.emplace_back(
      typeid(union_t<::test_cpp2::cpp_reflection::union2>));
  expectedTags.emplace_back(
      typeid(struct_t<::test_cpp2::cpp_reflection::struct1>));
  expectedTags.emplace_back(
      typeid(union_t<::test_cpp2::cpp_reflection::union2>));
  expectedTags.emplace_back(typeid(list<i32_t>));
  expectedTags.emplace_back(typeid(list<string_t>));
  expectedTags.emplace_back(typeid(list<string_t>));
  expectedTags.emplace_back(
      typeid(list<struct_t<::test_cpp2::cpp_reflection::structA>>));
  expectedTags.emplace_back(typeid(set<i32_t>));
  expectedTags.emplace_back(typeid(set<string_t>));
  expectedTags.emplace_back(typeid(set<string_t>));
  expectedTags.emplace_back(
      typeid(set<struct_t<::test_cpp2::cpp_reflection::structB>>));
  expectedTags.emplace_back(
      typeid(map<string_t, struct_t<::test_cpp2::cpp_reflection::structA>>));
  expectedTags.emplace_back(
      typeid(map<string_t, struct_t<::test_cpp2::cpp_reflection::structB>>));
  expectedTags.emplace_back(typeid(map<binary_t, binary_t>));

  ExtractFieldsInfo info;
  boost::mp11::mp_for_each<
      struct_private_access::fields<test_cpp2::cpp_reflection::struct3>>(info);
  EXPECT_EQ(info.ids, expectedIds);
  EXPECT_EQ(info.tags, expectedTags);
}
} // namespace apache::thrift::type
