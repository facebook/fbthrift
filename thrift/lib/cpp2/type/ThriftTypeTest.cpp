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

#include <folly/portability/GTest.h>

namespace apache::thrift::type {
namespace {

template <typename...>
struct TestTemplate;

// is_concrete static asserts.
static_assert(!is_concrete_v<int>);
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
static_assert(!is_concrete_v<enum_c>);
static_assert(!is_concrete_v<struct_c>);
static_assert(!is_concrete_v<union_c>);
static_assert(!is_concrete_v<exception_c>);
static_assert(!is_concrete_v<list_c>);
static_assert(!is_concrete_v<set_c>);
static_assert(!is_concrete_v<map_c>);

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

// is_thrift_type_tag_v static asserts.
static_assert(!is_thrift_type_tag_v<int>);
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
static_assert(is_thrift_type_tag_v<enum_c>);
static_assert(is_thrift_type_tag_v<struct_c>);
static_assert(is_thrift_type_tag_v<union_c>);
static_assert(is_thrift_type_tag_v<exception_c>);
static_assert(is_thrift_type_tag_v<list_c>);
static_assert(is_thrift_type_tag_v<set_c>);
static_assert(is_thrift_type_tag_v<map_c>);

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

} // namespace
} // namespace apache::thrift::type
