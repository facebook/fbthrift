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

#include <folly/Utility.h>
#include <folly/portability/GTest.h>
#include <thrift/lib/cpp2/BadFieldAccess.h>
#include <thrift/lib/cpp2/Thrift.h>
#include <thrift/lib/cpp2/op/Get.h>
#include <thrift/lib/cpp2/type/Field.h>
#include <thrift/lib/cpp2/type/Tag.h>
#include <thrift/lib/cpp2/type/Testing.h>

namespace apache::thrift::type {
using apache::thrift::detail::st::struct_private_access;

// A type tag for a given field id.
template <typename FieldTag>
using field_id_t = std::integral_constant<FieldId, type::field_id_v<FieldTag>>;

// Helper for visiting field metadata.
template <typename List>
struct ForEach;
template <typename... Tags>
struct ForEach<type::fields<Tags...>> {
  template <typename F>
  static void id(F&& fun) {
    (..., fun(field_id_t<Tags>{}));
  }
  template <typename F>
  static void tag(F&& fun) {
    (..., fun(type::field_type_tag<Tags>{}));
  }
  template <typename F>
  static void field(F&& fun) {
    (..., fun(field_id_t<Tags>{}, type::field_type_tag<Tags>{}));
  }
};

static_assert(
    test::same_tag<
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
            field_t<
                FieldId{11},
                cpp_type<std::deque<std::string>, list<string_t>>>,
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

TEST(FieldsTest, Fields) {
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
      typeid(cpp_type<std::deque<std::string>, list<string_t>>),
      typeid(list<struct_t<::test_cpp2::cpp_reflection::structA>>),
      typeid(set<i32_t>),
      typeid(set<string_t>),
      typeid(set<string_t>),
      typeid(set<struct_t<::test_cpp2::cpp_reflection::structB>>),
      typeid(map<string_t, struct_t<::test_cpp2::cpp_reflection::structA>>),
      typeid(map<string_t, struct_t<::test_cpp2::cpp_reflection::structB>>),
      typeid(map<binary_t, binary_t>)};

  std::vector<int> ids;
  std::vector<std::type_index> tags;
  ForEach<struct_private_access::fields<test_cpp2::cpp_reflection::struct3>>::
      field([&](auto id, auto tag) {
        ids.emplace_back(folly::to_underlying(id.value));
        tags.emplace_back(typeid(tag));
      });
  EXPECT_EQ(ids, expectedIds);
  EXPECT_EQ(tags, expectedTags);
}

TEST(FieldsTest, Get) {
  test_cpp2::cpp_reflection::struct3 s;

  s.fieldA() = 10;
  EXPECT_EQ(op::getById<FieldId{2}>(s), 10);
  op::getById<FieldId{2}>(s) = 20;
  EXPECT_EQ(*s.fieldA(), 20);
  test::same_tag<decltype(s.fieldA()), decltype(op::getById<FieldId{2}>(s))>;

  s.fieldE()->ui_ref() = 10;
  EXPECT_EQ(op::getById<FieldId{5}>(s)->ui_ref(), 10);
  op::getById<FieldId{5}>(s)->us_ref() = "20";
  EXPECT_EQ(s.fieldE()->us_ref(), "20");
  test::same_tag<decltype(s.fieldE()), decltype(op::getById<FieldId{5}>(s))>;

  ForEach<struct_private_access::fields<test_cpp2::cpp_reflection::struct3>>::
      id([&](auto id) { op::getById<decltype(id)::value>(s).emplace(); });

  EXPECT_EQ(*s.fieldA(), 0);
  EXPECT_FALSE(s.fieldE()->us_ref());
}

TEST(FieldsTest, field_tag_by_id) {
  using StructTag = struct_t<test_cpp2::cpp_reflection::struct3>;
  test::same_tag<field_tag_by_id<StructTag, FieldId{0}>, void>;
  test::same_tag<
      field_tag_by_id<StructTag, FieldId{1}>,
      field_t<FieldId{1}, string_t>>;
  test::same_tag<
      field_tag_by_id<StructTag, FieldId{2}>,
      field_t<FieldId{2}, i32_t>>;
  test::same_tag<field_tag_by_id<StructTag, FieldId{19}>, void>;
  test::same_tag<
      field_tag_by_id<StructTag, FieldId{20}>,
      field_t<FieldId{20}, map<binary_t, binary_t>>>;
  test::same_tag<field_tag_by_id<StructTag, FieldId{21}>, void>;
  ForEach<struct_private_access::fields<test_cpp2::cpp_reflection::struct3>>::
      field([](auto id, auto tag) {
        constexpr auto Id = decltype(id)::value;
        using Tag = decltype(tag);
        test::same_tag<field_tag_by_id<StructTag, Id>, field_t<Id, Tag>>;
      });
}

TEST(UnionFieldsTest, Get) {
  test_cpp2::cpp_reflection::union1 u;

  EXPECT_THROW(*op::getById<FieldId{1}>(u), bad_field_access);

  u.ui_ref() = 10;
  EXPECT_EQ(op::getById<FieldId{1}>(u), 10);
  EXPECT_THROW(*op::getById<FieldId{2}>(u), bad_field_access);
  test::same_tag<decltype(u.ui_ref()), decltype(op::getById<FieldId{1}>(u))>;

  op::getById<FieldId{1}>(u) = 20;
  EXPECT_EQ(u.ui_ref(), 20);
  EXPECT_EQ(op::getById<FieldId{1}>(u), 20);

  u.us_ref() = "foo";
  EXPECT_EQ(op::getById<FieldId{3}>(u), "foo");
  test::same_tag<decltype(u.us_ref()), decltype(op::getById<FieldId{3}>(u))>;
  EXPECT_THROW(*op::getById<FieldId{1}>(u), bad_field_access);
}

} // namespace apache::thrift::type
