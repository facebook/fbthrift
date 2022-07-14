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

TEST(FieldsTest, Get) {
  test_cpp2::cpp_reflection::struct3 s;
  using Tag = type::struct_t<test_cpp2::cpp_reflection::struct3>;
  EXPECT_EQ(&(*op::get<Tag, field_id<2>>(s)), &*s.fieldA());

  s.fieldA() = 10;
  EXPECT_EQ((op::get<Tag, field_id<2>>(s)), 10);
  op::get<Tag, field_id<2>>(s) = 20;
  EXPECT_EQ(*s.fieldA(), 20);
  test::same_tag<decltype(s.fieldA()), decltype(op::get<Tag, field_id<2>>(s))>;

  s.fieldE()->ui_ref() = 10;
  EXPECT_EQ((op::get<Tag, field_id<5>>(s)->ui_ref()), 10);
  op::get<Tag, field_id<5>>(s)->us_ref() = "20";
  EXPECT_EQ(s.fieldE()->us_ref(), "20");
  test::same_tag<decltype(s.fieldE()), decltype(op::get<Tag, field_id<5>>(s))>;
}

TEST(FieldsTest, field_id_by_ordinal) {
  using StructTag = struct_t<test_cpp2::cpp_reflection::struct3>;
  EXPECT_EQ(field_size_v<StructTag>, 19);
}

TEST(UnionFieldsTest, Get) {
  test_cpp2::cpp_reflection::union1 u;
  using Tag = type::union_t<test_cpp2::cpp_reflection::union1>;

  EXPECT_THROW((*op::get<Tag, field_id<1>>(u)), bad_field_access);

  u.ui_ref() = 10;
  EXPECT_EQ((op::get<Tag, field_id<1>>(u)), 10);
  EXPECT_THROW((*op::get<Tag, field_id<2>>(u)), bad_field_access);
  test::same_tag<decltype(u.ui_ref()), decltype(op::get<Tag, field_id<1>>(u))>;
  EXPECT_EQ(&(*op::get<Tag, field_id<1>>(u)), &*u.ui_ref());

  op::get<Tag, field_id<1>>(u) = 20;
  EXPECT_EQ(u.ui_ref(), 20);
  EXPECT_EQ((op::get<Tag, field_id<1>>(u)), 20);

  u.us_ref() = "foo";
  EXPECT_EQ((*op::get<Tag, field_id<3>>(u)), "foo");
  test::same_tag<decltype(u.us_ref()), decltype(op::get<Tag, field_id<3>>(u))>;
  EXPECT_THROW((*op::get<Tag, field_id<1>>(u)), bad_field_access);
}

template <
    class Struct,
    class Ordinal,
    class Id,
    class Ident,
    bool is_type_tag_unique,
    class TypeTag,
    class FieldTag>
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
  test::same_tag<type::get_field_ordinal<StructTag, Ordinal>, Ordinal>;
  test::same_tag<type::get_field_id<StructTag, Ordinal>, Id>;
  test::same_tag<type::get_field_type_tag<StructTag, Ordinal>, TypeTag>;
  test::same_tag<type::get_field_ident<StructTag, Ordinal>, Ident>;
  test::same_tag<type::get_field_tag<StructTag, Ordinal>, FieldTag>;

  test::same_tag<type::get_field_ordinal<StructTag, Id>, Ordinal>;
  test::same_tag<type::get_field_id<StructTag, Id>, Id>;
  test::same_tag<type::get_field_type_tag<StructTag, Id>, TypeTag>;
  test::same_tag<type::get_field_ident<StructTag, Id>, Ident>;
  test::same_tag<type::get_field_tag<StructTag, Id>, FieldTag>;

  if constexpr (is_type_tag_unique) {
    test::same_tag<type::get_field_ordinal<StructTag, TypeTag>, Ordinal>;
    test::same_tag<type::get_field_id<StructTag, TypeTag>, Id>;
    test::same_tag<type::get_field_type_tag<StructTag, TypeTag>, TypeTag>;
    test::same_tag<type::get_field_ident<StructTag, TypeTag>, Ident>;
    test::same_tag<type::get_field_tag<StructTag, TypeTag>, FieldTag>;
  }

  test::same_tag<type::get_field_ordinal<StructTag, Ident>, Ordinal>;
  test::same_tag<type::get_field_id<StructTag, Ident>, Id>;
  test::same_tag<type::get_field_type_tag<StructTag, Ident>, TypeTag>;
  test::same_tag<type::get_field_ident<StructTag, Ident>, Ident>;
  test::same_tag<type::get_field_tag<StructTag, Ident>, FieldTag>;

  test::same_tag<type::get_field_ordinal<StructTag, FieldTag>, Ordinal>;
  test::same_tag<type::get_field_id<StructTag, FieldTag>, Id>;
  test::same_tag<type::get_field_type_tag<StructTag, FieldTag>, TypeTag>;
  test::same_tag<type::get_field_ident<StructTag, FieldTag>, Ident>;
  test::same_tag<type::get_field_tag<StructTag, FieldTag>, FieldTag>;
}

