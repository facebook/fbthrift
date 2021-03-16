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

#include <thrift/compiler/lib/cpp2/util.h>

#include <memory>

#include <folly/portability/GMock.h>
#include <folly/portability/GTest.h>

#include <thrift/compiler/ast/t_base_type.h>
#include <thrift/compiler/ast/t_exception.h>
#include <thrift/compiler/ast/t_field.h>
#include <thrift/compiler/ast/t_program.h>
#include <thrift/compiler/ast/t_struct.h>
#include <thrift/compiler/ast/t_type.h>
#include <thrift/compiler/ast/t_typedef.h>
#include <thrift/compiler/ast/t_union.h>

namespace apache::thrift::compiler {
namespace {

class UtilTest : public ::testing::Test {};

TEST_F(UtilTest, get_gen_namespace_components_cpp2) {
  t_program p("path/to/program.thrift");
  p.set_namespace("cpp2", "foo.bar");
  p.set_namespace("cpp", "baz.foo");
  EXPECT_THAT(
      cpp2::get_gen_namespace_components(p),
      testing::ElementsAreArray({"foo", "bar"}));
}

TEST_F(UtilTest, get_gen_namespace_components_cpp) {
  t_program p("path/to/program.thrift");
  p.set_namespace("cpp", "baz.foo");
  EXPECT_THAT(
      cpp2::get_gen_namespace_components(p),
      testing::ElementsAreArray({"baz", "foo", "cpp2"}));
}

TEST_F(UtilTest, get_gen_namespace_components_none) {
  t_program p("path/to/program.thrift");
  EXPECT_THAT(
      cpp2::get_gen_namespace_components(p),
      testing::ElementsAreArray({"cpp2"}));
}

TEST_F(UtilTest, get_gen_namespace__cpp2) {
  t_program p("path/to/program.thrift");
  p.set_namespace("cpp2", "foo.bar");
  p.set_namespace("cpp", "baz.foo");
  EXPECT_EQ("::foo::bar", cpp2::get_gen_namespace(p));
}

TEST_F(UtilTest, get_gen_namespace_cpp) {
  t_program p("path/to/program.thrift");
  p.set_namespace("cpp", "baz.foo");
  EXPECT_EQ("::baz::foo::cpp2", cpp2::get_gen_namespace(p));
}

TEST_F(UtilTest, get_gen_namespace_none) {
  t_program p("path/to/program.thrift");
  EXPECT_EQ("::cpp2", cpp2::get_gen_namespace(p));
}

TEST_F(UtilTest, is_orderable_set_template) {
  t_set t(&t_base_type::t_double());
  t.set_annotation("cpp2.template", "blah");
  t_program p("path/to/program.thrift");
  t_struct s(&p, "struct_name");
  s.append(std::make_unique<t_field>(&t, "set_field", 1));
  EXPECT_FALSE(cpp2::is_orderable(t));
  EXPECT_FALSE(cpp2::is_orderable(s));
}

TEST_F(UtilTest, is_orderable_struct) {
  t_program p("path/to/program.thrift");
  t_struct s(&p, "struct_name");
  s.append(
      std::make_unique<t_field>(&t_base_type::t_string(), "field_name", 1));
  EXPECT_TRUE(cpp2::is_orderable(s));
}

TEST_F(UtilTest, get_gen_type_class) {
  // a single example as demo
  EXPECT_EQ(
      "::apache::thrift::type_class::string",
      cpp2::get_gen_type_class(t_base_type::t_string()));
}

class TypeResolverTest : public ::testing::Test {
 public:
  TypeResolverTest() noexcept : program_("path/to/program.thrift") {
    program_.set_namespace("cpp2", "path.to");
  }

  const std::string& get_type_name(const t_type* node) {
    return resolver_.get_type_name(node);
  }

