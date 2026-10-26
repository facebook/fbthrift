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

#pragma once

#include <deque>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <memory_resource>
#include <set>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <folly/Traits.h>

namespace apache::thrift::pmr {

/**
 * Maps a C++ type to one that allocates all of its memory, including that of
 * its elements, through std::pmr::polymorphic_allocator. @cpp.Arena gives a
 * string, binary or container field the type `Rebind<T>::type`, where T is
 * the field's type without @cpp.Arena.
 *
 * - std::basic_string, std::vector, std::deque, std::list and the std ordered
 *   and unordered sets and maps map to their std::pmr counterparts, with their
 *   element types rebound.
 *   std::less, std::greater, std::hash and std::equal_to of the key type
 *   become those of the rebound key type; other comparators and hashers are
 *   kept, so they must accept the rebound key type.
 * - Any other specialization of a class template whose parameters are all
 *   types fails to build. Specialize Rebind to use such a type with
 *   @cpp.Arena: map it to a type that uses std::pmr::polymorphic_allocator and
 *   rebinds its element types through Rebind, so that nested values use the
 *   same memory resource, or to itself if it never allocates memory.
 * - Every other type maps to itself. This template does not inspect T, so T
 *   may be incomplete, e.g. a struct holding a container of itself.
 *
 * A user specialization must be declared before any use of Rebind for its
 * type, in a header that the .thrift file includes with cpp_include.
 * Otherwise a non-template class silently maps to itself in the translation
 * units that don't see the specialization.
 */
template <class T>
struct Rebind {
  using type = T;
};

namespace detail {

template <class T>
using rebind_t = typename Rebind<T>::type;

// Base of the specializations Thrift ships. side_effect_free_dtor says that
// destroying the container or string itself, apart from its elements, only
// frees memory through its allocator. Thrift trusts its own specializations,
// so they bypass the check that user specializations setting
// side_effect_free_dtor are subject to.
struct ShippedRebind {
  static constexpr bool side_effect_free_dtor = true;
  static constexpr bool bypass_constexpr_dtor_check_internal_do_not_use = true;
};

// The comparator, hasher or key-equality functor F of a container with key
// type K, for the rebound key type.
template <class F, class K>
struct RebindKeyFunctor {
  using type = F;
};
template <class K>
struct RebindKeyFunctor<std::less<K>, K> {
  using type = std::less<rebind_t<K>>;
};
template <class K>
struct RebindKeyFunctor<std::greater<K>, K> {
  using type = std::greater<rebind_t<K>>;
};
template <class K>
struct RebindKeyFunctor<std::hash<K>, K> {
  using type = std::hash<rebind_t<K>>;
};
template <class K>
struct RebindKeyFunctor<std::equal_to<K>, K> {
  using type = std::equal_to<rebind_t<K>>;
};

template <class F, class K>
using rebind_key_functor_t = typename RebindKeyFunctor<F, K>::type;

} // namespace detail

template <template <class...> class C, class... Ts>
struct Rebind<C<Ts...>> {
  static_assert(
      folly::always_false<C<Ts...>>,
      "apache::thrift::pmr::Rebind has no specialization for this class "
      "template. Specialize apache::thrift::pmr::Rebind for it, mapping it to "
      "a type that uses std::pmr::polymorphic_allocator and whose element "
      "types are rebound through Rebind, or to itself if it never allocates "
      "memory.");
  // Only so that uses of `type` add no errors after the static_assert.
  using type = C<Ts...>;
};

template <class C, class Traits, class A>
struct Rebind<std::basic_string<C, Traits, A>> : detail::ShippedRebind {
  using type = std::basic_string<C, Traits, std::pmr::polymorphic_allocator<C>>;
};

template <class T, class A>
struct Rebind<std::vector<T, A>> : detail::ShippedRebind {
  using type = std::pmr::vector<detail::rebind_t<T>>;
};

template <class T, class A>
struct Rebind<std::deque<T, A>> : detail::ShippedRebind {
  using type = std::pmr::deque<detail::rebind_t<T>>;
};

template <class T, class A>
struct Rebind<std::list<T, A>> : detail::ShippedRebind {
  using type = std::pmr::list<detail::rebind_t<T>>;
};

template <class K, class C, class A>
struct Rebind<std::set<K, C, A>> : detail::ShippedRebind {
  using type =
      std::pmr::set<detail::rebind_t<K>, detail::rebind_key_functor_t<C, K>>;
};

template <class K, class C, class A>
struct Rebind<std::multiset<K, C, A>> : detail::ShippedRebind {
  using type = std::pmr::
      multiset<detail::rebind_t<K>, detail::rebind_key_functor_t<C, K>>;
};

template <class K, class V, class C, class A>
struct Rebind<std::map<K, V, C, A>> : detail::ShippedRebind {
  using type = std::pmr::map<
      detail::rebind_t<K>,
      detail::rebind_t<V>,
      detail::rebind_key_functor_t<C, K>>;
};

template <class K, class V, class C, class A>
struct Rebind<std::multimap<K, V, C, A>> : detail::ShippedRebind {
  using type = std::pmr::multimap<
      detail::rebind_t<K>,
      detail::rebind_t<V>,
      detail::rebind_key_functor_t<C, K>>;
};

template <class K, class H, class E, class A>
struct Rebind<std::unordered_set<K, H, E, A>> : detail::ShippedRebind {
  using type = std::pmr::unordered_set<
      detail::rebind_t<K>,
      detail::rebind_key_functor_t<H, K>,
      detail::rebind_key_functor_t<E, K>>;
};

template <class K, class H, class E, class A>
struct Rebind<std::unordered_multiset<K, H, E, A>> : detail::ShippedRebind {
  using type = std::pmr::unordered_multiset<
      detail::rebind_t<K>,
      detail::rebind_key_functor_t<H, K>,
      detail::rebind_key_functor_t<E, K>>;
};

template <class K, class V, class H, class E, class A>
struct Rebind<std::unordered_map<K, V, H, E, A>> : detail::ShippedRebind {
  using type = std::pmr::unordered_map<
      detail::rebind_t<K>,
      detail::rebind_t<V>,
      detail::rebind_key_functor_t<H, K>,
      detail::rebind_key_functor_t<E, K>>;
};

template <class K, class V, class H, class E, class A>
struct Rebind<std::unordered_multimap<K, V, H, E, A>> : detail::ShippedRebind {
  using type = std::pmr::unordered_multimap<
      detail::rebind_t<K>,
      detail::rebind_t<V>,
      detail::rebind_key_functor_t<H, K>,
      detail::rebind_key_functor_t<E, K>>;
};

} // namespace apache::thrift::pmr