TEST(FieldsTest, UnifiedAPIs) {
  using test_cpp2::cpp_reflection::struct3;

  using TypeTag0 = void;
  using TypeTag1 = i32_t;
  using TypeTag2 = string_t;
  using TypeTag3 = enum_t<::test_cpp2::cpp_reflection::enum1>;
  using TypeTag4 = enum_t<::test_cpp2::cpp_reflection::enum2>;
  using TypeTag5 = union_t<::test_cpp2::cpp_reflection::union1>;
  using TypeTag6 = union_t<::test_cpp2::cpp_reflection::union2>;
  using TypeTag7 = struct_t<::test_cpp2::cpp_reflection::struct1>;
  using TypeTag8 = union_t<::test_cpp2::cpp_reflection::union2>;
  using TypeTag9 = list<i32_t>;
  using TypeTag10 = list<string_t>;
  using TypeTag11 = cpp_type<std::deque<std::string>, list<string_t>>;
  using TypeTag12 = list<struct_t<::test_cpp2::cpp_reflection::structA>>;
  using TypeTag13 = set<i32_t>;
  using TypeTag14 = set<string_t>;
  using TypeTag15 = set<string_t>;
  using TypeTag16 = set<struct_t<::test_cpp2::cpp_reflection::structB>>;
  using TypeTag17 =
      map<string_t, struct_t<::test_cpp2::cpp_reflection::structA>>;
  using TypeTag18 =
      map<string_t, struct_t<::test_cpp2::cpp_reflection::structB>>;
  using TypeTag19 = map<binary_t, binary_t>;

  using FieldTag0 = void;
  using FieldTag1 = type::field<TypeTag1, FieldContext<struct3, 2>>;
  using FieldTag2 = type::field<TypeTag2, FieldContext<struct3, 1>>;
  using FieldTag3 = type::field<TypeTag3, FieldContext<struct3, 3>>;
  using FieldTag4 = type::field<TypeTag4, FieldContext<struct3, 4>>;
  using FieldTag5 = type::field<TypeTag5, FieldContext<struct3, 5>>;
  using FieldTag6 = type::field<TypeTag6, FieldContext<struct3, 6>>;
  using FieldTag7 = type::field<TypeTag7, FieldContext<struct3, 7>>;
  using FieldTag8 = type::field<TypeTag8, FieldContext<struct3, 8>>;
  using FieldTag9 = type::field<TypeTag9, FieldContext<struct3, 9>>;
  using FieldTag10 = type::field<TypeTag10, FieldContext<struct3, 10>>;
  using FieldTag11 = type::field<TypeTag11, FieldContext<struct3, 11>>;
  using FieldTag12 = type::field<TypeTag12, FieldContext<struct3, 12>>;
  using FieldTag13 = type::field<TypeTag13, FieldContext<struct3, 13>>;
  using FieldTag14 = type::field<TypeTag14, FieldContext<struct3, 14>>;
  using FieldTag15 = type::field<TypeTag15, FieldContext<struct3, 15>>;
  using FieldTag16 = type::field<TypeTag16, FieldContext<struct3, 16>>;
  using FieldTag17 = type::field<TypeTag17, FieldContext<struct3, 17>>;
  using FieldTag18 = type::field<TypeTag18, FieldContext<struct3, 18>>;
  using FieldTag19 = type::field<TypeTag19, FieldContext<struct3, 20>>;

  // clang-format off
  checkField<struct3, field_ordinal<0>,  void,         void,        true,  TypeTag0,  FieldTag0>();
  checkField<struct3, field_ordinal<1>,  field_id<2>,  tag::fieldA, true,  TypeTag1,  FieldTag1>();
  checkField<struct3, field_ordinal<2>,  field_id<1>,  tag::fieldB, true,  TypeTag2,  FieldTag2>();
  checkField<struct3, field_ordinal<3>,  field_id<3>,  tag::fieldC, true,  TypeTag3,  FieldTag3>();
  checkField<struct3, field_ordinal<4>,  field_id<4>,  tag::fieldD, true,  TypeTag4,  FieldTag4>();
  checkField<struct3, field_ordinal<5>,  field_id<5>,  tag::fieldE, true,  TypeTag5,  FieldTag5>();
  checkField<struct3, field_ordinal<6>,  field_id<6>,  tag::fieldF, false, TypeTag6,  FieldTag6>();
  checkField<struct3, field_ordinal<7>,  field_id<7>,  tag::fieldG, true,  TypeTag7,  FieldTag7>();
  checkField<struct3, field_ordinal<8>,  field_id<8>,  tag::fieldH, false, TypeTag8,  FieldTag8>();
  checkField<struct3, field_ordinal<9>,  field_id<9>,  tag::fieldI, true,  TypeTag9,  FieldTag9>();
  checkField<struct3, field_ordinal<10>, field_id<10>, tag::fieldJ, true,  TypeTag10, FieldTag10>();
  checkField<struct3, field_ordinal<11>, field_id<11>, tag::fieldK, true,  TypeTag11, FieldTag11>();
  checkField<struct3, field_ordinal<12>, field_id<12>, tag::fieldL, true,  TypeTag12, FieldTag12>();
  checkField<struct3, field_ordinal<13>, field_id<13>, tag::fieldM, true,  TypeTag13, FieldTag13>();
  checkField<struct3, field_ordinal<14>, field_id<14>, tag::fieldN, false, TypeTag14, FieldTag14>();
  checkField<struct3, field_ordinal<15>, field_id<15>, tag::fieldO, false, TypeTag15, FieldTag15>();
  checkField<struct3, field_ordinal<16>, field_id<16>, tag::fieldP, true,  TypeTag16, FieldTag16>();
  checkField<struct3, field_ordinal<17>, field_id<17>, tag::fieldQ, true,  TypeTag17, FieldTag17>();
  checkField<struct3, field_ordinal<18>, field_id<18>, tag::fieldR, true,  TypeTag18, FieldTag18>();
  checkField<struct3, field_ordinal<19>, field_id<20>, tag::fieldS, true,  TypeTag19, FieldTag19>();
  // clang-format off
}