 protected:
  cpp2::TypeResolver resolver_;
  t_program program_;
  t_scope scope_;
};

TEST_F(TypeResolverTest, BaseTypes) {
  EXPECT_EQ(get_type_name(&t_base_type::t_void()), "void");
  EXPECT_EQ(get_type_name(&t_base_type::t_bool()), "bool");
  EXPECT_EQ(get_type_name(&t_base_type::t_byte()), "::std::int8_t");
  EXPECT_EQ(get_type_name(&t_base_type::t_i16()), "::std::int16_t");
  EXPECT_EQ(get_type_name(&t_base_type::t_i32()), "::std::int32_t");
  EXPECT_EQ(get_type_name(&t_base_type::t_i64()), "::std::int64_t");
  EXPECT_EQ(get_type_name(&t_base_type::t_float()), "float");
  EXPECT_EQ(get_type_name(&t_base_type::t_double()), "double");
  EXPECT_EQ(get_type_name(&t_base_type::t_string()), "::std::string");
  EXPECT_EQ(get_type_name(&t_base_type::t_binary()), "::std::string");
}

TEST_F(TypeResolverTest, Containers) {
  t_map tmap(&t_base_type::t_string(), &t_base_type::t_i32());
  EXPECT_EQ(get_type_name(&tmap), "::std::map<::std::string, ::std::int32_t>");
  t_list tlist(&t_base_type::t_double());
  EXPECT_EQ(get_type_name(&tlist), "::std::vector<double>");
  t_set tset(&tmap);
  EXPECT_EQ(
      get_type_name(&tset),
      "::std::set<::std::map<::std::string, ::std::int32_t>>");
}

TEST_F(TypeResolverTest, Structs) {
  t_struct tstruct(&program_, "Foo");
  EXPECT_EQ(get_type_name(&tstruct), "::path::to::Foo");
  t_union tunion(&program_, "Bar");
  EXPECT_EQ(get_type_name(&tunion), "::path::to::Bar");
  t_exception texcept(&program_, "Baz");
  EXPECT_EQ(get_type_name(&texcept), "::path::to::Baz");
}

TEST_F(TypeResolverTest, TypeDefs) {
  t_typedef ttypedef(&program_, &t_base_type::t_bool(), "Foo", &scope_);
  EXPECT_EQ(get_type_name(&ttypedef), "::path::to::Foo");
}

TEST_F(TypeResolverTest, CustomTemplate) {
  t_map tmap(&t_base_type::t_string(), &t_base_type::t_i32());
  tmap.set_annotation("cpp.template", "std::unordered_map");
  EXPECT_EQ(
      get_type_name(&tmap),
      "std::unordered_map<::std::string, ::std::int32_t>");
  t_list tlist(&t_base_type::t_double());
  tlist.set_annotation("cpp2.template", "std::list");
  EXPECT_EQ(get_type_name(&tlist), "std::list<double>");
  t_set tset(&t_base_type::t_binary());
  tset.set_annotation("cpp2.template", "::std::unordered_set");
  EXPECT_EQ(get_type_name(&tset), "::std::unordered_set<::std::string>");
}

TEST_F(TypeResolverTest, CustomType) {
  t_base_type tui64(t_base_type::t_i64());
  tui64.set_name("ui64");
  tui64.set_annotation("cpp2.type", "::std::uint64_t");
  EXPECT_EQ(get_type_name(&tui64), "::std::uint64_t");

  t_union tunion(&program_, "Bar");
  tunion.set_annotation("cpp2.type", "Other");
  EXPECT_EQ(get_type_name(&tunion), "Other");

  t_typedef ttypedef(&program_, &t_base_type::t_bool(), "Foo", &scope_);
  ttypedef.set_annotation("cpp2.type", "Other");
  EXPECT_EQ(get_type_name(&ttypedef), "Other");

  t_map tmap1(&t_base_type::t_string(), &tui64);
  EXPECT_EQ(
      get_type_name(&tmap1), "::std::map<::std::string, ::std::uint64_t>");

  // Can be combined with template.
  t_map tmap2(tmap1);
  tmap2.set_annotation("cpp.template", "std::unordered_map");
  EXPECT_EQ(
      get_type_name(&tmap2),
      "std::unordered_map<::std::string, ::std::uint64_t>");

  // Custom type overrides template.
  t_map tmap3(tmap2);
  tmap3.set_annotation("cpp.type", "MyMap");
  EXPECT_EQ(get_type_name(&tmap3), "MyMap");
}

} // namespace
} // namespace apache::thrift::compiler
