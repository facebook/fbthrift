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

#include <thrift/lib/cpp2/type/ThriftType.h>

#include <list>
#include <unordered_map>
#include <unordered_set>

#include <folly/portability/GTest.h>
#include <thrift/lib/cpp2/type/Tag.h>
#include <thrift/lib/cpp2/type/Testing.h>

namespace apache::thrift::type {
namespace {
using test::TestAdapter;
using test::TestTemplate;

// Evil types trying to inject themselves into the system!
struct evil_c : enum_c {};
struct evil_t : i64_t {};

// is_concrete static asserts.
static_assert(!is_concrete_v<int>);
static_assert(!is_concrete_v<evil_c>);
static_assert(!is_concrete_v<evil_t>);

static_assert(!is_concrete_v<integral_c>);
static_assert(!is_concrete_v<floating_point_c>);
static_assert(!is_concrete_v<enum_c>);
static_assert(!is_concrete_v<struct_except_c>);
static_assert(!is_concrete_v<struct_c>);
static_assert(!is_concrete_v<union_c>);
static_assert(!is_concrete_v<exception_c>);
static_assert(!is_concrete_v<list_c>);
static_assert(!is_concrete_v<set_c>);
static_assert(!is_concrete_v<map_c>);

static_assert(is_concrete_v<void_t>);
static_assert(is_concrete_v<bool_t>);
static_assert(is_concrete_v<byte_t>);
static_assert(is_concrete_v<i16_t>);
static_assert(is_concrete_v<i32_t>);
static_assert(is_concrete_v<i64_t>);
static_assert(is_concrete_v<float_t>);
static_assert(is_concrete_v<double_t>);
static_assert(is_concrete_v<string_t>);
static_assert(is_concrete_v<binary_t>);

static_assert(is_concrete_v<enum_t<int>>);
static_assert(is_concrete_v<struct_t<int>>);
static_assert(is_concrete_v<union_t<int>>);
static_assert(is_concrete_v<exception_t<int>>);

static_assert(!is_concrete_v<list<int>>);
static_assert(is_concrete_v<list<void_t>>);
static_assert(is_concrete_v<list<void_t, TestTemplate>>);

static_assert(!is_concrete_v<set<int>>);
static_assert(is_concrete_v<set<void_t>>);
static_assert(is_concrete_v<set<void_t, TestTemplate>>);

static_assert(!is_concrete_v<map<int, void_t>>);
static_assert(!is_concrete_v<map<int, int>>);
static_assert(!is_concrete_v<map<void_t, int>>);
static_assert(is_concrete_v<map<void_t, void_t>>);
static_assert(is_concrete_v<map<void_t, void_t, TestTemplate>>);

static_assert(!is_concrete_v<adapted<int, int>>);
static_assert(is_concrete_v<adapted<int, void_t>>);
static_assert(is_concrete_v<list<adapted<int, void_t>>>);

static_assert(!is_concrete_v<cpp_type<int, int>>);
static_assert(is_concrete_v<cpp_type<int, void_t>>);
static_assert(is_concrete_v<list<cpp_type<int, void_t>>>);

// is_thrift_type_tag_v static asserts.
static_assert(!is_thrift_type_tag_v<int>);
static_assert(!is_thrift_type_tag_v<evil_c>);
static_assert(!is_thrift_type_tag_v<evil_t>);

static_assert(is_thrift_type_tag_v<integral_c>);
static_assert(is_thrift_type_tag_v<floating_point_c>);
static_assert(is_thrift_type_tag_v<enum_c>);
static_assert(is_thrift_type_tag_v<struct_except_c>);
static_assert(is_thrift_type_tag_v<struct_c>);
static_assert(is_thrift_type_tag_v<union_c>);
static_assert(is_thrift_type_tag_v<exception_c>);
static_assert(is_thrift_type_tag_v<list_c>);
static_assert(is_thrift_type_tag_v<set_c>);
static_assert(is_thrift_type_tag_v<map_c>);

static_assert(is_thrift_type_tag_v<void_t>);
static_assert(is_thrift_type_tag_v<bool_t>);
static_assert(is_thrift_type_tag_v<byte_t>);
static_assert(is_thrift_type_tag_v<i16_t>);
static_assert(is_thrift_type_tag_v<i32_t>);
static_assert(is_thrift_type_tag_v<i64_t>);
static_assert(is_thrift_type_tag_v<float_t>);
static_assert(is_thrift_type_tag_v<double_t>);
static_assert(is_thrift_type_tag_v<string_t>);
static_assert(is_thrift_type_tag_v<binary_t>);

static_assert(is_thrift_type_tag_v<enum_t<int>>);
static_assert(is_thrift_type_tag_v<struct_t<int>>);
static_assert(is_thrift_type_tag_v<union_t<int>>);
static_assert(is_thrift_type_tag_v<exception_t<int>>);

static_assert(!is_thrift_type_tag_v<list<int>>);
static_assert(is_thrift_type_tag_v<list<void_t>>);
static_assert(is_thrift_type_tag_v<list<void_t, TestTemplate>>);

static_assert(!is_thrift_type_tag_v<set<int>>);
static_assert(is_thrift_type_tag_v<set<void_t>>);
static_assert(is_thrift_type_tag_v<set<void_t, TestTemplate>>);

static_assert(!is_thrift_type_tag_v<map<int, int>>);
static_assert(!is_thrift_type_tag_v<map<int, void_t>>);
static_assert(!is_thrift_type_tag_v<map<void_t, int>>);
static_assert(is_thrift_type_tag_v<map<void_t, void_t>>);
static_assert(is_thrift_type_tag_v<map<void_t, void_t, TestTemplate>>);

static_assert(!is_thrift_type_tag_v<adapted<int, int>>);
static_assert(is_thrift_type_tag_v<adapted<int, void_t>>);
static_assert(is_thrift_type_tag_v<list<adapted<int, void_t>>>);
static_assert(!is_thrift_type_tag_v<cpp_type<int, int>>);
static_assert(is_thrift_type_tag_v<cpp_type<int, void_t>>);
static_assert(is_thrift_type_tag_v<list<cpp_type<int, void_t>>>);

// is_not_concrete static asserts.
static_assert(!is_not_concrete_v<int>);
static_assert(!is_not_concrete_v<evil_c>);
static_assert(!is_not_concrete_v<evil_t>);

static_assert(is_not_concrete_v<integral_c>);
static_assert(is_not_concrete_v<floating_point_c>);
static_assert(is_not_concrete_v<enum_c>);
static_assert(is_not_concrete_v<struct_except_c>);
static_assert(is_not_concrete_v<struct_c>);
static_assert(is_not_concrete_v<union_c>);
static_assert(is_not_concrete_v<exception_c>);
static_assert(is_not_concrete_v<list_c>);
static_assert(is_not_concrete_v<set_c>);
static_assert(is_not_concrete_v<map_c>);

static_assert(!is_not_concrete_v<void_t>);
static_assert(!is_not_concrete_v<bool_t>);
static_assert(!is_not_concrete_v<byte_t>);
static_assert(!is_not_concrete_v<i16_t>);
static_assert(!is_not_concrete_v<i32_t>);
static_assert(!is_not_concrete_v<i64_t>);
static_assert(!is_not_concrete_v<float_t>);
static_assert(!is_not_concrete_v<double_t>);
static_assert(!is_not_concrete_v<string_t>);
static_assert(!is_not_concrete_v<binary_t>);

static_assert(!is_not_concrete_v<enum_t<int>>);
static_assert(!is_not_concrete_v<struct_t<int>>);
static_assert(!is_not_concrete_v<union_t<int>>);
static_assert(!is_not_concrete_v<exception_t<int>>);

static_assert(!is_not_concrete_v<list<int>>);
static_assert(!is_not_concrete_v<list<void_t>>);
static_assert(!is_not_concrete_v<list<void_t, TestTemplate>>);

static_assert(!is_not_concrete_v<set<int>>);
static_assert(!is_not_concrete_v<set<void_t>>);
static_assert(!is_not_concrete_v<set<void_t, TestTemplate>>);

static_assert(!is_not_concrete_v<map<int, void_t>>);
static_assert(!is_not_concrete_v<map<int, int>>);
static_assert(!is_not_concrete_v<map<void_t, int>>);
static_assert(!is_not_concrete_v<map<void_t, void_t>>);
static_assert(!is_not_concrete_v<map<void_t, void_t, TestTemplate>>);

static_assert(!is_not_concrete_v<adapted<int, int>>);
static_assert(!is_not_concrete_v<adapted<int, void_t>>);
static_assert(is_concrete_v<list<adapted<int, void_t>>>);

// Test concrete helpers.
template <typename Tag>
constexpr if_concrete<Tag, bool> isConcrete() {
  return true;
}

template <typename Tag>
constexpr if_not_concrete<Tag, bool> isConcrete() {
  return false;
}

// Uncomment to produce expected compile time error.
// static_assert(isConcrete<int>());
static_assert(isConcrete<void_t>());
static_assert(!isConcrete<enum_c>());

// Containers are concrete if their type parameters are concrete.
static_assert(isConcrete<list<void_t>>());
static_assert(isConcrete<list<void_t, std::list>>());
static_assert(!isConcrete<list<enum_c>>());
static_assert(!isConcrete<list<enum_c, std::list>>());

static_assert(isConcrete<set<void_t>>());
static_assert(isConcrete<set<void_t, std::unordered_set>>());
static_assert(!isConcrete<set<enum_c>>());
static_assert(!isConcrete<set<enum_c, std::unordered_set>>());

static_assert(isConcrete<map<void_t, void_t>>());
static_assert(isConcrete<map<void_t, void_t, std::unordered_map>>());
static_assert(!isConcrete<map<enum_c, void_t>>());
static_assert(!isConcrete<map<enum_c, void_t, std::unordered_map>>());
static_assert(!isConcrete<map<void_t, enum_c>>());
static_assert(!isConcrete<map<void_t, enum_c, std::unordered_map>>());
static_assert(!isConcrete<map<enum_c, enum_c>>());
static_assert(!isConcrete<map<enum_c, enum_c, std::unordered_map>>());

// An adapted type is concrete if it's type parameter is concrete.
static_assert(isConcrete<adapted<TestAdapter, void_t>>());
static_assert(!isConcrete<adapted<TestAdapter, enum_c>>());
static_assert(isConcrete<cpp_type<void, void_t>>());
static_assert(!isConcrete<cpp_type<int, enum_c>>());

} // namespace
} // namespace apache::thrift::type
