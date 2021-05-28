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

#include <thrift/compiler/ast/t_program.h>
#include <thrift/compiler/ast/t_typedef.h>

#include <folly/portability/GTest.h>

namespace apache {
namespace thrift {
namespace compiler {
namespace {

TEST(TypeDefTest, InheritedAnnotations) {
  t_program program("test");
  t_scope scope;
  t_typedef t1(&program, &t_base_type::t_i32(), "t1", &scope);
  t_typedef t2(&program, &t1, "t2", &scope);
  t_typedef t3(&program, &t2, "t3", &scope);
  const t_type* p1(&t1);
  const t_type* p2(&t2);
  const t_type* p3(&t3);

  EXPECT_EQ(p1->get_annotation({"foo1", "foo2"}), "");
  EXPECT_EQ(p2->get_annotation({"foo1", "foo2"}), "");
  EXPECT_EQ(p3->get_annotation({"foo1", "foo2"}), "");
  EXPECT_EQ(t_typedef::get_first_annotation(p1, {"foo1", "foo2"}), "");
  EXPECT_EQ(t_typedef::get_first_annotation(p2, {"foo1", "foo2"}), "");
  EXPECT_EQ(t_typedef::get_first_annotation(p3, {"foo1", "foo2"}), "");
  EXPECT_EQ(t_typedef::get_first_annotation(p1, {"foo1"}), "");
  EXPECT_EQ(t_typedef::get_first_annotation(p2, {"foo1"}), "");
  EXPECT_EQ(t_typedef::get_first_annotation(p3, {"foo1"}), "");
  EXPECT_EQ(t_typedef::get_first_annotation(p1, {"foo2"}), "");
  EXPECT_EQ(t_typedef::get_first_annotation(p2, {"foo2"}), "");
  EXPECT_EQ(t_typedef::get_first_annotation(p3, {"foo2"}), "");

  t2.set_annotation("foo2", "a");
  EXPECT_EQ(p1->get_annotation({"foo1", "foo2"}), "");
  EXPECT_EQ(p2->get_annotation({"foo1", "foo2"}), "a");
  EXPECT_EQ(p3->get_annotation({"foo1", "foo2"}), "");
  EXPECT_EQ(t_typedef::get_first_annotation(p1, {"foo1", "foo2"}), "");
  EXPECT_EQ(t_typedef::get_first_annotation(p2, {"foo1", "foo2"}), "a");
  EXPECT_EQ(t_typedef::get_first_annotation(p3, {"foo1", "foo2"}), "a");
  EXPECT_EQ(t_typedef::get_first_annotation(p1, {"foo1"}), "");
  EXPECT_EQ(t_typedef::get_first_annotation(p2, {"foo1"}), "");
  EXPECT_EQ(t_typedef::get_first_annotation(p3, {"foo1"}), "");
  EXPECT_EQ(t_typedef::get_first_annotation(p1, {"foo2"}), "");
  EXPECT_EQ(t_typedef::get_first_annotation(p2, {"foo2"}), "a");
  EXPECT_EQ(t_typedef::get_first_annotation(p3, {"foo2"}), "a");

  t1.set_annotation("foo1", "b");
  EXPECT_EQ(p1->get_annotation({"foo1", "foo2"}), "b");
  EXPECT_EQ(p2->get_annotation({"foo1", "foo2"}), "a");
  EXPECT_EQ(p3->get_annotation({"foo1", "foo2"}), "");
  EXPECT_EQ(t_typedef::get_first_annotation(p1, {"foo1", "foo2"}), "b");
  EXPECT_EQ(t_typedef::get_first_annotation(p2, {"foo1", "foo2"}), "a");
  EXPECT_EQ(t_typedef::get_first_annotation(p3, {"foo1", "foo2"}), "a");
  EXPECT_EQ(t_typedef::get_first_annotation(p1, {"foo1"}), "b");
  EXPECT_EQ(t_typedef::get_first_annotation(p2, {"foo1"}), "b");
  EXPECT_EQ(t_typedef::get_first_annotation(p3, {"foo1"}), "b");
  EXPECT_EQ(t_typedef::get_first_annotation(p1, {"foo2"}), "");
  EXPECT_EQ(t_typedef::get_first_annotation(p2, {"foo2"}), "a");
  EXPECT_EQ(t_typedef::get_first_annotation(p3, {"foo2"}), "a");

  t2.set_annotation("foo1", "c");
  EXPECT_EQ(p1->get_annotation({"foo1", "foo2"}), "b");
  EXPECT_EQ(p2->get_annotation({"foo1", "foo2"}), "c");
  EXPECT_EQ(p3->get_annotation({"foo1", "foo2"}), "");
  EXPECT_EQ(t_typedef::get_first_annotation(p1, {"foo1", "foo2"}), "b");
  EXPECT_EQ(t_typedef::get_first_annotation(p2, {"foo1", "foo2"}), "c");
  EXPECT_EQ(t_typedef::get_first_annotation(p3, {"foo1", "foo2"}), "c");
  EXPECT_EQ(t_typedef::get_first_annotation(p1, {"foo1"}), "b");
  EXPECT_EQ(t_typedef::get_first_annotation(p2, {"foo1"}), "c");
  EXPECT_EQ(t_typedef::get_first_annotation(p3, {"foo1"}), "c");
  EXPECT_EQ(t_typedef::get_first_annotation(p1, {"foo2"}), "");
  EXPECT_EQ(t_typedef::get_first_annotation(p2, {"foo2"}), "a");
  EXPECT_EQ(t_typedef::get_first_annotation(p3, {"foo2"}), "a");

  t3.set_annotation("foo2", "d");
  EXPECT_EQ(p1->get_annotation({"foo1", "foo2"}), "b");
  EXPECT_EQ(p2->get_annotation({"foo1", "foo2"}), "c");
  EXPECT_EQ(p3->get_annotation({"foo1", "foo2"}), "d");
  EXPECT_EQ(t_typedef::get_first_annotation(p1, {"foo1", "foo2"}), "b");
  EXPECT_EQ(t_typedef::get_first_annotation(p2, {"foo1", "foo2"}), "c");
  EXPECT_EQ(t_typedef::get_first_annotation(p3, {"foo1", "foo2"}), "d");
  EXPECT_EQ(t_typedef::get_first_annotation(p1, {"foo1"}), "b");
  EXPECT_EQ(t_typedef::get_first_annotation(p2, {"foo1"}), "c");
  EXPECT_EQ(t_typedef::get_first_annotation(p3, {"foo1"}), "c");
  EXPECT_EQ(t_typedef::get_first_annotation(p1, {"foo2"}), "");
  EXPECT_EQ(t_typedef::get_first_annotation(p2, {"foo2"}), "a");
  EXPECT_EQ(t_typedef::get_first_annotation(p3, {"foo2"}), "d");
}

} // namespace
} // namespace compiler
} // namespace thrift
} // namespace apache
