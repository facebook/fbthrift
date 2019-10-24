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

#ifndef THRIFT_FATAL_TRAITS_TEST_HELPERS_INL_H
#define THRIFT_FATAL_TRAITS_TEST_HELPERS_INL_H

#include <algorithm>
#include <array>
#include <iterator>
#include <unordered_set>
#include <utility>

#include <folly/portability/GTest.h>

#include <folly/Range.h>
#include <folly/Traits.h>
#include <thrift/lib/cpp2/reflection/internal/test_helpers.h>
#include <thrift/lib/cpp2/reflection/reflection.h>

namespace apache {
namespace thrift {
namespace detail {

template <typename LHS, typename RHS>
void compare_elements_eq(LHS const& lhs, RHS const& rhs) {
  EXPECT_EQ(lhs, static_cast<LHS>(rhs));
}

template <typename LHSF, typename LHSS, typename RHSF, typename RHSS>
void compare_elements_eq(
    std::pair<LHSF, LHSS> const& lhs,
    std::pair<RHSF, RHSS> const& rhs) {
  EXPECT_EQ(lhs.first, rhs.first);
  EXPECT_EQ(lhs.second, rhs.second);
}

#define THRIFT_COMPARE_ITERATORS_IMPL(TRAITS, EI, EE, AI, AE) \
  do {                                                        \
    auto ei = (EI);                                           \
    auto ee = (EE);                                           \
    auto ai = (AI);                                           \
    auto ae = (AE);                                           \
    EXPECT_EQ(std::distance(ei, ee), std::distance(ai, ae));  \
    for (; ei != ee; ++ei, ++ai) {                            \
      ASSERT_NE(ae, ai);                                      \
      EXPECT_EQ(*ei, *ai);                                    \
    }                                                         \
    EXPECT_EQ(ae, ai);                                        \
  } while (false)

#define THRIFT_COMPARE_CONTAINER_TO_ITERATORS_IMPL(TRAITS, CONTAINER, EI, EE) \
  do {                                                                        \
    EXPECT_EQ(                                                                \
        CONTAINER.size(),                                                     \
        std::distance(TRAITS::cbegin(CONTAINER), TRAITS::cend(CONTAINER)));   \
    EXPECT_EQ(                                                                \
        CONTAINER.size(),                                                     \
        std::distance(TRAITS::begin(CONTAINER), TRAITS::end(CONTAINER)));     \
    THRIFT_COMPARE_ITERATORS_IMPL(                                            \
        TRAITS, EI, EE, TRAITS::cbegin(CONTAINER), TRAITS::cend(CONTAINER));  \
    THRIFT_COMPARE_ITERATORS_IMPL(                                            \
        TRAITS, EI, EE, TRAITS::begin(CONTAINER), TRAITS::end(CONTAINER));    \
  } while (false)

#define THRIFT_COMPARE_UNORDERED_CONTAINERS_IMPL2(                   \
    TRAITS, EXPECTED, CONTAINER, BEGIN, END)                         \
  do {                                                               \
    auto ai = TRAITS::BEGIN(CONTAINER);                              \
    auto ae = TRAITS::END(CONTAINER);                                \
    EXPECT_EQ(CONTAINER.size(), std::distance(ai, ae));              \
    using expected_type =                                            \
        folly::remove_cvref_t<decltype(*(EXPECTED).begin())>;        \
    std::unordered_set<expected_type> const expected(                \
        (EXPECTED).begin(), (EXPECTED).end());                       \
    for (auto const ee = expected.end(); ai != ae; ++ai) {           \
      auto const e = expected.find(static_cast<expected_type>(*ai)); \
      ASSERT_NE(ee, e);                                              \
      ::apache::thrift::detail::compare_elements_eq(*e, *ai);        \
    }                                                                \
  } while (false)

#define THRIFT_COMPARE_UNORDERED_CONTAINERS_IMPL(TRAITS, EXPECTED, CONTAINER) \
  do {                                                                        \
    {                                                                         \
      SCOPED_TRACE("const");                                                  \
      THRIFT_COMPARE_UNORDERED_CONTAINERS_IMPL2(                              \
          TRAITS, EXPECTED, CONTAINER, cbegin, cend);                         \
    }                                                                         \
    {                                                                         \
      SCOPED_TRACE("mutable");                                                \
      THRIFT_COMPARE_UNORDERED_CONTAINERS_IMPL2(                              \
          TRAITS, EXPECTED, CONTAINER, begin, end);                           \
    }                                                                         \
  } while (false)

#define THRIFT_COMPARE_UNORDERED_CONTAINERS_IMPL2_TRAITS(     \
    TRAITS, EXPECTED, CONTAINER, BEGIN, END)                  \
  do {                                                        \
    auto ai = TRAITS::BEGIN(CONTAINER);                       \
    auto ae = TRAITS::END(CONTAINER);                         \
    EXPECT_EQ(CONTAINER.size(), std::distance(ai, ae));       \
    using expected_type = typename TRAITS::value_type;        \
    std::unordered_set<expected_type> const expected(         \
        (EXPECTED).begin(), (EXPECTED).end());                \
    for (auto const ee = expected.end(); ai != ae; ++ai) {    \
      auto const e = expected.find(expected_type(*ai));       \
      ASSERT_NE(ee, e);                                       \
      ::apache::thrift::detail::compare_elements_eq(*e, *ai); \
    }                                                         \
  } while (false)

#define THRIFT_COMPARE_UNORDERED_CONTAINERS_IMPL_TRAITS( \
    TRAITS, EXPECTED, CONTAINER)                         \
  do {                                                   \
    {                                                    \
      SCOPED_TRACE("const");                             \
      THRIFT_COMPARE_UNORDERED_CONTAINERS_IMPL2_TRAITS(  \
          TRAITS, EXPECTED, CONTAINER, cbegin, cend);    \
    }                                                    \
    {                                                    \
      SCOPED_TRACE("mutable");                           \
      THRIFT_COMPARE_UNORDERED_CONTAINERS_IMPL2_TRAITS(  \
          TRAITS, EXPECTED, CONTAINER, begin, end);      \
    }                                                    \
  } while (false)

