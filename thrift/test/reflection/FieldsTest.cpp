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
using field_id_t = FieldIdTag<type::field_id_v<FieldTag>>;

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
        struct_private_access::fields<
            ::test_cpp2::cpp_reflection::StructWithAdaptedField>,
        fields<
            field_t<FieldId{1}, string_t>,
            field_t<
                FieldId{2},
                adapted<
                    test::TemplatedTestAdapter,
                    struct_t<::test_cpp2::cpp_reflection::IntStruct>>>,
            field_t<
                FieldId{3},
                adapted<
                    test::AdapterWithContext,
                    struct_t<::test_cpp2::cpp_reflection::IntStruct>>>,
            field_t<FieldId{4}, adapted<test::TemplatedTestAdapter, i64_t>>,
            field_t<FieldId{5}, adapted<test::TemplatedTestAdapter, i64_t>>,
            field_t<
                FieldId{6},
                adapted<
                    test::AdapterWithContext,
                    adapted<test::TemplatedTestAdapter, i64_t>>>>>);

static_assert(
    ordinal<struct_t<::test_cpp2::cpp_reflection::StructWithAdaptedField>>(
        FieldId{1}) == FieldOrdinal{1});
static_assert(
    ordinal<struct_t<::test_cpp2::cpp_reflection::StructWithAdaptedField>>(
        FieldId{2}) == FieldOrdinal{2});
static_assert(
    ordinal<struct_t<::test_cpp2::cpp_reflection::StructWithAdaptedField>>(
        FieldId{3}) == FieldOrdinal{3});
static_assert(
    ordinal<struct_t<::test_cpp2::cpp_reflection::StructWithAdaptedField>>(
        FieldId{4}) == FieldOrdinal{4});
static_assert(
    ordinal<struct_t<::test_cpp2::cpp_reflection::StructWithAdaptedField>>(
        FieldId{5}) == FieldOrdinal{5});
static_assert(
    ordinal<struct_t<::test_cpp2::cpp_reflection::StructWithAdaptedField>>(
        FieldId{6}) == FieldOrdinal{6});

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
  const std::vector<type::field_id_u_t> expectedIds = {
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

  std::vector<type::field_id_u_t> ids;
  std::vector<std::type_index> tags;
  ForEach<struct_private_access::fields<test_cpp2::cpp_reflection::struct3>>::
      field([&](auto id, auto tag) {
        ids.emplace_back(id.value);
        tags.emplace_back(typeid(tag));
      });
  EXPECT_EQ(ids, expectedIds);
  EXPECT_EQ(tags, expectedTags);

  for (uint16_t idx = 0; idx < expectedIds.size(); ++idx) {
    EXPECT_EQ(
        ordinal<type::struct_t<test_cpp2::cpp_reflection::struct3>>(
            FieldId{expectedIds[idx]}),
        static_cast<FieldOrdinal>(idx + 1));
  }

  static_assert(
      ordinal<type::struct_t<test_cpp2::cpp_reflection::struct3>>(
          FieldId{19}) == FieldOrdinal{0});
}

TEST(FieldsTest, Get) {
  test_cpp2::cpp_reflection::struct3 s;
  using Tag = type::struct_t<test_cpp2::cpp_reflection::struct3>;
  EXPECT_EQ(
      &(*op::getById<Tag, FieldId{2}>(s)),
      &(*op::getByIdent<Tag, tag::fieldA>(s)));

  s.fieldA() = 10;
  EXPECT_EQ((op::getById<Tag, FieldId{2}>(s)), 10);
  op::getById<Tag, FieldId{2}>(s) = 20;
  EXPECT_EQ(*s.fieldA(), 20);
  test::
      same_tag<decltype(s.fieldA()), decltype(op::getById<Tag, FieldId{2}>(s))>;

  s.fieldE()->ui_ref() = 10;
  EXPECT_EQ((op::getById<Tag, FieldId{5}>(s)->ui_ref()), 10);
  op::getById<Tag, FieldId{5}>(s)->us_ref() = "20";
  EXPECT_EQ(s.fieldE()->us_ref(), "20");
  test::
      same_tag<decltype(s.fieldE()), decltype(op::getById<Tag, FieldId{5}>(s))>;

  ForEach<struct_private_access::fields<test_cpp2::cpp_reflection::struct3>>::
      id([&](auto id) {
        op::getById<Tag, FieldId{decltype(id)::value}>(s).emplace();
      });

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
        constexpr auto Id = FieldId{decltype(id)::value};
        using Tag = decltype(tag);
        test::same_tag<field_tag_by_id<StructTag, Id>, field_t<Id, Tag>>;
      });
}

