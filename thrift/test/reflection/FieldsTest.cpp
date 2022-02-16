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
#include <thrift/lib/cpp2/type/Get.h>
#include <thrift/lib/cpp2/type/Tag.h>
#include <thrift/lib/cpp2/type/Testing.h>

using apache::thrift::detail::st::struct_private_access;
using apache::thrift::test::same_tag;

namespace apache::thrift::type {
static_assert(
    same_tag<
        struct_private_access::fields<test_cpp2::cpp_reflection::struct3>,
        fields<
            field_t<FieldId{2}, i32_t>,
            field_t<FieldId{1}, string_t>,
            field_t<FieldId{3}, enum_t<::test_cpp2::cpp_reflection::enum1>>,
            field_t<FieldId{4}, enum_t<::test_cpp2::cpp_reflection::enum2>>,
            field_t<FieldId{5}, union_t<::test_cpp2::cpp_reflection::union1>>,
            field_t<FieldId{6}, union_t<::test_cpp2::cpp_reflection::union2>>,
            field_t<FieldId{7}, struct_t<::test_cpp2::cpp_reflection::struct1>>,
            field_t<FieldId{8}, union_t<::test_cpp2::cpp_reflection::union2>>,
            field_t<FieldId{9}, list<i32_t>>,
            field_t<FieldId{10}, list<string_t>>,
            field_t<FieldId{11}, list<string_t>>,
            field_t<
                FieldId{12},
                list<struct_t<::test_cpp2::cpp_reflection::structA>>>,
            field_t<FieldId{13}, set<i32_t>>,
            field_t<FieldId{14}, set<string_t>>,
            field_t<FieldId{15}, set<string_t>>,
            field_t<
                FieldId{16},
                set<struct_t<::test_cpp2::cpp_reflection::structB>>>,
            field_t<
                FieldId{17},
                map<string_t, struct_t<::test_cpp2::cpp_reflection::structA>>>,
            field_t<
                FieldId{18},
                map<string_t, struct_t<::test_cpp2::cpp_reflection::structB>>>,
            field_t<FieldId{20}, map<binary_t, binary_t>>>>);

struct ExtractFieldsInfo {
  std::vector<int> ids;
  std::vector<std::type_index> tags;

  template <FieldId Id, class Tag>
  void operator()(field_t<Id, Tag>) {
    ids.push_back(int(Id));
    tags.emplace_back(typeid(Tag));
  }
};

TEST(Fields, for_each) {
  const std::vector<int> expectedIds = {
      2, 1, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 20};
  const std::vector<std::type_index> expectedTags = {
      typeid(i32_t),
      typeid(string_t),
      typeid(enum_t<::test_cpp2::cpp_reflection::enum1>),
      typeid(enum_t<::test_cpp2::cpp_reflection::enum2>),
      typeid(union_t<::test_cpp2::cpp_reflection::union1>),
      typeid(union_t<::test_cpp2::cpp_reflection::union2>),
      typeid(struct_t<::test_cpp2::cpp_reflection::struct1>),
      typeid(union_t<::test_cpp2::cpp_reflection::union2>),
      typeid(list<i32_t>),
      typeid(list<string_t>),
      typeid(list<string_t>),
      typeid(list<struct_t<::test_cpp2::cpp_reflection::structA>>),
      typeid(set<i32_t>),
      typeid(set<string_t>),
      typeid(set<string_t>),
      typeid(set<struct_t<::test_cpp2::cpp_reflection::structB>>),
      typeid(map<string_t, struct_t<::test_cpp2::cpp_reflection::structA>>),
      typeid(map<string_t, struct_t<::test_cpp2::cpp_reflection::structB>>),
      typeid(map<binary_t, binary_t>)};

  ExtractFieldsInfo info;
  boost::mp11::mp_for_each<
      struct_private_access::fields<test_cpp2::cpp_reflection::struct3>>(info);
  EXPECT_EQ(info.ids, expectedIds);
  EXPECT_EQ(info.tags, expectedTags);
}

struct Emplacer {
  test_cpp2::cpp_reflection::struct3& s;

  template <FieldId Id, class Tag>
  void operator()(field_t<Id, Tag>) {
    op::get<Id>(s).emplace();
  }
};

TEST(Fields, get) {
  test_cpp2::cpp_reflection::struct3 s;

  s.fieldA() = 10;
  EXPECT_EQ(op::get<FieldId{2}>(s), 10);
  op::get<FieldId{2}>(s) = 20;
  EXPECT_EQ(*s.fieldA(), 20);
  EXPECT_TRUE(
      (same_tag<decltype(s.fieldA()), decltype(op::get<FieldId{2}>(s))>));

  s.fieldE()->ui_ref() = 10;
  EXPECT_EQ(op::get<FieldId{5}>(s)->ui_ref(), 10);
  op::get<FieldId{5}>(s)->us_ref() = "20";
  EXPECT_EQ(s.fieldE()->us_ref(), "20");
  EXPECT_TRUE(
      (same_tag<decltype(s.fieldE()), decltype(op::get<FieldId{5}>(s))>));

  boost::mp11::mp_for_each<
      struct_private_access::fields<test_cpp2::cpp_reflection::struct3>>(
      Emplacer{s});

  EXPECT_EQ(*s.fieldA(), 0);
  EXPECT_FALSE(s.fieldE()->us_ref());
}

template <class StructTag>
struct CheckTypeTag {
  template <FieldId Id, class Tag>
  void operator()(field_t<Id, Tag>) {
    static_assert(same_tag<field_tag_t<StructTag, Id>, field_t<Id, Tag>>);
  }
};

TEST(Fields, field_tag_t) {
  using StructTag = struct_t<test_cpp2::cpp_reflection::struct3>;
  static_assert(same_tag<field_tag_t<StructTag, FieldId{0}>, void>);
  static_assert(same_tag<
                field_tag_t<StructTag, FieldId{1}>,
                field_t<FieldId{1}, string_t>>);
  static_assert(
      same_tag<field_tag_t<StructTag, FieldId{2}>, field_t<FieldId{2}, i32_t>>);
  static_assert(same_tag<field_tag_t<StructTag, FieldId{19}>, void>);
  static_assert(same_tag<
                field_tag_t<StructTag, FieldId{20}>,
                field_t<FieldId{20}, map<binary_t, binary_t>>>);
  static_assert(same_tag<field_tag_t<StructTag, FieldId{21}>, void>);
  boost::mp11::mp_for_each<
      struct_private_access::fields<test_cpp2::cpp_reflection::struct3>>(
      CheckTypeTag<StructTag>{});
}
} // namespace apache::thrift::type