template <typename Traits, typename T, typename Iterator>
void copy_iterators(T& s, Iterator ei, Iterator ee) {
  auto ai = Traits::begin(s);
  auto ae = Traits::end(s);
  EXPECT_EQ(std::distance(ai, ae), s.size());
  for (; ei != ee; ++ei, ++ai) {
    ASSERT_NE(ae, ai);
    *ai = *ei;
  }
  EXPECT_EQ(ae, ai);
}

struct test_data {
  static constexpr std::array<int, 10> const primes{
      {2, 3, 5, 7, 11, 13, 17, 19, 23, 29}};

#define THRIFT_DATA_ENTRY_IMPL(X, Modifier) \
  { X, X Modifier }
#define THRIFT_ARRAY_DATA_IMPL(Modifier)                                    \
  THRIFT_DATA_ENTRY_IMPL(2, Modifier), THRIFT_DATA_ENTRY_IMPL(3, Modifier), \
      THRIFT_DATA_ENTRY_IMPL(5, Modifier),                                  \
      THRIFT_DATA_ENTRY_IMPL(7, Modifier),                                  \
      THRIFT_DATA_ENTRY_IMPL(11, Modifier),                                 \
      THRIFT_DATA_ENTRY_IMPL(13, Modifier),                                 \
      THRIFT_DATA_ENTRY_IMPL(17, Modifier),                                 \
      THRIFT_DATA_ENTRY_IMPL(19, Modifier),                                 \
      THRIFT_DATA_ENTRY_IMPL(23, Modifier),                                 \
      THRIFT_DATA_ENTRY_IMPL(29, Modifier)

  static constexpr std::array<std::pair<const int, int>, 10> const primes_2x{
      {THRIFT_ARRAY_DATA_IMPL(*2)}};

