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
#include <vector>

#include <folly/portability/GMock.h>
#include <folly/portability/GTest.h>

#include <thrift/compiler/ast/t_base_type.h>
#include <thrift/compiler/ast/t_exception.h>
#include <thrift/compiler/ast/t_field.h>
#include <thrift/compiler/ast/t_map.h>
#include <thrift/compiler/ast/t_paramlist.h>
#include <thrift/compiler/ast/t_program.h>
#include <thrift/compiler/ast/t_struct.h>
#include <thrift/compiler/ast/t_type.h>
#include <thrift/compiler/ast/t_typedef.h>
#include <thrift/compiler/ast/t_union.h>

namespace apache::thrift::compiler {
namespace {

class UtilTest : public ::testing::Test {};

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
    EXPECT_TRUE(is_eligible_for_constexpr(&s));
  }
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

TEST_F(UtilTest, for_each_transitive_field) {
  auto program = t_program("path/to/program.thrift");
  auto empty = t_struct(&program, "struct_name");
  cpp2::for_each_transitive_field(&empty, [](const t_field*) {
    ADD_FAILURE();
    return true;
  });
  //            a
  //           / \
  //          b   c
  //         / \
  //        d   e
  //             \
  //              f
  auto i32 = t_base_type::t_i32();
  auto a = t_struct(&program, "a");
  auto b = t_struct(&program, "b");
  auto e = t_struct(&program, "e");
  a.append(std::make_unique<t_field>(&b, "b", 1));
  a.append(std::make_unique<t_field>(&i32, "c", 2));
  b.append(std::make_unique<t_field>(&i32, "d", 1));
  b.append(std::make_unique<t_field>(&e, "e", 2));
  e.append(std::make_unique<t_field>(&i32, "f", 1));

  auto fields = std::vector<std::string>();
  cpp2::for_each_transitive_field(&a, [&](const t_field* f) {
    fields.push_back(f->get_name());
    return true;
  });
  EXPECT_THAT(fields, testing::ElementsAreArray({"b", "d", "e", "f", "c"}));

  fields = std::vector<std::string>();
  cpp2::for_each_transitive_field(&a, [&](const t_field* f) {
    auto name = f->get_name();
    fields.push_back(name);
    return name != "e"; // Stop at e.
  });
  EXPECT_THAT(fields, testing::ElementsAreArray({"b", "d", "e"}));

  auto depth = 1'000'000;
  auto structs = std::vector<std::unique_ptr<t_struct>>();
  structs.reserve(depth);
  structs.push_back(std::make_unique<t_paramlist>(&program));
  for (int i = 1; i < depth; ++i) {
    structs.push_back(std::make_unique<t_paramlist>(&program));
    structs[i - 1]->append(
        std::make_unique<t_field>(structs[i].get(), "field", 1));
  }
  auto count = 0;
  cpp2::for_each_transitive_field(structs.front().get(), [&](const t_field*) {
    ++count;
    return true;
  });
  EXPECT_EQ(count, depth - 1);
}

TEST_F(UtilTest, field_transitively_refers_to_unique) {
  auto i = t_base_type::t_i32();
  auto li = t_list(&i);
  auto lli = t_list(t_list(&i));
  auto si = t_set(&i);
  auto mii = t_map(&i, &i);

  const t_type* no_uniques[] = {&i, &li, &lli, &si, &mii};
  for (const auto* no_unique : no_uniques) {
    // no_unique f;
    auto f = t_field(no_unique, "f", 1);
    EXPECT_FALSE(cpp2::field_transitively_refers_to_unique(&f));
  }

  // typedef binary (cpp.type = "std::unique_ptr<folly::IOBuf>") IOBufPtr;
  auto p = t_base_type::t_binary();
  p.set_annotation("cpp.type", "std::unique_ptr<folly::IOBuf>");

  auto lp = t_list(&p);
  auto llp = t_list(t_list(&p));
  auto sp = t_set(&p);
  auto mip = t_map(&i, &p);

  const t_type* uniques[] = {&p, &lp, &llp, &sp, &mip};
  for (const auto* unique : uniques) {
    // unique f;
    auto f = t_field(unique, "f", 1);
    EXPECT_TRUE(cpp2::field_transitively_refers_to_unique(&f));
  }

  const t_type* types[] = {&i, &li, &si, &mii, &p, &lp, &sp, &mip};
  for (const auto* type : types) {
    // type r (cpp.ref = "true");
    auto r = t_field(type, "r", 1);
    r.set_annotation("cpp.ref", "true");
    EXPECT_TRUE(cpp2::field_transitively_refers_to_unique(&r));

    // type u (cpp.ref_type = "unique");
    auto u = t_field(type, "u", 1);
    u.set_annotation("cpp.ref_type", "unique");
    EXPECT_TRUE(cpp2::field_transitively_refers_to_unique(&u));

    // type s (cpp.ref_type = "shared");
    auto s = t_field(type, "s", 1);
    s.set_annotation("cpp.ref_type", "shared");
    EXPECT_FALSE(cpp2::field_transitively_refers_to_unique(&s));
  }
}

TEST_F(UtilTest, get_gen_type_class) {
  // a single example as demo
  EXPECT_EQ(
      "::apache::thrift::type_class::string",
      cpp2::get_gen_type_class(t_base_type::t_string()));
}
} // namespace
} // namespace apache::thrift::compiler
