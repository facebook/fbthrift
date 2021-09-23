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

#pragma once

#include <functional>
#include <utility>

#include <folly/Traits.h>

namespace apache {
namespace thrift {
namespace adapt_detail {

// Identical to std::declval<const T&>
template <typename T>
const T& cr();

template <typename Adapter, typename AdaptedT, typename NativeT>
void fromThrift(AdaptedT& adapted, NativeT&& native) {
  adapted = Adapter::fromThrift(std::forward<NativeT>(native));
}

// The type returned by the adapter for the given thrift type.
template <typename Adapter, typename ThriftT>
using adapted_t = decltype(Adapter::fromThrift(std::declval<ThriftT&&>()));

// Equal op based on the thrift types.
template <typename Adapter, typename AdaptedT>
struct thrift_equal {
  constexpr bool operator()(const AdaptedT& lhs, const AdaptedT& rhs) const {
    return Adapter::toThrift(lhs) == Adapter::toThrift(rhs);
  }
};

// Equal op based on the adapted types, with a fallback on thrift_equal.
template <typename Adapter, typename AdaptedT, typename = void>
struct adapted_equal : thrift_equal<Adapter, AdaptedT> {};
template <typename Adapter, typename AdaptedT>
struct adapted_equal<
    Adapter,
    AdaptedT,
    folly::void_t<decltype(cr<AdaptedT>() == cr<AdaptedT>())>> {
  constexpr bool operator()(const AdaptedT& lhs, const AdaptedT& rhs) const {
    return lhs == rhs;
  }
};

// Equal op based on the adapter, with a fallback on adapted_equal.
template <typename Adapter, typename AdaptedT, typename = void>
struct adapter_equal : adapted_equal<Adapter, AdaptedT> {};
template <typename Adapter, typename AdaptedT>
struct adapter_equal<
    Adapter,
    AdaptedT,
    folly::void_t<decltype(Adapter::equal(cr<AdaptedT>(), cr<AdaptedT>()))>> {
  constexpr bool operator()(const AdaptedT& lhs, const AdaptedT& rhs) const {
    return Adapter::equal(lhs, rhs);
  }
};

// Less op based on the thrift types.
template <typename Adapter, typename AdaptedT>
struct thrift_less {
  constexpr bool operator()(const AdaptedT& lhs, const AdaptedT& rhs) const {
    return Adapter::toThrift(lhs) < Adapter::toThrift(rhs);
  }
};

// Less op based on the adapted types, with a fallback on thrift_less.
template <typename Adapter, typename AdaptedT, typename = void>
struct adapted_less : thrift_less<Adapter, AdaptedT> {};
template <typename Adapter, typename AdaptedT>
struct adapted_less<
    Adapter,
    AdaptedT,
    folly::void_t<decltype(cr<AdaptedT>() < cr<AdaptedT>())>> {
  constexpr bool operator()(const AdaptedT& lhs, const AdaptedT& rhs) const {
    return lhs < rhs;
  }
};

// Less op based on the adapter, with a fallback on adapted_less.
template <typename Adapter, typename AdaptedT, typename = void>
struct adapter_less : adapted_less<Adapter, AdaptedT> {};
template <typename Adapter, typename AdaptedT>
struct adapter_less<
    Adapter,
    AdaptedT,
    folly::void_t<decltype(Adapter::less(cr<AdaptedT>(), cr<AdaptedT>()))>> {
  constexpr bool operator()(const AdaptedT& lhs, const AdaptedT& rhs) const {
    return Adapter::less(lhs, rhs);
  }
};

// Hash based on the thrift type.
template <typename Adapter, typename AdaptedT>
struct thrift_hash {
  constexpr size_t operator()(const AdaptedT& value) const {
    auto&& tvalue = Adapter::toThrift(value);
    return std::hash<folly::remove_cvref_t<decltype(tvalue)>>()(tvalue);
  }
};

// Hash based on the adapted types, with a fallback on thrift_hash.
template <typename Adapter, typename AdaptedT, typename = void>
struct adapted_hash : thrift_hash<Adapter, AdaptedT> {};
template <typename Adapter, typename AdaptedT>
struct adapted_hash<
    Adapter,
    AdaptedT,
    folly::void_t<decltype(std::hash<std::decay_t<AdaptedT>>())>>
    : std::hash<std::decay_t<AdaptedT>> {};

// Hash based on the adapter, with a fallback on adapted_hash.
template <typename Adapter, typename AdaptedT, typename = void>
struct adapter_hash : adapted_hash<Adapter, AdaptedT> {};
template <typename Adapter, typename AdaptedT>
struct adapter_hash<
    Adapter,
    AdaptedT,
    folly::void_t<decltype(Adapter::hash(cr<AdaptedT>()))>> {
  constexpr size_t operator()(const AdaptedT& value) const {
    return Adapter::hash(value);
  }
};

template <typename Adapter, typename AdaptedT>
constexpr bool equal(const AdaptedT& lhs, const AdaptedT& rhs) {
  return adapter_equal<Adapter, AdaptedT>()(lhs, rhs);
}

template <typename Adapter, typename AdaptedT>
constexpr bool not_equal(const AdaptedT& lhs, const AdaptedT& rhs) {
  return !adapter_equal<Adapter, AdaptedT>()(lhs, rhs);
}

template <typename Adapter, typename AdaptedT>
constexpr bool less(const AdaptedT& lhs, const AdaptedT& rhs) {
  return adapter_less<Adapter, AdaptedT>()(lhs, rhs);
}

template <typename Adapter, typename AdaptedT>
constexpr size_t hash(const AdaptedT& value) {
  return adapter_hash<Adapter, AdaptedT>()(value);
}

} // namespace adapt_detail
} // namespace thrift
} // namespace apache
