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

#include <thrift/lib/cpp2/pmr/Rebind.h>

#include <array>
#include <cstddef>
#include <deque>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <memory_resource>
#include <set>
#include <string>
#include <string_view>
#include <tuple>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

namespace {

template <class T, class A = std::allocator<T>>
struct UserVector : std::vector<T, A> {};

template <class T>
struct UserAllocator {
  using value_type = T;
  UserAllocator() = default;
  template <class U>
  explicit UserAllocator(const UserAllocator<U>&) noexcept {}
  T* allocate(std::size_t n) { return std::allocator<T>{}.allocate(n); }
  void deallocate(T* p, std::size_t n) noexcept {
    std::allocator<T>{}.deallocate(p, n);
  }
  friend bool operator==(const UserAllocator&, const UserAllocator&) = default;
};

} // namespace

namespace apache::thrift::pmr {
// NOLINTNEXTLINE(facebook-hte-MisplacedTemplateSpecialization)
template <class T, class A>
struct Rebind<UserVector<T, A>> {
  using type = UserVector<
      typename Rebind<T>::type,
      std::pmr::polymorphic_allocator<typename Rebind<T>::type>>;
};
} // namespace apache::thrift::pmr

namespace {

using apache::thrift::pmr::Rebind;

template <class T, class Expected>
constexpr bool kRebindsTo = std::is_same_v<typename Rebind<T>::type, Expected>;

template <class T>
constexpr bool kHasDtorTraits = requires { Rebind<T>::side_effect_free_dtor; };

template <class T>
constexpr bool kTrusted = Rebind<T>::side_effect_free_dtor &&
    Rebind<T>::bypass_constexpr_dtor_check_internal_do_not_use;

enum class Color { Red };
struct Struct {
  int i;
  std::string s;
};

static_assert(kRebindsTo<int, int>);
static_assert(kRebindsTo<double, double>);
static_assert(kRebindsTo<Color, Color>);
static_assert(kRebindsTo<Struct, Struct>);

static_assert(kRebindsTo<std::string, std::pmr::string>);
static_assert(kRebindsTo<std::u16string, std::pmr::u16string>);
static_assert(kRebindsTo<std::vector<int>, std::pmr::vector<int>>);
static_assert(kRebindsTo<std::vector<Struct>, std::pmr::vector<Struct>>);
static_assert(
    kRebindsTo<std::vector<std::string>, std::pmr::vector<std::pmr::string>>);
static_assert(
    kRebindsTo<std::deque<std::string>, std::pmr::deque<std::pmr::string>>);
static_assert(
    kRebindsTo<std::list<std::string>, std::pmr::list<std::pmr::string>>);
static_assert(
    kRebindsTo<std::set<std::string>, std::pmr::set<std::pmr::string>>);
static_assert(kRebindsTo<
              std::multiset<std::string>,
              std::pmr::multiset<std::pmr::string>>);
static_assert(kRebindsTo<
              std::map<std::string, std::string>,
              std::pmr::map<std::pmr::string, std::pmr::string>>);
static_assert(kRebindsTo<
              std::multimap<std::string, std::string>,
              std::pmr::multimap<std::pmr::string, std::pmr::string>>);
static_assert(kRebindsTo<
              std::unordered_set<std::string>,
              std::pmr::unordered_set<std::pmr::string>>);
static_assert(kRebindsTo<
              std::unordered_multiset<std::string>,
              std::pmr::unordered_multiset<std::pmr::string>>);
static_assert(kRebindsTo<
              std::unordered_map<std::string, std::string>,
              std::pmr::unordered_map<std::pmr::string, std::pmr::string>>);
static_assert(
    kRebindsTo<
        std::unordered_multimap<std::string, std::string>,
        std::pmr::unordered_multimap<std::pmr::string, std::pmr::string>>);

static_assert(kRebindsTo<
              std::vector<std::map<std::string, std::vector<std::string>>>,
              std::pmr::vector<std::pmr::map<
                  std::pmr::string,
                  std::pmr::vector<std::pmr::string>>>>);
static_assert(kRebindsTo<
              std::unordered_map<int, std::set<std::string>>,
              std::pmr::unordered_map<int, std::pmr::set<std::pmr::string>>>);

// Rebound types map to themselves.
static_assert(kRebindsTo<std::pmr::string, std::pmr::string>);
static_assert(kRebindsTo<
              std::pmr::vector<std::pmr::map<
                  std::pmr::string,
                  std::pmr::vector<std::pmr::string>>>,
              std::pmr::vector<std::pmr::map<
                  std::pmr::string,
                  std::pmr::vector<std::pmr::string>>>>);
static_assert(kRebindsTo<
              std::pmr::unordered_set<std::pmr::string>,
              std::pmr::unordered_set<std::pmr::string>>);

// Custom allocators are replaced.
static_assert(
    kRebindsTo<std::vector<int, UserAllocator<int>>, std::pmr::vector<int>>);
static_assert(
    kRebindsTo<
        std::basic_string<char, std::char_traits<char>, UserAllocator<char>>,
        std::pmr::string>);
static_assert(kRebindsTo<
              std::map<
                  std::string,
                  std::string,
                  std::less<std::string>,
                  UserAllocator<std::pair<const std::string, std::string>>>,
              std::pmr::map<std::pmr::string, std::pmr::string>>);

// std::less, std::greater, std::hash and std::equal_to of any key type the
// container rebinds are replaced; other functors are kept.
static_assert(kRebindsTo<
              std::set<std::vector<std::string>>,
              std::pmr::set<std::pmr::vector<std::pmr::string>>>);
static_assert(kRebindsTo<
              std::set<std::string, std::greater<std::string>>,
              std::pmr::set<std::pmr::string, std::greater<std::pmr::string>>>);
static_assert(kRebindsTo<
              std::set<std::string, std::less<>>,
              std::pmr::set<std::pmr::string, std::less<>>>);
static_assert(kRebindsTo<
              std::set<std::string, std::less<std::string_view>>,
              std::pmr::set<std::pmr::string, std::less<std::string_view>>>);
static_assert(kRebindsTo<
              std::map<std::string, int, std::greater<>>,
              std::pmr::map<std::pmr::string, int, std::greater<>>>);
static_assert(kRebindsTo<
              std::unordered_set<
                  std::string,
                  std::hash<std::string_view>,
                  std::equal_to<>>,
              std::pmr::unordered_set<
                  std::pmr::string,
                  std::hash<std::string_view>,
                  std::equal_to<>>>);
static_assert(
    kRebindsTo<
        std::unordered_map<std::string, int, std::hash<std::string_view>>,
        std::pmr::
            unordered_map<std::pmr::string, int, std::hash<std::string_view>>>);

// Thrift structs are often incomplete where their containers are rebound:
// recursive structs, and structs defined later in the file.
struct Incomplete;
static_assert(
    kRebindsTo<std::vector<Incomplete>, std::pmr::vector<Incomplete>>);
static_assert(kRebindsTo<std::set<Incomplete>, std::pmr::set<Incomplete>>);
static_assert(kRebindsTo<
              std::map<std::string, Incomplete>,
              std::pmr::map<std::pmr::string, Incomplete>>);

struct Node {
  Rebind<std::vector<Node>>::type children;
};
static_assert(std::is_same_v<decltype(Node::children), std::pmr::vector<Node>>);

struct Later;
struct Earlier {
  Rebind<std::vector<Later>>::type later;
};
struct Later {
  Rebind<std::vector<Earlier>>::type earlier;
};
static_assert(
    std::is_same_v<decltype(Earlier::later), std::pmr::vector<Later>>);
static_assert(
    std::is_same_v<decltype(Later::earlier), std::pmr::vector<Earlier>>);

// A user specialization takes precedence over the error for unknown class
// templates, and composes with Thrift's.
static_assert(kRebindsTo<
              std::vector<UserVector<std::string>>,
              std::pmr::vector<UserVector<
                  std::pmr::string,
                  std::pmr::polymorphic_allocator<std::pmr::string>>>>);

static_assert(kTrusted<std::string>);
static_assert(kTrusted<std::vector<int>>);
static_assert(kTrusted<std::deque<int>>);
static_assert(kTrusted<std::list<int>>);
static_assert(kTrusted<std::set<int>>);
static_assert(kTrusted<std::multiset<int>>);
static_assert(kTrusted<std::map<int, int>>);
static_assert(kTrusted<std::multimap<int, int>>);
static_assert(kTrusted<std::unordered_set<int>>);
static_assert(kTrusted<std::unordered_multiset<int>>);
static_assert(kTrusted<std::unordered_map<int, int>>);
static_assert(kTrusted<std::unordered_multimap<int, int>>);
static_assert(!kHasDtorTraits<int>);
static_assert(!kHasDtorTraits<Struct>);
static_assert(!kHasDtorTraits<UserVector<int>>);

class DefaultResourceGuard {
 public:
  explicit DefaultResourceGuard(std::pmr::memory_resource* r)
      : prev_(std::pmr::set_default_resource(r)) {}
  DefaultResourceGuard(const DefaultResourceGuard&) = delete;
  DefaultResourceGuard& operator=(const DefaultResourceGuard&) = delete;
  ~DefaultResourceGuard() { std::pmr::set_default_resource(prev_); }

 private:
  std::pmr::memory_resource* prev_;
};

TEST(RebindTest, NestedValuesUseTheContainerResource) {
  constexpr std::string_view kText =
      "a string too long for the small string buffer";
  std::array<std::byte, 4096> buffer{};
  std::pmr::monotonic_buffer_resource arena(
      buffer.data(), buffer.size(), std::pmr::null_memory_resource());
  // Any allocation that does not come from `arena` throws.
  DefaultResourceGuard guard(std::pmr::null_memory_resource());

  Rebind<std::vector<std::map<std::string, std::vector<std::string>>>>::type
      values(&arena);
  auto& map = values.emplace_back();
  auto& [key, strings] = *map.emplace(
                                 std::piecewise_construct,
                                 std::forward_as_tuple(kText),
                                 std::forward_as_tuple())
                              .first;
  strings.emplace_back(kText);

  EXPECT_EQ(map.get_allocator().resource(), &arena);
  EXPECT_EQ(key.get_allocator().resource(), &arena);
  EXPECT_EQ(strings.get_allocator().resource(), &arena);
  EXPECT_EQ(strings.front().get_allocator().resource(), &arena);
  EXPECT_EQ(key, kText);
  EXPECT_EQ(strings.front(), kText);
}

} // namespace
