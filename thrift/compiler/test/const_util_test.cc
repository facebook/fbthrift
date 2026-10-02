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

#include <thrift/compiler/generate/const_util.h>

#include <gtest/gtest.h>
#include <folly/container/View.h>
#include <thrift/compiler/test/gen-cpp2/const_util_test_types.h>

using apache::thrift::compiler::t_const_value;
using apache::thrift::compiler::t_primitive_type;
using apache::thrift::compiler::t_set;

namespace {
template <typename... Args>
std::unique_ptr<t_const_value> val(Args&&... args) {
  return std::make_unique<t_const_value>(std::forward<Args>(args)...);
}
template <typename Enum, typename = std::enable_if_t<std::is_enum_v<Enum>>>
std::unique_ptr<t_const_value> val(Enum val) {
  return std::make_unique<t_const_value>(
      static_cast<std::underlying_type_t<Enum>>(val));
}
} // namespace

TEST(ConstUtilTest, HydrateConst) {
  auto outer = t_const_value::make_map();
  auto dbl = val();
  dbl->set_double(42.0);
  outer->add_map(val("floatField"), std::move(dbl));

  auto list = t_const_value::make_list();
  list->add_list(val(42));
  auto inner = t_const_value::make_map();
  inner->add_map(val("listField"), std::move(list));
  outer->add_map(val("unionField"), std::move(inner));

  outer->add_map(val("enumField"), val(cpp2::E::B));

  cpp2::Outer s;
  hydrate_const(s, *outer);
  EXPECT_EQ(*s.floatField(), 42.0);
  EXPECT_EQ(s.unionField()->listField_ref()->at(0), 42);
  EXPECT_EQ(s.unionField()->listField_ref()->size(), 1);
  EXPECT_EQ(*s.enumField(), cpp2::E::B);
}

TEST(ConstUtilTest, ConstToValue) {
  auto str = val("foo");
  EXPECT_EQ(const_to_value(*str).as_string(), "foo");

  auto map = t_const_value::make_map();
  map->add_map(val("first"), val(1));
  map->add_map(val("second"), val(2));
  map->add_map(val("first"), val(3));
  auto value = const_to_value(*map);
  const auto& converted_map = value.as_map();
  EXPECT_EQ(converted_map.at(const_to_value(*val("first"))).as_i64(), 1);
  EXPECT_EQ(converted_map.at(const_to_value(*val("second"))).as_i64(), 2);

  auto set = t_const_value::make_list();
  set->add_list(val(1));
  set->add_list(val(2));
  set->add_list(val(1));
  t_set set_type(t_primitive_type::t_i32());
  auto converted = const_to_value(*set, &set_type);
  // The order serialization writes, which is not the set's iteration order.
  auto written = folly::order_preserving_reinsertion_view(converted.as_set());
  auto it = written.begin();
  EXPECT_EQ((it++)->as_i32(), 1);
  EXPECT_EQ((it++)->as_i32(), 2);
  EXPECT_EQ(it, written.end());
}