  static constexpr std::array<std::pair<const int, int>, 10> const primes_3x{
      {THRIFT_ARRAY_DATA_IMPL(*3)}};

#undef THRIFT_ARRAY_DATA_IMPL
#undef THRIFT_DATA_ENTRY_IMPL
};

constexpr std::array<int, 10> const test_data::primes;
constexpr std::array<std::pair<const int, int>, 10> const test_data::primes_2x;
constexpr std::array<std::pair<const int, int>, 10> const test_data::primes_3x;

} // namespace detail

template <typename T>
void test_thrift_list_traits() {
  using traits = apache::thrift::thrift_list_traits<T>;

  EXPECT_SAME<T, typename traits::type>();
  EXPECT_SAME<typename T::value_type, typename traits::value_type>();
  EXPECT_SAME<typename T::size_type, typename traits::size_type>();
  EXPECT_SAME<typename T::iterator, typename traits::iterator>();
  EXPECT_SAME<typename T::const_iterator, typename traits::const_iterator>();

  {
    T s(detail::test_data::primes.begin(), detail::test_data::primes.end());
    THRIFT_COMPARE_UNORDERED_CONTAINERS_IMPL_TRAITS(traits, s, s);
    THRIFT_COMPARE_UNORDERED_CONTAINERS_IMPL(
        traits, detail::test_data::primes, s);

    auto other = detail::test_data::primes;
    detail::copy_iterators<traits>(s, other.begin(), other.end());
    THRIFT_COMPARE_UNORDERED_CONTAINERS_IMPL(traits, other, s);
  }

  {
    T const s(
        detail::test_data::primes.begin(), detail::test_data::primes.end());
    THRIFT_COMPARE_UNORDERED_CONTAINERS_IMPL_TRAITS(traits, s, s);
    THRIFT_COMPARE_UNORDERED_CONTAINERS_IMPL(
        traits, detail::test_data::primes, s);
  }

  {
    T s(detail::test_data::primes.begin(), detail::test_data::primes.end());
    EXPECT_FALSE(traits::empty(s));
    EXPECT_EQ(s.size(), traits::size(s));
    EXPECT_EQ(detail::test_data::primes.size(), traits::size(s));
    EXPECT_EQ(detail::test_data::primes.size(), s.size());
    traits::clear(s);
    EXPECT_EQ(s.size(), traits::size(s));
    EXPECT_TRUE(traits::empty(s));
    EXPECT_EQ(0, s.size());
  }

  {
    T s(detail::test_data::primes.begin(), detail::test_data::primes.end());
    auto sizeBefore = traits::size(s);
    EXPECT_FALSE(traits::empty(s));
    auto iter = traits::erase(s, s.begin());
    EXPECT_EQ(s.size(), traits::size(s));
    EXPECT_EQ(traits::size(s), sizeBefore - 1);
    EXPECT_EQ(*iter, *(traits::begin(s)));
  }
}

template <typename T>
void test_thrift_string_traits() {
  using traits = apache::thrift::thrift_string_traits<T>;
  folly::StringPiece const source("hello, world");
  folly::StringPiece const other("HELLO, WORLD");
  ASSERT_EQ(source.size(), other.size());

  EXPECT_SAME<T, typename traits::type>();
  EXPECT_SAME<typename T::value_type, typename traits::value_type>();
  EXPECT_SAME<typename T::size_type, typename traits::size_type>();
  EXPECT_SAME<typename T::iterator, typename traits::iterator>();
  EXPECT_SAME<typename T::const_iterator, typename traits::const_iterator>();

  {
    T s(source.begin(), source.end());
    THRIFT_COMPARE_CONTAINER_TO_ITERATORS_IMPL(traits, s, s.begin(), s.end());
    THRIFT_COMPARE_CONTAINER_TO_ITERATORS_IMPL(
        traits, s, source.begin(), source.end());
    detail::copy_iterators<traits>(s, other.begin(), other.end());
    THRIFT_COMPARE_CONTAINER_TO_ITERATORS_IMPL(
        traits, s, other.begin(), other.end());
  }

  {
    T const s(source.begin(), source.end());
    THRIFT_COMPARE_CONTAINER_TO_ITERATORS_IMPL(traits, s, s.begin(), s.end());
    THRIFT_COMPARE_CONTAINER_TO_ITERATORS_IMPL(
        traits, s, source.begin(), source.end());

    auto const data = traits::data(s);
    EXPECT_EQ(s.data(), data);
    THRIFT_COMPARE_ITERATORS_IMPL(
        traits, source.begin(), source.end(), data, std::next(data, s.size()));

    auto const c_str = traits::c_str(s);
    EXPECT_EQ(s.c_str(), c_str);
    EXPECT_EQ(0, c_str[s.size()]);
    THRIFT_COMPARE_ITERATORS_IMPL(
        traits,
        source.begin(),
        source.end(),
        c_str,
        std::next(c_str, s.size()));
  }

  {
    T s(source.begin(), source.end());
    EXPECT_FALSE(traits::empty(s));
    EXPECT_EQ(s.size(), traits::size(s));
    EXPECT_EQ(source.size(), traits::size(s));
    EXPECT_EQ(source.size(), s.size());
    traits::clear(s);
    EXPECT_EQ(s.size(), traits::size(s));
    EXPECT_TRUE(traits::empty(s));
    EXPECT_EQ(0, s.size());
  }
}