TEST(FieldsTest, field_id_by_ordinal) {
  using StructTag = struct_t<test_cpp2::cpp_reflection::struct3>;
  test::same_tag<field_tag_by_ordinal<StructTag, FieldOrdinal{0}>, void>;
  EXPECT_EQ(field_size_v<StructTag>, 19);
  test::same_tag<field_tag_by_ordinal<StructTag, FieldOrdinal{20}>, void>;
  EXPECT_EQ((field_id_by_ordinal_v<StructTag, FieldOrdinal{1}>), FieldId{2});
  EXPECT_EQ((field_id_by_ordinal_v<StructTag, FieldOrdinal{2}>), FieldId{1});
  EXPECT_EQ((field_id_by_ordinal_v<StructTag, FieldOrdinal{3}>), FieldId{3});
  EXPECT_EQ((field_id_by_ordinal_v<StructTag, FieldOrdinal{19}>), FieldId{20});
}

TEST(UnionFieldsTest, Get) {
  test_cpp2::cpp_reflection::union1 u;
  using Tag = type::union_t<test_cpp2::cpp_reflection::union1>;

  EXPECT_THROW((*op::getById<Tag, FieldId{1}>(u)), bad_field_access);

  u.ui_ref() = 10;
  EXPECT_EQ((op::getById<Tag, FieldId{1}>(u)), 10);
  EXPECT_THROW((*op::getById<Tag, FieldId{2}>(u)), bad_field_access);
  test::
      same_tag<decltype(u.ui_ref()), decltype(op::getById<Tag, FieldId{1}>(u))>;
  EXPECT_EQ(
      &(*op::getById<Tag, FieldId{1}>(u)), &(*op::getByIdent<Tag, tag::ui>(u)));

  op::getById<Tag, FieldId{1}>(u) = 20;
  EXPECT_EQ(u.ui_ref(), 20);
  EXPECT_EQ((op::getById<Tag, FieldId{1}>(u)), 20);

  u.us_ref() = "foo";
  EXPECT_EQ((*op::getById<Tag, FieldId{3}>(u)), "foo");
  test::
      same_tag<decltype(u.us_ref()), decltype(op::getById<Tag, FieldId{3}>(u))>;
  EXPECT_THROW((*op::getById<Tag, FieldId{1}>(u)), bad_field_access);
}

template <
    class Struct,
    class Ordinal,
    class Id,
    class Ident,
    bool is_type_tag_unique,
    class TypeTag>
void checkField() {
  test::same_tag<Id, struct_private_access::field_id<Struct, Ordinal>>;
  test::same_tag<TypeTag, struct_private_access::type_tag<Struct, Ordinal>>;
  test::same_tag<Ident, struct_private_access::ident<Struct, Ordinal>>;
  test::same_tag<Ordinal, struct_private_access::ordinal<Struct, Id>>;
  test::same_tag<Ordinal, struct_private_access::ordinal<Struct, Ident>>;

  if constexpr (is_type_tag_unique) {
    test::same_tag<Ordinal, struct_private_access::ordinal<Struct, TypeTag>>;
  }

  using StructTag = struct_t<Struct>;
  namespace field = apache::thrift::field;
  test::same_tag<field::ordinal<StructTag, Ordinal>, Ordinal>;
  test::same_tag<field::id<StructTag, Ordinal>, Id>;
  test::same_tag<field::type_tag<StructTag, Ordinal>, TypeTag>;
  test::same_tag<field::ident<StructTag, Ordinal>, Ident>;

  test::same_tag<field::ordinal<StructTag, Id>, Ordinal>;
  test::same_tag<field::id<StructTag, Id>, Id>;
  test::same_tag<field::type_tag<StructTag, Id>, TypeTag>;
  test::same_tag<field::ident<StructTag, Id>, Ident>;

  if constexpr (is_type_tag_unique) {
    test::same_tag<field::ordinal<StructTag, TypeTag>, Ordinal>;
    test::same_tag<field::id<StructTag, TypeTag>, Id>;
    test::same_tag<field::type_tag<StructTag, TypeTag>, TypeTag>;
    test::same_tag<field::ident<StructTag, TypeTag>, Ident>;
  }

  test::same_tag<field::ordinal<StructTag, Ident>, Ordinal>;
  test::same_tag<field::id<StructTag, Ident>, Id>;
  test::same_tag<field::type_tag<StructTag, Ident>, TypeTag>;
  test::same_tag<field::ident<StructTag, Ident>, Ident>;
}

