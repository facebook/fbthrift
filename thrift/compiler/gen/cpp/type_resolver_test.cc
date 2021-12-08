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

#include <thrift/compiler/gen/cpp/type_resolver.h>

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include <folly/portability/GTest.h>

#include <thrift/compiler/ast/t_typedef.h>
#include <thrift/compiler/ast/t_union.h>
#include <thrift/compiler/gen/cpp/namespace_resolver.h>

namespace apache::thrift::compiler::gen::cpp {
namespace {

class TypeResolverTest : public ::testing::Test {
 public:
  TypeResolverTest() noexcept : program_("path/to/program.thrift") {
    program_.set_namespace("cpp2", "path.to");
  }

  bool can_resolve_to_scalar(const t_type& node) {
    return resolver_.can_resolve_to_scalar(&node);
  }

  const std::string& get_type_name(const t_type& node) {
    return resolver_.get_type_name(&node);
  }

  const std::string& get_standard_type_name(const t_type& node) {
    return resolver_.get_standard_type_name(&node);
  }

  const std::string& get_storage_type_name(const t_field& node) {
    return resolver_.get_storage_type_name(&node);
  }

 protected:
  type_resolver resolver_;
  t_program program_;
};

TEST_F(TypeResolverTest, BaseTypes) {
  EXPECT_EQ(get_type_name(t_base_type::t_void()), "void");
  EXPECT_EQ(get_type_name(t_base_type::t_bool()), "bool");
  EXPECT_EQ(get_type_name(t_base_type::t_byte()), "::std::int8_t");
  EXPECT_EQ(get_type_name(t_base_type::t_i16()), "::std::int16_t");
  EXPECT_EQ(get_type_name(t_base_type::t_i32()), "::std::int32_t");
  EXPECT_EQ(get_type_name(t_base_type::t_i64()), "::std::int64_t");
  EXPECT_EQ(get_type_name(t_base_type::t_float()), "float");
  EXPECT_EQ(get_type_name(t_base_type::t_double()), "double");
  EXPECT_EQ(get_type_name(t_base_type::t_string()), "::std::string");
  EXPECT_EQ(get_type_name(t_base_type::t_binary()), "::std::string");

  EXPECT_FALSE(can_resolve_to_scalar(t_base_type::t_void()));
  EXPECT_TRUE(can_resolve_to_scalar(t_base_type::t_bool()));
  EXPECT_TRUE(can_resolve_to_scalar(t_base_type::t_byte()));
  EXPECT_TRUE(can_resolve_to_scalar(t_base_type::t_i16()));
  EXPECT_TRUE(can_resolve_to_scalar(t_base_type::t_i32()));
  EXPECT_TRUE(can_resolve_to_scalar(t_base_type::t_i64()));
  EXPECT_TRUE(can_resolve_to_scalar(t_base_type::t_float()));
  EXPECT_TRUE(can_resolve_to_scalar(t_base_type::t_double()));
  EXPECT_FALSE(can_resolve_to_scalar(t_base_type::t_string()));
  EXPECT_FALSE(can_resolve_to_scalar(t_base_type::t_binary()));
}

TEST_F(TypeResolverTest, BaseTypes_Adapter) {
  t_base_type dbl(t_base_type::t_double());
  dbl.set_annotation("cpp.adapter", "DblAdapter");
  // The standard type is the default, double.
  EXPECT_EQ(get_standard_type_name(dbl), "double");
  // The c++ type is adapted.
  EXPECT_EQ(
      get_type_name(dbl),
      "::apache::thrift::adapt_detail::adapted_t<DblAdapter, double>");
  EXPECT_TRUE(can_resolve_to_scalar(dbl));

  // cpp.type overrides the 'default' standard type.
  t_base_type ui64(t_base_type::t_i64());
  ui64.set_annotation("cpp.type", "uint64_t");
  ui64.set_annotation("cpp.adapter", "HashAdapter");
  EXPECT_EQ(get_standard_type_name(ui64), "uint64_t");
  EXPECT_EQ(
      get_type_name(ui64),
      "::apache::thrift::adapt_detail::adapted_t<HashAdapter, uint64_t>");
  EXPECT_TRUE(can_resolve_to_scalar(ui64));

  // cpp.adapter could resolve to a scalar.
  t_base_type str(t_base_type::t_string());
  str.set_annotation("cpp.adapter", "HashAdapter");
  EXPECT_TRUE(can_resolve_to_scalar(str));
}

TEST_F(TypeResolverTest, CppName) {
  t_enum tenum(&program_, "MyEnum");
  tenum.set_annotation("cpp.name", "YourEnum");
  EXPECT_EQ(get_type_name(tenum), "::path::to::YourEnum");
}

TEST_F(TypeResolverTest, Containers) {
  // A container could not resolve to a scalar.
  t_map tmap(t_base_type::t_string(), t_base_type::t_i32());
  EXPECT_EQ(get_type_name(tmap), "::std::map<::std::string, ::std::int32_t>");
  EXPECT_FALSE(can_resolve_to_scalar(tmap));
  t_list tlist(t_base_type::t_double());
  EXPECT_EQ(get_type_name(tlist), "::std::vector<double>");
  EXPECT_FALSE(can_resolve_to_scalar(tlist));
  t_set tset(tmap);
  EXPECT_EQ(
      get_type_name(tset),
      "::std::set<::std::map<::std::string, ::std::int32_t>>");
  EXPECT_FALSE(can_resolve_to_scalar(tset));
}

TEST_F(TypeResolverTest, Containers_CustomTemplate) {
  // cpp.template could not resolve to a scalar since it can be only used for
  // container fields.
  t_map tmap(t_base_type::t_string(), t_base_type::t_i32());
  tmap.set_annotation("cpp.template", "std::unordered_map");
  EXPECT_EQ(
      get_type_name(tmap), "std::unordered_map<::std::string, ::std::int32_t>");
  EXPECT_FALSE(can_resolve_to_scalar(tmap));
  t_list tlist(t_base_type::t_double());
  tlist.set_annotation("cpp2.template", "std::list");
  EXPECT_EQ(get_type_name(tlist), "std::list<double>");
  EXPECT_FALSE(can_resolve_to_scalar(tlist));
  t_set tset(t_base_type::t_binary());
  tset.set_annotation("cpp2.template", "::std::unordered_set");
  EXPECT_EQ(get_type_name(tset), "::std::unordered_set<::std::string>");
  EXPECT_FALSE(can_resolve_to_scalar(tset));
}

TEST_F(TypeResolverTest, Containers_Adapter) {
  // cpp.adapter could resolve to a scalar.
  t_base_type ui64(t_base_type::t_i64());
  ui64.set_annotation("cpp.type", "uint64_t");
  ui64.set_annotation("cpp.adapter", "HashAdapter");
  EXPECT_TRUE(can_resolve_to_scalar(ui64));

  // Adapters work on container type arguments.
  t_map tmap(t_base_type::t_i16(), ui64);
  tmap.set_annotation("cpp.adapter", "MapAdapter");
  EXPECT_EQ(
      get_standard_type_name(tmap), "::std::map<::std::int16_t, uint64_t>");
  EXPECT_EQ(
      get_type_name(tmap),
      "::apache::thrift::adapt_detail::adapted_t<"
      "MapAdapter, "
      "::std::map<::std::int16_t, ::apache::thrift::adapt_detail::adapted_t<HashAdapter, uint64_t>>>");
  EXPECT_TRUE(can_resolve_to_scalar(tmap));

  // The container can also be addapted.
  t_set tset(ui64);
  tset.set_annotation("cpp.adapter", "SetAdapter");
  tset.set_annotation("cpp.template", "std::unordered_set");
  // The template argument is respected for both standard and adapted types.
  EXPECT_EQ(get_standard_type_name(tset), "std::unordered_set<uint64_t>");
  EXPECT_EQ(
      get_type_name(tset),
      "::apache::thrift::adapt_detail::adapted_t<"
      "SetAdapter, "
      "std::unordered_set<::apache::thrift::adapt_detail::adapted_t<HashAdapter, uint64_t>>>");
  EXPECT_TRUE(can_resolve_to_scalar(tset));

  // cpp.type on the container overrides the 'default' standard type.
  t_list tlist(ui64);
  tlist.set_annotation("cpp.adapter", "ListAdapter");
  tlist.set_annotation("cpp.type", "MyList");
  EXPECT_EQ(get_standard_type_name(tlist), "MyList");
  EXPECT_EQ(
      get_type_name(tlist),
      "::apache::thrift::adapt_detail::adapted_t<ListAdapter, MyList>");
  EXPECT_TRUE(can_resolve_to_scalar(tlist));
}

TEST_F(TypeResolverTest, Structs) {
  t_struct tstruct(&program_, "Foo");
  EXPECT_EQ(get_type_name(tstruct), "::path::to::Foo");
  EXPECT_FALSE(can_resolve_to_scalar(tstruct));
  t_union tunion(&program_, "Bar");
  EXPECT_EQ(get_type_name(tunion), "::path::to::Bar");
  EXPECT_FALSE(can_resolve_to_scalar(tunion));
  t_exception texcept(&program_, "Baz");
  EXPECT_EQ(get_type_name(texcept), "::path::to::Baz");
  EXPECT_FALSE(can_resolve_to_scalar(texcept));
}

TEST_F(TypeResolverTest, TypeDefs) {
  // Scalar
  t_typedef ttypedef1(&program_, "Foo", t_base_type::t_bool());
  EXPECT_EQ(get_type_name(ttypedef1), "::path::to::Foo");
  EXPECT_TRUE(can_resolve_to_scalar(ttypedef1));

  // Non-scalar
  t_typedef ttypedef2(&program_, "Foo", t_base_type::t_string());
  EXPECT_EQ(get_type_name(ttypedef2), "::path::to::Foo");
  EXPECT_FALSE(can_resolve_to_scalar(ttypedef2));
}

TEST_F(TypeResolverTest, TypeDefs_Nested) {
  // Scalar
  t_typedef ttypedef1(&program_, "Foo", t_base_type::t_bool());
  t_typedef ttypedef2(&program_, "Bar", ttypedef1);
  EXPECT_EQ(get_type_name(ttypedef2), "::path::to::Bar");
  EXPECT_TRUE(can_resolve_to_scalar(ttypedef1));
  EXPECT_TRUE(can_resolve_to_scalar(ttypedef2));

  // Non-scalar
  t_typedef ttypedef3(&program_, "Foo", t_base_type::t_string());
  t_typedef ttypedef4(&program_, "Bar", ttypedef3);
  EXPECT_EQ(get_type_name(ttypedef4), "::path::to::Bar");
  EXPECT_FALSE(can_resolve_to_scalar(ttypedef3));
  EXPECT_FALSE(can_resolve_to_scalar(ttypedef4));
}

TEST_F(TypeResolverTest, TypeDefs_Adapter) {
  t_base_type ui64(t_base_type::t_i64());
  ui64.set_annotation("cpp.type", "uint64_t");
  ui64.set_annotation("cpp.adapter", "HashAdapter");

  // Type defs can refer to adatped types.
  t_typedef ttypedef1(&program_, "MyHash", ui64);
  // It does not affect the type name.
  EXPECT_EQ(get_standard_type_name(ttypedef1), "uint64_t");
  EXPECT_EQ(get_type_name(ttypedef1), "::path::to::MyHash");
  // It is the refered to type that has the adapter.
  EXPECT_EQ(
      get_type_name(*ttypedef1.type()),
      "::apache::thrift::adapt_detail::adapted_t<HashAdapter, uint64_t>");
  EXPECT_TRUE(can_resolve_to_scalar(ttypedef1));

  // Type defs can also be adapted.
  t_typedef ttypedef2(ttypedef1);
  ttypedef2.set_annotation("cpp.adapter", "TypeDefAdapter");
  EXPECT_EQ(get_standard_type_name(ttypedef2), "uint64_t");
  EXPECT_EQ(
      get_type_name(ttypedef2),
      "::apache::thrift::adapt_detail::adapted_t<TypeDefAdapter, ::path::to::MyHash>");
  EXPECT_TRUE(can_resolve_to_scalar(ttypedef2));
}

TEST_F(TypeResolverTest, CustomType) {
  t_base_type tui64(t_base_type::t_i64());
  tui64.set_name("ui64");
  tui64.set_annotation("cpp2.type", "::std::uint64_t");
  EXPECT_EQ(get_type_name(tui64), "::std::uint64_t");
  EXPECT_TRUE(can_resolve_to_scalar(tui64));

  t_union tunion(&program_, "Bar");
  tunion.set_annotation("cpp2.type", "Other");
  EXPECT_EQ(get_type_name(tunion), "Other");
  EXPECT_TRUE(can_resolve_to_scalar(tunion));

  t_typedef ttypedef1(&program_, "Foo", t_base_type::t_bool());
  ttypedef1.set_annotation("cpp2.type", "Other");
  EXPECT_EQ(get_type_name(ttypedef1), "Other");
  EXPECT_TRUE(can_resolve_to_scalar(ttypedef1));

  t_typedef ttypedef2(&program_, "FooBar", t_base_type::t_string());
  ttypedef2.set_annotation("cpp2.type", "Other");
  EXPECT_EQ(get_type_name(ttypedef2), "Other");
  EXPECT_TRUE(can_resolve_to_scalar(ttypedef2));

  t_map tmap1(t_base_type::t_string(), tui64);
  EXPECT_EQ(get_type_name(tmap1), "::std::map<::std::string, ::std::uint64_t>");
  EXPECT_FALSE(can_resolve_to_scalar(tmap1));

  // Can be combined with template.
  t_map tmap2(tmap1);
  tmap2.set_annotation("cpp.template", "std::unordered_map");
  EXPECT_EQ(
      get_type_name(tmap2),
      "std::unordered_map<::std::string, ::std::uint64_t>");
  EXPECT_FALSE(can_resolve_to_scalar(tmap2));

  // Custom type overrides template.
  t_map tmap3(tmap2);
  tmap3.set_annotation("cpp.type", "MyMap");
  EXPECT_EQ(get_type_name(tmap3), "MyMap");
  EXPECT_TRUE(can_resolve_to_scalar(tmap3));
}

TEST_F(TypeResolverTest, StreamingRes) {
  t_base_type ui64(t_base_type::t_i64());
  ui64.set_annotation("cpp.type", "uint64_t");
  ui64.set_annotation("cpp.adapter", "HashAdapter");

  t_stream_response res1(ui64);
  EXPECT_EQ(
      get_standard_type_name(res1), "::apache::thrift::ServerStream<uint64_t>");
  EXPECT_EQ(
      get_type_name(res1),
      "::apache::thrift::ServerStream<::apache::thrift::adapt_detail::adapted_t<HashAdapter, uint64_t>>");

  t_stream_response res2(ui64);
  res2.set_first_response_type(t_type_ref(ui64));
  EXPECT_EQ(
      get_standard_type_name(res2),
      "::apache::thrift::ResponseAndServerStream<uint64_t, uint64_t>");
  EXPECT_EQ(
      get_type_name(res2),
      "::apache::thrift::ResponseAndServerStream<"
      "::apache::thrift::adapt_detail::adapted_t<HashAdapter, uint64_t>, "
      "::apache::thrift::adapt_detail::adapted_t<HashAdapter, uint64_t>>");
}

TEST_F(TypeResolverTest, StreamingSink) {
  t_base_type ui64(t_base_type::t_i64());
  ui64.set_annotation("cpp.type", "uint64_t");
  ui64.set_annotation("cpp.adapter", "HashAdapter");

  t_sink req1(ui64, ui64);
  EXPECT_EQ(
      get_standard_type_name(req1),
      "::apache::thrift::SinkConsumer<uint64_t, uint64_t>");
  EXPECT_EQ(
      get_type_name(req1),
      "::apache::thrift::SinkConsumer<"
      "::apache::thrift::adapt_detail::adapted_t<HashAdapter, uint64_t>, "
      "::apache::thrift::adapt_detail::adapted_t<HashAdapter, uint64_t>>");

  t_sink req2(ui64, ui64);
  req2.set_first_response_type(t_type_ref(ui64));
  EXPECT_EQ(
      get_standard_type_name(req2),
      "::apache::thrift::ResponseAndSinkConsumer<uint64_t, uint64_t, uint64_t>");
  EXPECT_EQ(
      get_type_name(req2),
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
    t_field ui64_field(ui64, "hash", 1);
    EXPECT_EQ(
        get_storage_type_name(ui64_field),
        "::apache::thrift::adapt_detail::adapted_t<HashAdapter, uint64_t>");
  }

  {
    t_field ui64_field(ui64, "hash", 1);
    ui64_field.set_annotation("cpp.ref", "");
    EXPECT_EQ(
        get_storage_type_name(ui64_field),
        "::std::unique_ptr<::apache::thrift::adapt_detail::adapted_t<HashAdapter, uint64_t>>");
  }
  {
    t_field ui64_field(ui64, "hash", 1);
    ui64_field.set_annotation("cpp2.ref", ""); // Works with cpp2.
    EXPECT_EQ(
        get_storage_type_name(ui64_field),
        "::std::unique_ptr<::apache::thrift::adapt_detail::adapted_t<HashAdapter, uint64_t>>");
  }

  {
    t_field ui64_field(ui64, "hash", 1);
    ui64_field.set_annotation("cpp.ref_type", "unique");
    EXPECT_EQ(
        get_storage_type_name(ui64_field),
        "::std::unique_ptr<::apache::thrift::adapt_detail::adapted_t<HashAdapter, uint64_t>>");
  }

  {
    t_field ui64_field(ui64, "hash", 1);
    ui64_field.set_annotation("cpp2.ref_type", "shared"); // Works with cpp2.
    EXPECT_EQ(
        get_storage_type_name(ui64_field),
        "::std::shared_ptr<::apache::thrift::adapt_detail::adapted_t<HashAdapter, uint64_t>>");
  }

  {
    t_field ui64_field(ui64, "hash", 1);
    ui64_field.set_annotation("cpp.ref_type", "shared_mutable");
    EXPECT_EQ(
        get_storage_type_name(ui64_field),
        "::std::shared_ptr<::apache::thrift::adapt_detail::adapted_t<HashAdapter, uint64_t>>");
  }

  {
    t_field ui64_field(ui64, "hash", 1);
    ui64_field.set_annotation("cpp.ref_type", "shared_const");
    EXPECT_EQ(
        get_storage_type_name(ui64_field),
        "::std::shared_ptr<const ::apache::thrift::adapt_detail::adapted_t<HashAdapter, uint64_t>>");
  }

  { // Unrecognized throws an exception.
    t_field ui64_field(ui64, "hash", 1);
    ui64_field.set_annotation("cpp.ref_type", "blah");
    EXPECT_THROW(get_storage_type_name(ui64_field), std::runtime_error);
  }
}

TEST_F(TypeResolverTest, Typedef_cpptemplate) {
  // cpp.template could not resolve to a scalar since it can be only used for
  // container fields.
  t_map imap(t_base_type::t_i32(), t_base_type::t_string());
  t_typedef iumap(&program_, "iumap", imap);
  iumap.set_annotation("cpp.template", "std::unorderd_map");
  t_typedef tiumap(&program_, "tiumap", iumap);

  EXPECT_EQ(get_type_name(imap), "::std::map<::std::int32_t, ::std::string>");
  EXPECT_EQ(
      get_standard_type_name(imap),
      "::std::map<::std::int32_t, ::std::string>");
  EXPECT_FALSE(can_resolve_to_scalar(imap));

  // The 'cpp.template' annotations is applied to the typedef; however, the type
  // resolver only looks for it on container types.
  // TODO(afuller): Consider making the template annotation propagate through
  // the typedef.
  EXPECT_EQ(get_type_name(iumap), "::path::to::iumap");
  EXPECT_EQ(get_standard_type_name(iumap), "::path::to::iumap");
  EXPECT_FALSE(can_resolve_to_scalar(iumap));

  EXPECT_EQ(get_type_name(tiumap), "::path::to::tiumap");
  EXPECT_EQ(get_standard_type_name(tiumap), "::path::to::tiumap");
  EXPECT_FALSE(can_resolve_to_scalar(tiumap));
}

TEST_F(TypeResolverTest, Typedef_cpptype) {
  t_map imap(t_base_type::t_i32(), t_base_type::t_string());
  t_typedef iumap(&program_, "iumap", imap);
  iumap.set_annotation(
      "cpp.type", "std::unorderd_map<::std::int32_t, ::std::string>");
  t_typedef tiumap(&program_, "tiumap", iumap);

  EXPECT_EQ(get_type_name(imap), "::std::map<::std::int32_t, ::std::string>");
  EXPECT_EQ(
      get_standard_type_name(imap),
      "::std::map<::std::int32_t, ::std::string>");
  EXPECT_FALSE(can_resolve_to_scalar(imap));

  // The 'cpp.type' annotation is respected on the typedef.
  // TODO(afuller): It seems like this annotation is applied incorrectly and the
  // annotation should apply to the type being referenced, not the type name of
  // the typedef itself (which should just be the typedef's specified name)
  // which is still code-genend, but with the wrong type!
  EXPECT_EQ(
      get_type_name(iumap), "std::unorderd_map<::std::int32_t, ::std::string>");
  EXPECT_EQ(
      get_standard_type_name(iumap),
      "std::unorderd_map<::std::int32_t, ::std::string>");
  EXPECT_TRUE(can_resolve_to_scalar(iumap));

  EXPECT_EQ(get_type_name(tiumap), "::path::to::tiumap");
  EXPECT_EQ(get_standard_type_name(tiumap), "::path::to::tiumap");
  EXPECT_TRUE(can_resolve_to_scalar(tiumap));
}

TEST_F(TypeResolverTest, Typedef_cpptype_and_adapter) {
  // cpp.type sets the 'standard' type.
  // cpp.adapter adapts the 'standard' type.

  // Switch the standard type to IOBuf:
  //   iobuf_type = binary (cpp.type="folly::IOBuf")
  t_base_type iobuf_type = t_base_type::t_binary();
  iobuf_type.set_annotation("cpp.type", "folly::IOBuf");
  EXPECT_EQ(get_type_name(iobuf_type), "folly::IOBuf");
  EXPECT_EQ(get_standard_type_name(iobuf_type), "folly::IOBuf");
  EXPECT_TRUE(can_resolve_to_scalar(iobuf_type));

  // Then adapt it:
  //   binary (cpp.type="folly::IOBuf", cpp.adapter="MyAdapter1")
  t_base_type adapated_type = iobuf_type;
  adapated_type.set_annotation("cpp.adapter", "MyAdapter1");
  EXPECT_EQ(
      get_type_name(adapated_type),
      "::apache::thrift::adapt_detail::adapted_t<MyAdapter1, folly::IOBuf>");
  EXPECT_EQ(get_standard_type_name(adapated_type), "folly::IOBuf");
  EXPECT_TRUE(can_resolve_to_scalar(adapated_type));

  // Give the adapated type an alias:
  //   typedef binary (cpp.type="folly::IOBuf", cpp.adapter="MyAdapter2")
  //       AdaptedTypeTypeDef
  t_typedef adapted_type_typedef(
      &program_, "AdaptedTypeTypeDef", adapated_type);
  EXPECT_EQ(
      get_type_name(adapted_type_typedef), "::path::to::AdaptedTypeTypeDef");
  EXPECT_EQ(get_standard_type_name(adapted_type_typedef), "folly::IOBuf");
  EXPECT_TRUE(can_resolve_to_scalar(adapted_type_typedef));

  // Use the alias for the adapted type for field 1.
  // 1: AdaptedTypeTypeDef field1
  t_field field1(adapted_type_typedef, "field1", 1);
  EXPECT_EQ(get_storage_type_name(field1), "::path::to::AdaptedTypeTypeDef");
  EXPECT_TRUE(can_resolve_to_scalar(*field1.type()));

  // Give the type an alias, then adapt the alias:
  //   typedef binary (cpp.type="folly::IOBuf") AdaptedTypeDef
  //       (cpp.adapter="MyAdapter")
  //
  // Moving the adapter annotation to the typedef itself (or any user defined
  // type), is somewhat nonsensical and means the type should be defined
  // normally, but all uses of the type should be adapted.
  t_typedef adapted_typedef(&program_, "AdaptedTypeDef", iobuf_type);
  adapted_typedef.set_annotation("cpp.adapter", "MyAdapter2");
  EXPECT_EQ(
      get_type_name(adapted_typedef),
      "::apache::thrift::adapt_detail::adapted_t<MyAdapter2, ::path::to::AdaptedTypeDef>");
  EXPECT_EQ(get_standard_type_name(adapted_typedef), "folly::IOBuf");
  EXPECT_TRUE(can_resolve_to_scalar(adapted_typedef));

  // 2: AdaptedTypeDef field2
  t_field field2(adapted_typedef, "field2", 2);
  EXPECT_EQ(
      get_storage_type_name(field2),
      "::apache::thrift::adapt_detail::adapted_t<MyAdapter2, ::path::to::AdaptedTypeDef>");
  EXPECT_TRUE(can_resolve_to_scalar(*field2.type()));
}

std::unique_ptr<t_const_value> make_string(const char* value) {
  auto name = std::make_unique<t_const_value>();
  name->set_string(value);
  return name;
}

struct adapter_builder {
  t_program* program;
  t_struct type;

