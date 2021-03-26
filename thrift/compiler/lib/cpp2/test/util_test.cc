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
#include <stdexcept>
#include <string>

#include <folly/portability/GMock.h>
#include <folly/portability/GTest.h>

#include <thrift/compiler/ast/t_base_type.h>
#include <thrift/compiler/ast/t_exception.h>
#include <thrift/compiler/ast/t_field.h>
#include <thrift/compiler/ast/t_map.h>
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

TEST_F(UtilTest, is_eligible_for_constexpr) {
  auto is_eligible_for_constexpr = [](const t_type* t) {
    return cpp2::is_eligible_for_constexpr()(t);
  };
  auto i32 = t_base_type::t_i32();
  EXPECT_TRUE(is_eligible_for_constexpr(&i32));
  EXPECT_TRUE(is_eligible_for_constexpr(&t_base_type::t_double()));
  EXPECT_TRUE(is_eligible_for_constexpr(&t_base_type::t_bool()));
  EXPECT_FALSE(is_eligible_for_constexpr(&t_base_type::t_string()));
  EXPECT_FALSE(is_eligible_for_constexpr(&t_base_type::t_binary()));

  auto list = t_list(&i32);
  EXPECT_FALSE(is_eligible_for_constexpr(&list));

  auto set = t_set(&i32);
  EXPECT_FALSE(is_eligible_for_constexpr(&set));

  auto map = t_map(&i32, &t_base_type::t_double());
  EXPECT_FALSE(is_eligible_for_constexpr(&map));

  for (auto a : {"cpp.indirection"}) {
    auto ref = t_base_type::t_i32();
    ref.set_annotation(a, "true");
    EXPECT_FALSE(is_eligible_for_constexpr(&ref));
  }

  for (auto a : {"cpp.template", "cpp2.template", "cpp.type", "cpp2.type"}) {
    auto type = i32;
    type.set_annotation(a, "custom_int");
    EXPECT_TRUE(is_eligible_for_constexpr(&type)) << a;
  }

  auto program = t_program("path/to/program.thrift");
  {
    auto s = t_struct(&program, "struct_name");
    s.append(std::make_unique<t_field>(&i32, "field1", 1));
    EXPECT_TRUE(is_eligible_for_constexpr(&s));
  }
  for (auto a : {"cpp.virtual", "cpp2.virtual", "cpp.allocator"}) {
    auto s = t_struct(&program, "struct_name");
    s.set_annotation(a, "true");
    EXPECT_FALSE(is_eligible_for_constexpr(&s)) << a;
  }
  {
    auto s = t_struct(&program, "struct_name");
    s.append(std::make_unique<t_field>(&i32, "field1", 1));
    s.append(std::make_unique<t_field>(&set, "field2", 2));
    EXPECT_FALSE(is_eligible_for_constexpr(&s));
  }
  for (auto a : {"cpp.ref", "cpp2.ref"}) {
    auto s = t_struct(&program, "struct_name");
    auto field = std::make_unique<t_field>(&i32, "field1", 1);
    field->set_annotation(a, "true");
    s.append(std::move(field));
    EXPECT_FALSE(is_eligible_for_constexpr(&s)) << a;
  }
  for (auto a : {"cpp.ref_type", "cpp2.ref_type"}) {
    auto s = t_struct(&program, "struct_name");
    auto field = std::make_unique<t_field>(&i32, "field1", 1);
    field->set_annotation(a, "unique");
    s.append(std::move(field));
    EXPECT_FALSE(is_eligible_for_constexpr(&s)) << a;
  }

  auto u = t_union(&program, "union_name");
  EXPECT_FALSE(is_eligible_for_constexpr(&u));
  auto e = t_exception(&program, "exception_name");
  EXPECT_FALSE(is_eligible_for_constexpr(&e));
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

  const std::string& get_native_type_name(const t_type* node) {
    return resolver_.get_native_type_name(node);
  }

  const std::string& get_storage_type_name(const t_field* node) {
    return resolver_.get_storage_type_name(node);
  }

 protected:
  cpp2::type_resolver resolver_{/*enable_adapters=*/true};
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

TEST_F(TypeResolverTest, BaseTypes_Adapter) {
  t_base_type dbl(t_base_type::t_double());
  dbl.set_annotation("cpp.adapter", "DblAdapter");
  // The native type is the default, double.
  EXPECT_EQ(get_native_type_name(&dbl), "double");
  // The c++ type is adapted.
  EXPECT_EQ(
      get_type_name(&dbl),
      "::apache::thrift::adapt_detail::adapted_t<DblAdapter, double>");

  // cpp.type overrides the 'default' native type.
  t_base_type ui64(t_base_type::t_i64());
  ui64.set_annotation("cpp.type", "uint64_t");
  ui64.set_annotation("cpp.adapter", "HashAdapter");
  EXPECT_EQ(get_native_type_name(&ui64), "uint64_t");
  EXPECT_EQ(
      get_type_name(&ui64),
      "::apache::thrift::adapt_detail::adapted_t<HashAdapter, uint64_t>");
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

TEST_F(TypeResolverTest, Containers_CustomTemplate) {
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

TEST_F(TypeResolverTest, Containers_Adapter) {
  t_base_type ui64(t_base_type::t_i64());
  ui64.set_annotation("cpp.type", "uint64_t");
  ui64.set_annotation("cpp.adapter", "HashAdapter");

  // Adapters work on container type arguments.
  t_map tmap(&t_base_type::t_i16(), &ui64);
  tmap.set_annotation("cpp.adapter", "MapAdapter");
  EXPECT_EQ(
      get_native_type_name(&tmap), "::std::map<::std::int16_t, uint64_t>");
  EXPECT_EQ(
      get_type_name(&tmap),
      "::apache::thrift::adapt_detail::adapted_t<"
      "MapAdapter, "
      "::std::map<::std::int16_t, ::apache::thrift::adapt_detail::adapted_t<HashAdapter, uint64_t>>>");

  // The container can also be addapted.
  t_set tset(&ui64);
  tset.set_annotation("cpp.adapter", "SetAdapter");
  tset.set_annotation("cpp.template", "std::unordered_set");
  // The template argument is respected for both native and adapted types.
  EXPECT_EQ(get_native_type_name(&tset), "std::unordered_set<uint64_t>");
  EXPECT_EQ(
      get_type_name(&tset),
      "::apache::thrift::adapt_detail::adapted_t<"
      "SetAdapter, "
      "std::unordered_set<::apache::thrift::adapt_detail::adapted_t<HashAdapter, uint64_t>>>");

  // cpp.type on the container overrides the 'default' native type.
  t_list tlist(&ui64);
  tlist.set_annotation("cpp.adapter", "ListAdapter");
  tlist.set_annotation("cpp.type", "MyList");
  EXPECT_EQ(get_native_type_name(&tlist), "MyList");
  EXPECT_EQ(
      get_type_name(&tlist),
      "::apache::thrift::adapt_detail::adapted_t<ListAdapter, MyList>");
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

TEST_F(TypeResolverTest, TypeDefs_Adapter) {
  t_base_type ui64(t_base_type::t_i64());
  ui64.set_annotation("cpp.type", "uint64_t");
  ui64.set_annotation("cpp.adapter", "HashAdapter");

  // Type defs can refer to adatped types.
  t_typedef ttypedef1(&program_, &ui64, "MyHash", &scope_);
  // It does not affect the type name.
  EXPECT_EQ(get_native_type_name(&ttypedef1), "::path::to::MyHash");
  EXPECT_EQ(get_type_name(&ttypedef1), "::path::to::MyHash");
  // It is the refered to type that has the adapter.
  EXPECT_EQ(
      get_type_name(ttypedef1.get_type()),
      "::apache::thrift::adapt_detail::adapted_t<HashAdapter, uint64_t>");

  // Type defs can also be adapted.
  t_typedef ttypedef2(ttypedef1);
  ttypedef2.set_annotation("cpp.adapter", "TypeDefAdapter");
  EXPECT_EQ(get_native_type_name(&ttypedef2), "::path::to::MyHash");
  EXPECT_EQ(
      get_type_name(&ttypedef2),
      "::apache::thrift::adapt_detail::adapted_t<TypeDefAdapter, ::path::to::MyHash>");
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

TEST_F(TypeResolverTest, StreamingRes) {
  t_base_type ui64(t_base_type::t_i64());
  ui64.set_annotation("cpp.type", "uint64_t");
  ui64.set_annotation("cpp.adapter", "HashAdapter");

  t_stream_response res1(&ui64);
  EXPECT_EQ(
      get_native_type_name(&res1), "::apache::thrift::ServerStream<uint64_t>");
  EXPECT_EQ(
      get_type_name(&res1),
      "::apache::thrift::ServerStream<::apache::thrift::adapt_detail::adapted_t<HashAdapter, uint64_t>>");

  t_stream_response res2(&ui64);
  res2.set_first_response_type(&ui64);
  EXPECT_EQ(
      get_native_type_name(&res2),
      "::apache::thrift::ResponseAndServerStream<uint64_t, uint64_t>");
  EXPECT_EQ(
      get_type_name(&res2),
      "::apache::thrift::ResponseAndServerStream<"
      "::apache::thrift::adapt_detail::adapted_t<HashAdapter, uint64_t>, "
      "::apache::thrift::adapt_detail::adapted_t<HashAdapter, uint64_t>>");
}

TEST_F(TypeResolverTest, StreamingSink) {
  t_base_type ui64(t_base_type::t_i64());
  ui64.set_annotation("cpp.type", "uint64_t");
  ui64.set_annotation("cpp.adapter", "HashAdapter");

  t_sink req1(&ui64, nullptr, &ui64, nullptr);
  EXPECT_EQ(
      get_native_type_name(&req1),
      "::apache::thrift::SinkConsumer<uint64_t, uint64_t>");
  EXPECT_EQ(
      get_type_name(&req1),
      "::apache::thrift::SinkConsumer<"
      "::apache::thrift::adapt_detail::adapted_t<HashAdapter, uint64_t>, "
      "::apache::thrift::adapt_detail::adapted_t<HashAdapter, uint64_t>>");

  t_sink req2(&ui64, nullptr, &ui64, nullptr);
  req2.set_first_response(&ui64);
  EXPECT_EQ(
      get_native_type_name(&req2),
      "::apache::thrift::ResponseAndSinkConsumer<uint64_t, uint64_t, uint64_t>");
  EXPECT_EQ(
      get_type_name(&req2),
      "::apache::thrift::ResponseAndSinkConsumer<"
      "::apache::thrift::adapt_detail::adapted_t<HashAdapter, uint64_t>, "
      "::apache::thrift::adapt_detail::adapted_t<HashAdapter, uint64_t>, "
      "::apache::thrift::adapt_detail::adapted_t<HashAdapter, uint64_t>>");
}

TEST_F(TypeResolverTest, StorageType) {
  t_base_type ui64(t_base_type::t_i64());
  ui64.set_annotation("cpp.type", "uint64_t");
  ui64.set_annotation("cpp.adapter", "HashAdapter");

  {
    t_field ui64_field(&ui64, "hash", 1);
    EXPECT_EQ(
        get_storage_type_name(&ui64_field),
        "::apache::thrift::adapt_detail::adapted_t<HashAdapter, uint64_t>");
  }

  {
    t_field ui64_field(&ui64, "hash", 1);
    ui64_field.set_annotation("cpp.ref", "");
    EXPECT_EQ(
        get_storage_type_name(&ui64_field),
        "::std::unique_ptr<::apache::thrift::adapt_detail::adapted_t<HashAdapter, uint64_t>>");
  }
  {
    t_field ui64_field(&ui64, "hash", 1);
    ui64_field.set_annotation("cpp2.ref", ""); // Works with cpp2.
    EXPECT_EQ(
        get_storage_type_name(&ui64_field),
        "::std::unique_ptr<::apache::thrift::adapt_detail::adapted_t<HashAdapter, uint64_t>>");
  }

  {
    t_field ui64_field(&ui64, "hash", 1);
    ui64_field.set_annotation("cpp.ref_type", "unique");
    EXPECT_EQ(
        get_storage_type_name(&ui64_field),
        "::std::unique_ptr<::apache::thrift::adapt_detail::adapted_t<HashAdapter, uint64_t>>");
  }

  {
    t_field ui64_field(&ui64, "hash", 1);
    ui64_field.set_annotation("cpp2.ref_type", "shared"); // Works with cpp2.
    EXPECT_EQ(
        get_storage_type_name(&ui64_field),
        "::std::shared_ptr<::apache::thrift::adapt_detail::adapted_t<HashAdapter, uint64_t>>");
  }

  {
    t_field ui64_field(&ui64, "hash", 1);
    ui64_field.set_annotation("cpp.ref_type", "shared_mutable");
    EXPECT_EQ(
        get_storage_type_name(&ui64_field),
        "::std::shared_ptr<::apache::thrift::adapt_detail::adapted_t<HashAdapter, uint64_t>>");
  }

  {
    t_field ui64_field(&ui64, "hash", 1);
    ui64_field.set_annotation("cpp.ref_type", "shared_const");
    EXPECT_EQ(
        get_storage_type_name(&ui64_field),
        "::std::shared_ptr<const ::apache::thrift::adapt_detail::adapted_t<HashAdapter, uint64_t>>");
  }

  { // Unrecognized throws an exception.
    t_field ui64_field(&ui64, "hash", 1);
    ui64_field.set_annotation("cpp.ref_type", "blah");
    EXPECT_THROW(get_storage_type_name(&ui64_field), std::runtime_error);
  }
}

TEST_F(TypeResolverTest, Typedef_cpptemplate) {
  t_map imap(&t_base_type::t_i32(), &t_base_type::t_string());
  t_typedef iumap(&program_, &imap, "iumap", &scope_);
  iumap.set_annotation("cpp.template", "std::unorderd_map");
  t_typedef tiumap(&program_, &iumap, "tiumap", &scope_);

  EXPECT_EQ(get_type_name(&imap), "::std::map<::std::int32_t, ::std::string>");
  EXPECT_EQ(
      get_native_type_name(&imap), "::std::map<::std::int32_t, ::std::string>");

  // The 'cpp.template' annotations is applied to the typedef; however, the type
  // resolver only looks for it on container types.
  // TODO(afuller): Consider making the template annotation propagate through
  // the typedef.
  EXPECT_EQ(get_type_name(&iumap), "::path::to::iumap");
  EXPECT_EQ(get_native_type_name(&iumap), "::path::to::iumap");

  EXPECT_EQ(get_type_name(&tiumap), "::path::to::tiumap");
  EXPECT_EQ(get_native_type_name(&tiumap), "::path::to::tiumap");
}

TEST_F(TypeResolverTest, Typedef_cpptype) {
  t_map imap(&t_base_type::t_i32(), &t_base_type::t_string());
  t_typedef iumap(&program_, &imap, "iumap", &scope_);
  iumap.set_annotation(
      "cpp.type", "std::unorderd_map<::std::int32_t, ::std::string>");
  t_typedef tiumap(&program_, &iumap, "tiumap", &scope_);

  EXPECT_EQ(get_type_name(&imap), "::std::map<::std::int32_t, ::std::string>");
  EXPECT_EQ(
      get_native_type_name(&imap), "::std::map<::std::int32_t, ::std::string>");

  // The 'cpp.type' annotation is respected on the typedef.
  // TODO(afuller): It seems like this annotation is applied incorrectly and the
  // annotation should apply to the type being referenced, not the type name of
  // the typedef itself (which should just be the typedef's specified name)
  // which is still code-genend, but with the wrong type!
  EXPECT_EQ(
      get_type_name(&iumap),
      "std::unorderd_map<::std::int32_t, ::std::string>");
  EXPECT_EQ(
      get_native_type_name(&iumap),
      "std::unorderd_map<::std::int32_t, ::std::string>");

  EXPECT_EQ(get_type_name(&tiumap), "::path::to::tiumap");
  EXPECT_EQ(get_native_type_name(&tiumap), "::path::to::tiumap");
}

} // namespace
} // namespace apache::thrift::compiler
