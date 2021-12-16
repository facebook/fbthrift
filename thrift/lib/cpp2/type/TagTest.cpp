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

#include <thrift/lib/cpp2/type/Tag.h>

#include <list>
#include <map>
#include <unordered_set>

#include <folly/portability/GTest.h>
#include <thrift/lib/cpp2/type/Testing.h>

namespace apache::thrift::type {

struct General;
struct Specialized;

void_t testOverload(const void_t&);
bool_t testOverload(const bool_t&);
byte_t testOverload(const byte_t&);
i16_t testOverload(const i16_t&);
i32_t testOverload(const i32_t&);
i64_t testOverload(const i64_t&);
float_t testOverload(const float_t&);
double_t testOverload(const double_t&);
string_t testOverload(const string_t&);
binary_t testOverload(const binary_t&);

enum_c testOverload(const enum_c&);
enum_t<Specialized> testOverload(const enum_t<Specialized>&);

struct_c testOverload(const struct_c&);
struct_t<Specialized> testOverload(const struct_t<Specialized>&);

union_c testOverload(const union_c&);
union_t<Specialized> testOverload(const union_t<Specialized>&);

exception_c testOverload(const exception_c&);
exception_t<Specialized> testOverload(const exception_t<Specialized>&);

list_c testOverload(const list_c&);
template <template <typename...> typename ListT>
list<Specialized, ListT> testOverload(const list<Specialized, ListT>&);

set_c testOverload(const set_c&);
set<Specialized, std::unordered_set> testOverload(
    const set<Specialized, std::unordered_set>&);

map_c testOverload(const map_c&);
map<Specialized, Specialized> testOverload(
    const map<Specialized, Specialized>&);

namespace {
using test::same_tag;
using test::TestAdapter;
using test::TestTemplate;

// Test that type tags can be used to find an overload in ~constant time
// by the compiler.
static_assert(same_tag<void_t, decltype(testOverload(void_t{}))>);
static_assert(same_tag<bool_t, decltype(testOverload(bool_t{}))>);
static_assert(same_tag<byte_t, decltype(testOverload(byte_t{}))>);
static_assert(same_tag<i16_t, decltype(testOverload(i16_t{}))>);
static_assert(same_tag<i32_t, decltype(testOverload(i32_t{}))>);
static_assert(same_tag<i64_t, decltype(testOverload(i64_t{}))>);
static_assert(same_tag<float_t, decltype(testOverload(float_t{}))>);
static_assert(same_tag<double_t, decltype(testOverload(double_t{}))>);
static_assert(same_tag<string_t, decltype(testOverload(string_t{}))>);
static_assert(same_tag<binary_t, decltype(testOverload(binary_t{}))>);

static_assert(same_tag<enum_c, decltype(testOverload(enum_c{}))>);
static_assert(same_tag<enum_c, decltype(testOverload(enum_t<General>{}))>);
static_assert(same_tag<
              enum_t<Specialized>,
              decltype(testOverload(enum_t<Specialized>{}))>);

static_assert(same_tag<struct_c, decltype(testOverload(struct_c{}))>);
static_assert(same_tag<struct_c, decltype(testOverload(struct_t<General>{}))>);
static_assert(same_tag<
              struct_t<Specialized>,
              decltype(testOverload(struct_t<Specialized>{}))>);

static_assert(same_tag<union_c, decltype(testOverload(union_c{}))>);
static_assert(same_tag<union_c, decltype(testOverload(union_t<General>{}))>);
static_assert(same_tag<
              union_t<Specialized>,
              decltype(testOverload(union_t<Specialized>{}))>);

static_assert(same_tag<exception_c, decltype(testOverload(exception_c{}))>);
static_assert(
    same_tag<exception_c, decltype(testOverload(exception_t<General>{}))>);
static_assert(same_tag<
              exception_t<Specialized>,
              decltype(testOverload(exception_t<Specialized>{}))>);

static_assert(same_tag<list_c, decltype(testOverload(list_c{}))>);
static_assert(same_tag<list_c, decltype(testOverload(list<void_t>{}))>);
static_assert(
    same_tag<list_c, decltype(testOverload(list<void_t, TestTemplate>{}))>);
static_assert(
    same_tag<list<Specialized>, decltype(testOverload(list<Specialized>{}))>);
static_assert(same_tag<
              list<Specialized, std::list>,
              decltype(testOverload(list<Specialized, std::list>{}))>);

static_assert(same_tag<set_c, decltype(testOverload(set_c{}))>);
static_assert(same_tag<set_c, decltype(testOverload(set<void_t>{}))>);
static_assert(
    same_tag<set_c, decltype(testOverload(set<void_t, TestTemplate>{}))>);
static_assert(same_tag<set_c, decltype(testOverload(set<Specialized>{}))>);
static_assert(same_tag<
              set<Specialized, std::unordered_set>,
              decltype(testOverload(set<Specialized, std::unordered_set>{}))>);

static_assert(same_tag<map_c, decltype(testOverload(map_c{}))>);
static_assert(same_tag<map_c, decltype(testOverload(map<void_t, void_t>{}))>);
static_assert(same_tag<
              map_c,
              decltype(testOverload(map<void_t, void_t, TestTemplate>{}))>);
static_assert(
    same_tag<map_c, decltype(testOverload(map<Specialized, void_t>{}))>);
static_assert(
    same_tag<map_c, decltype(testOverload(map<void_t, Specialized>{}))>);
static_assert(same_tag<
              map<Specialized, Specialized>,
              decltype(testOverload(map<Specialized, Specialized>{}))>);
static_assert(
    same_tag<
        map_c,
        decltype(testOverload(map<Specialized, Specialized, std::map>{}))>);

// Adapted types are convertable to the underlying tag.
static_assert(
    same_tag<void_t, decltype(testOverload(adapted<TestAdapter, void_t>{}))>);
static_assert(same_tag<
              set_c,
              decltype(testOverload(adapted<TestAdapter, set<enum_c>>{}))>);
// CppType types are convertable to the underlying tag.
static_assert(
    same_tag<void_t, decltype(testOverload(cpp_type<TestAdapter, void_t>{}))>);
static_assert(same_tag<
              set_c,
              decltype(testOverload(cpp_type<TestAdapter, set<enum_c>>{}))>);

} // namespace
} // namespace apache::thrift::type
