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
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>

#include <folly/portability/GTest.h>

namespace apache::thrift::type {
template <typename...>
struct TestTemplate;

struct TestAdapter;
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

// Helper that produces a compile time error (with the types of the tags) if the
// tags do not match. For example:
//   static_assert(same_tag<bool_t, void_t>);
// Will produce an error message similar to:
//   implicit instantiation of undefined template 'SameTag<bool_t, void_t>'
template <typename Expected, typename Actual>
struct SameTag;
template <typename T>
struct SameTag<T, T> : std::true_type {};
template <typename Expected, typename Actual>
constexpr bool same_tag = SameTag<Expected, Actual>::value;

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

} // namespace
} // namespace apache::thrift::type