template <typename T>
void test_thrift_set_traits() {
  using traits = apache::thrift::thrift_set_traits<T>;

  EXPECT_SAME<T, typename traits::type>();
  EXPECT_SAME<typename T::key_type, typename traits::key_type>();
  EXPECT_SAME<typename T::value_type, typename traits::value_type>();
  EXPECT_SAME<typename T::size_type, typename traits::size_type>();
  EXPECT_SAME<typename T::iterator, typename traits::iterator>();
  EXPECT_SAME<typename T::const_iterator, typename traits::const_iterator>();

  {
    T s(detail::test_data::primes.begin(), detail::test_data::primes.end());
    THRIFT_COMPARE_UNORDERED_CONTAINERS_IMPL_TRAITS(traits, s, s);
    THRIFT_COMPARE_UNORDERED_CONTAINERS_IMPL(
        traits, detail::test_data::primes, s);
  }

  {
    T const s(
        detail::test_data::primes.begin(), detail::test_data::primes.end());
    THRIFT_COMPARE_UNORDERED_CONTAINERS_IMPL_TRAITS(traits, s, s);
    THRIFT_COMPARE_UNORDERED_CONTAINERS_IMPL(
        traits, detail::test_data::primes, s);
  }

  {
    T s(detail::test_data::primes.begin(), detail::test_data::primes.end());
    EXPECT_FALSE(traits::empty(s));
    EXPECT_EQ(s.size(), traits::size(s));
    EXPECT_EQ(detail::test_data::primes.size(), traits::size(s));
    EXPECT_EQ(detail::test_data::primes.size(), s.size());
    traits::clear(s);
    EXPECT_EQ(s.size(), traits::size(s));
    EXPECT_TRUE(traits::empty(s));
    EXPECT_EQ(0, s.size());
  }

  {
    T s(detail::test_data::primes.begin(), detail::test_data::primes.end());
    T const& sconst = s;
    auto k = 17;
    EXPECT_EQ(1, s.count(k));
    EXPECT_EQ(s.find(k), traits::find(s, k));
    EXPECT_EQ(sconst.find(k), traits::find(sconst, k));
  }

  {
    T s(detail::test_data::primes.begin(), detail::test_data::primes.end());
    EXPECT_FALSE(traits::empty(s));
    auto sizeBefore = traits::size(s);
    EXPECT_FALSE(traits::empty(s));

    auto count = traits::erase(s, 6);
    EXPECT_EQ(traits::size(s), sizeBefore);
    EXPECT_EQ(count, 0);

    count = traits::erase(s, 5);
    EXPECT_EQ(traits::size(s), sizeBefore - 1);
    EXPECT_EQ(count, 1);
  }
}