TEST(FieldsTest, NotFoundFieldInfo) {
  using Tag = struct_t<test_cpp2::cpp_reflection::struct3>;

  test::same_tag<type::get_field_ordinal<Tag, field_ordinal<0>>, field_ordinal<0>>;
  test::same_tag<type::get_field_id<Tag, field_ordinal<0>>, void>;
  test::same_tag<type::get_field_type_tag<Tag, field_ordinal<0>>, void>;
  test::same_tag<type::get_field_ident<Tag, field_ordinal<0>>, void>;
  test::same_tag<type::get_field_tag<Tag, field_ordinal<0>>, void>;

  test::same_tag<type::get_field_ordinal<Tag, field_id<200>>, field_ordinal<0>>;
  test::same_tag<type::get_field_id<Tag, field_id<200>>, void>;
  test::same_tag<type::get_field_type_tag<Tag, field_id<200>>, void>;
  test::same_tag<type::get_field_ident<Tag, field_id<200>>, void>;
  test::same_tag<type::get_field_tag<Tag, field_id<200>>, void>;

  test::same_tag<type::get_field_ordinal<Tag, binary_t>, field_ordinal<0>>;
  test::same_tag<type::get_field_id<Tag, binary_t>, void>;
  test::same_tag<type::get_field_type_tag<Tag, binary_t>, void>;
  test::same_tag<type::get_field_ident<Tag, binary_t>, void>;
  test::same_tag<type::get_field_tag<Tag, binary_t>, void>;

  test::same_tag<type::get_field_ordinal<Tag, tag::a>, field_ordinal<0>>;
  test::same_tag<type::get_field_id<Tag, tag::a>, void>;
  test::same_tag<type::get_field_type_tag<Tag, tag::a>, void>;
  test::same_tag<type::get_field_ident<Tag, tag::a>, void>;
  test::same_tag<type::get_field_tag<Tag, tag::a>, void>;
}

} // namespace apache::thrift::type