TEST(FieldsTest, UnifiedAPIs) {
  // clang-format off
  using test_cpp2::cpp_reflection::struct3;
  checkField<struct3, field_ordinal<0>,  void,         void,        true,  void>();
  checkField<struct3, field_ordinal<1>,  field_id<2>,  tag::fieldA, true,  i32_t>();
  checkField<struct3, field_ordinal<2>,  field_id<1>,  tag::fieldB, true,  string_t>();
  checkField<struct3, field_ordinal<3>,  field_id<3>,  tag::fieldC, true,  enum_t<::test_cpp2::cpp_reflection::enum1>>();
  checkField<struct3, field_ordinal<4>,  field_id<4>,  tag::fieldD, true,  enum_t<::test_cpp2::cpp_reflection::enum2>>();
  checkField<struct3, field_ordinal<5>,  field_id<5>,  tag::fieldE, true,  union_t<::test_cpp2::cpp_reflection::union1>>();
  checkField<struct3, field_ordinal<6>,  field_id<6>,  tag::fieldF, false, union_t<::test_cpp2::cpp_reflection::union2>>();
  checkField<struct3, field_ordinal<7>,  field_id<7>,  tag::fieldG, true,  struct_t<::test_cpp2::cpp_reflection::struct1>>();
  checkField<struct3, field_ordinal<8>,  field_id<8>,  tag::fieldH, false, union_t<::test_cpp2::cpp_reflection::union2>>();
  checkField<struct3, field_ordinal<9>,  field_id<9>,  tag::fieldI, true,  list<i32_t>>();
  checkField<struct3, field_ordinal<10>, field_id<10>, tag::fieldJ, true,  list<string_t>>();
  checkField<struct3, field_ordinal<11>, field_id<11>, tag::fieldK, true,  cpp_type<std::deque<std::string>, list<string_t>>>();
  checkField<struct3, field_ordinal<12>, field_id<12>, tag::fieldL, true,  list<struct_t<::test_cpp2::cpp_reflection::structA>>>();
  checkField<struct3, field_ordinal<13>, field_id<13>, tag::fieldM, true,  set<i32_t>>();
  checkField<struct3, field_ordinal<14>, field_id<14>, tag::fieldN, false, set<string_t>>();
  checkField<struct3, field_ordinal<15>, field_id<15>, tag::fieldO, false, set<string_t>>();
  checkField<struct3, field_ordinal<16>, field_id<16>, tag::fieldP, true,  set<struct_t<::test_cpp2::cpp_reflection::structB>>>();
  checkField<struct3, field_ordinal<17>, field_id<17>, tag::fieldQ, true,  map<string_t, struct_t<::test_cpp2::cpp_reflection::structA>>>();
  checkField<struct3, field_ordinal<18>, field_id<18>, tag::fieldR, true,  map<string_t, struct_t<::test_cpp2::cpp_reflection::structB>>>();
  checkField<struct3, field_ordinal<19>, field_id<20>, tag::fieldS, true,  map<binary_t, binary_t>>();
  // clang-format off
}

TEST(FieldsTest, NotFoundFieldInfo) {
  using Tag = struct_t<test_cpp2::cpp_reflection::struct3>;
  namespace field = apache::thrift::field;

  test::same_tag<field::ordinal<Tag, field_ordinal<0>>, field_ordinal<0>>;
  test::same_tag<field::id<Tag, field_ordinal<0>>, void>;
  test::same_tag<field::type_tag<Tag, field_ordinal<0>>, void>;
  test::same_tag<field::ident<Tag, field_ordinal<0>>, void>;

  test::same_tag<field::ordinal<Tag, field_id<200>>, field_ordinal<0>>;
  test::same_tag<field::id<Tag, field_id<200>>, void>;
  test::same_tag<field::type_tag<Tag, field_id<200>>, void>;
  test::same_tag<field::ident<Tag, field_id<200>>, void>;

  test::same_tag<field::ordinal<Tag, binary_t>, field_ordinal<0>>;
  test::same_tag<field::id<Tag, binary_t>, void>;
  test::same_tag<field::type_tag<Tag, binary_t>, void>;
  test::same_tag<field::ident<Tag, binary_t>, void>;

  test::same_tag<field::ordinal<Tag, tag::a>, field_ordinal<0>>;
  test::same_tag<field::id<Tag, tag::a>, void>;
  test::same_tag<field::type_tag<Tag, tag::a>, void>;
  test::same_tag<field::ident<Tag, tag::a>, void>;
}

} // namespace apache::thrift::type