template <typename T>
void test_thrift_map_traits() {
  using traits = apache::thrift::thrift_map_traits<T>;

  EXPECT_SAME<T, typename traits::type>();
  EXPECT_SAME<typename T::key_type, typename traits::key_type>();
  EXPECT_SAME<typename T::mapped_type, typename traits::mapped_type>();
  EXPECT_SAME<typename T::size_type, typename traits::size_type>();
  EXPECT_SAME<typename T::iterator, typename traits::iterator>();
  EXPECT_SAME<typename T::const_iterator, typename traits::const_iterator>();

  {
    T s(detail::test_data::primes_2x.begin(),
        detail::test_data::primes_2x.end());
    THRIFT_COMPARE_UNORDERED_CONTAINERS_IMPL_TRAITS(traits, s, s);
    THRIFT_COMPARE_UNORDERED_CONTAINERS_IMPL(
        traits, detail::test_data::primes_2x, s);

    for (auto ai = traits::begin(s), ae = traits::end(s); ai != ae; ++ai) {
      traits::mapped(ai) = traits::key(ai) * 3;
    }
    for (auto ai = traits::cbegin(s), ae = traits::cend(s); ai != ae; ++ai) {
      EXPECT_EQ(traits::key(ai) * 3, traits::mapped(ai));
    }
    for (auto ai = traits::begin(s), ae = traits::end(s); ai != ae; ++ai) {
      EXPECT_EQ(traits::key(ai) * 3, traits::mapped(ai));
    }
    THRIFT_COMPARE_UNORDERED_CONTAINERS_IMPL(
        traits, detail::test_data::primes_3x, s);
  }

  {
    T const s(
        detail::test_data::primes_2x.begin(),
        detail::test_data::primes_2x.end());
    THRIFT_COMPARE_UNORDERED_CONTAINERS_IMPL_TRAITS(traits, s, s);
    THRIFT_COMPARE_UNORDERED_CONTAINERS_IMPL(
        traits, detail::test_data::primes_2x, s);
  }

  {
    T s(detail::test_data::primes_2x.begin(),
        detail::test_data::primes_2x.end());
    EXPECT_FALSE(traits::empty(s));
    EXPECT_EQ(s.size(), traits::size(s));
    EXPECT_EQ(detail::test_data::primes_2x.size(), traits::size(s));
    EXPECT_EQ(detail::test_data::primes_2x.size(), s.size());
    traits::clear(s);
    EXPECT_EQ(s.size(), traits::size(s));
    EXPECT_TRUE(traits::empty(s));
    EXPECT_EQ(0, s.size());
  }

  {
    T s(detail::test_data::primes_2x.begin(),
        detail::test_data::primes_2x.end());
    T const& sconst = s;
    auto k = 17;
    EXPECT_EQ(1, s.count(k));
    EXPECT_EQ(s.find(17), traits::find(s, 17));
    EXPECT_EQ(sconst.find(17), traits::find(sconst, 17));
  }

  {
    T s(detail::test_data::primes_2x.begin(),
        detail::test_data::primes_2x.end());
    EXPECT_FALSE(traits::empty(s));
    auto sizeBefore = traits::size(s);

    auto count = traits::erase(s, 6);
    EXPECT_EQ(traits::size(s), sizeBefore);
    EXPECT_EQ(count, 0);

    count = traits::erase(s, 5);
    EXPECT_EQ(traits::size(s), sizeBefore - 1);
    EXPECT_EQ(count, 1);
  }
}

#undef THRIFT_COMPARE_UNORDERED_CONTAINERS_IMPL_TRAITS
#undef THRIFT_COMPARE_UNORDERED_CONTAINERS_IMPL2_TRAITS
#undef THRIFT_COMPARE_UNORDERED_CONTAINERS_IMPL
#undef THRIFT_COMPARE_UNORDERED_CONTAINERS_IMPL2
#undef THRIFT_COMPARE_CONTAINER_TO_ITERATORS_IMPL
#undef THRIFT_COMPARE_ITERATORS_IMPL

} // namespace thrift
} // namespace apache

#endif // THRIFT_FATAL_TRAITS_TEST_HELPERS_INL_H