  explicit adapter_builder(t_program* p) : program(p), type(p, "Adapter") {
    type.set_annotation(
        "thrift.uri", "facebook.com/thrift/annotation/cpp/Adapter");
  }

  std::unique_ptr<t_const> make() & {
    auto map = std::make_unique<t_const_value>();
    map->set_map();
    map->add_map(make_string("name"), make_string("MyAdapter"));
    map->set_ttype(t_type_ref::from_ptr(&type));
    return std::make_unique<t_const>(program, &type, "", std::move(map));
  }
};

TEST_F(TypeResolverTest, AdaptedFieldType) {
  auto i64 = t_base_type::t_i64();
  auto field = t_field(i64, "n", 42);
  auto adapter = adapter_builder(&program_);
  field.add_structured_annotation(adapter.make());
  EXPECT_EQ(
      get_storage_type_name(field),
      "::apache::thrift::adapt_detail::adapted_field_t<"
      "MyAdapter, 42, ::std::int64_t, __fbthrift_cpp2_type>");
}

TEST_F(TypeResolverTest, TransitivelyAdaptedFieldType) {
  auto annotation = t_struct(nullptr, "MyAnnotation");

  auto adapter = adapter_builder(&program_);
  annotation.add_structured_annotation(adapter.make());

  auto transitive = t_struct(nullptr, "Transitive");
  transitive.set_annotation(
      "thrift.uri", "facebook.com/thrift/annotation/Transitive");
  annotation.add_structured_annotation(
      std::make_unique<t_const>(&program_, &transitive, "", nullptr));

  auto i64 = t_base_type::t_i64();
  auto field1 = t_field(i64, "field1", 1);
  field1.add_structured_annotation(
      std::make_unique<t_const>(&program_, &annotation, "", nullptr));
  EXPECT_EQ(
      get_storage_type_name(field1),
      "::apache::thrift::adapt_detail::adapted_field_t<"
      "MyAdapter, 1, ::std::int64_t, __fbthrift_cpp2_type>");

  auto field2 = t_field(i64, "field2", 42);
  field2.add_structured_annotation(
      std::make_unique<t_const>(&program_, &annotation, "", nullptr));
  EXPECT_EQ(
      get_storage_type_name(field2),
      "::apache::thrift::adapt_detail::adapted_field_t<"
      "MyAdapter, 42, ::std::int64_t, __fbthrift_cpp2_type>");
}

} // namespace
} // namespace apache::thrift::compiler::gen::cpp
