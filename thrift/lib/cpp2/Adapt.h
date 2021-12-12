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

#include <stddef.h>
#include <functional>
#include <type_traits>
#include <utility>

#include <folly/Traits.h>
#include <thrift/lib/cpp2/Thrift.h>

namespace apache {
namespace thrift {

template <typename Struct, int16_t FieldId>
struct FieldAdapterContext {
  static constexpr int16_t kFieldId = FieldId;
  Struct& object;
};

namespace adapt_detail {

// Identical to std::declval<const T&>.
template <typename T>
const T& cr();

template <typename T>
using is_mutable_ref = folly::Conjunction<
    std::is_reference<T>,
    folly::Negation<std::is_const<std::remove_reference_t<T>>>>;

// Used to detect if Adapter has the fromThriftField function which takes an
// additional FieldAdapterContext argument.
template <typename Adapter, typename ThriftT, typename Struct>
using FromThriftFieldType = decltype(Adapter::fromThriftField(
    std::declval<ThriftT>(), std::declval<FieldAdapterContext<Struct, 0>>()));

// Used to detect if Adapter has the construct function.
template <typename Adapter, typename AdaptedT, typename Context>
using ConstructType = decltype(Adapter::construct(
    std::declval<AdaptedT&>(), std::declval<Context>()));

// Converts a Thrift field value into an adapted type via Adapter.
// This overload passes additional context containing the reference to the
// Thrift object containing the field and the field ID as a second argument
// to Adapter::fromThriftField.
template <
    typename Adapter,
    int16_t FieldId,
    typename ThriftT,
    typename Struct,
    std::enable_if_t<
        folly::is_detected_v<FromThriftFieldType, Adapter, ThriftT, Struct>,
        int> = 0>
constexpr decltype(auto) fromThriftField(ThriftT&& value, Struct& object) {
  return Adapter::fromThriftField(
      std::forward<ThriftT>(value),
      FieldAdapterContext<Struct, FieldId>{object});
}

// Converts a Thrift field value into an adapted type via Adapter.
// This overloads does the conversion via Adapter::fromThrift and is used when
// Adapter::fromThriftField is unavailable.
template <
    typename Adapter,
    int16_t FieldId,
    typename ThriftT,
    typename Struct,
    std::enable_if_t<
        !folly::is_detected_v<FromThriftFieldType, Adapter, ThriftT, Struct>,
        int> = 0>
constexpr decltype(auto) fromThriftField(ThriftT&& value, Struct&) {
  return Adapter::fromThrift(std::forward<ThriftT>(value));
}

// The type returned by the adapter for the given thrift type.
template <typename Adapter, typename ThriftT>
using adapted_t = decltype(Adapter::fromThrift(std::declval<ThriftT&&>()));

// The type returned by the adapter for the given thrift type of a struct field.
template <typename Adapter, int16_t FieldId, typename ThriftT, typename Struct>
using adapted_field_t = decltype(fromThriftField<Adapter, FieldId>(
    std::declval<ThriftT&&>(), std::declval<Struct&>()));

// The type returned by the adapter for the given adapted type.
template <typename Adapter, typename AdaptedT>
using thrift_t = decltype(Adapter::toThrift(std::declval<AdaptedT&>()));

// If the adapter exposes access to the standard thrift value
// from the toThrift method.
template <typename Adapter, typename AdaptedT, typename = void>
using has_inplace_toThrift =
    is_mutable_ref<folly::detected_t<thrift_t, Adapter, AdaptedT>>;

template <typename Adapter, typename AdaptedT, typename ThriftT>
void fromThrift(AdaptedT& adapted, ThriftT&& value) {
  adapted = Adapter::fromThrift(std::forward<ThriftT>(value));
}

// Called during the construction of a Thrift object to perform any additonal
// initialization of an adapted type. This overload passes a context containing
// the reference to the Thrift object containing the field and the field ID as
// a second argument to Adapter::construct.
template <
    typename Adapter,
    int16_t FieldId,
    typename AdaptedT,
    typename Struct,
    std::enable_if_t<
        folly::is_detected_v<
            ConstructType,
            Adapter,
            AdaptedT,
            FieldAdapterContext<Struct, FieldId>>,
        int> = 0>
constexpr void construct(AdaptedT& field, Struct& object) {
  Adapter::construct(field, FieldAdapterContext<Struct, FieldId>{object});
}
template <
    typename Adapter,
    int16_t FieldId,
    typename AdaptedT,
    typename Struct,
    std::enable_if_t<
        !folly::is_detected_v<
            ConstructType,
            Adapter,
            AdaptedT,
            FieldAdapterContext<Struct, FieldId>>,
        int> = 0>
constexpr void construct(AdaptedT&, Struct&) {}

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

// Helper for optional fields.
template <typename Adapter, typename FieldRefT>
constexpr bool equal_opt(const FieldRefT& lhs, const FieldRefT& rhs) {
  using AdaptedT = decltype(lhs.value());
  return lhs.has_value() == rhs.has_value() &&
      (!lhs.has_value() || equal<Adapter, AdaptedT>(lhs.value(), rhs.value()));
}

template <typename Adapter, typename AdaptedT>
constexpr bool not_equal(const AdaptedT& lhs, const AdaptedT& rhs) {
  return !adapter_equal<Adapter, AdaptedT>()(lhs, rhs);
}

// Helper for optional fields.
template <typename Adapter, typename FieldRefT>
constexpr bool not_equal_opt(const FieldRefT& lhs, const FieldRefT& rhs) {
  return !equal_opt<Adapter, FieldRefT>(lhs, rhs);
}

template <typename Adapter, typename AdaptedT>
constexpr bool less(const AdaptedT& lhs, const AdaptedT& rhs) {
  return adapter_less<Adapter, AdaptedT>()(lhs, rhs);
}

// A less comparision when the values are already known to be not equal.
// Helper for optional fields.
template <typename Adapter, typename FieldRefT>
constexpr bool neq_less_opt(const FieldRefT& lhs, const FieldRefT& rhs) {
  using AdaptedT = decltype(lhs.value());
  return !lhs.has_value() ||
      (rhs.has_value() &&
       adapter_less<Adapter, AdaptedT>()(lhs.value(), rhs.value()));
}

template <typename Adapter, typename AdaptedT>
constexpr size_t hash(const AdaptedT& value) {
  return adapter_hash<Adapter, AdaptedT>()(value);
}

// Validates an adapter.
// Checking decltype(equal<Adapter>(...)) is not sufficient for validation.
template <typename Adapter, typename AdaptedT>
void validate() {
  const auto adapted = AdaptedT();
  equal<Adapter>(adapted, adapted);
  not_equal<Adapter>(adapted, adapted);
  // less and hash are not validated because not all adapters provide it.
}

template <typename Adapter, typename ThriftT>
void validateAdapter() {
  validate<Adapter, adapted_t<Adapter, ThriftT>>();
}

template <typename Adapter, int16_t FieldID, typename ThriftT, typename Struct>
void validateFieldAdapter() {
  validate<Adapter, adapted_field_t<Adapter, FieldID, ThriftT, Struct>>();
}

} // namespace adapt_detail

template <typename AdaptedT>
struct IndirectionAdapter {
  template <typename ThriftT>
  static constexpr AdaptedT fromThrift(ThriftT&& value) {
    AdaptedT adapted;
    toThrift(adapted) = std::forward<ThriftT>(value);
    return adapted;
  }
  FOLLY_ERASE static constexpr decltype(auto)
  toThrift(AdaptedT& adapted) noexcept(
      noexcept(::apache::thrift::apply_indirection(adapted))) {
    return ::apache::thrift::apply_indirection(adapted);
  }
  FOLLY_ERASE static constexpr decltype(auto)
  toThrift(const AdaptedT& adapted) noexcept(
      noexcept(::apache::thrift::apply_indirection(adapted))) {
    return ::apache::thrift::apply_indirection(adapted);
  }
};

template <typename AdaptedT, typename ThriftT>
struct StaticCastAdapter {
  template <typename T>
  static constexpr decltype(auto) fromThrift(T&& value) {
    return static_cast<AdaptedT>(std::forward<T>(value));
  }
  template <typename T>
  static constexpr decltype(auto) toThrift(T&& value) {
    return static_cast<ThriftT>(std::forward<T>(value));
  }
};

} // namespace thrift
} // namespace apache
