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

#include <folly/CPortability.h>
#include <thrift/lib/cpp2/protocol/Protocol.h>
#include <thrift/lib/cpp2/type/ThriftType.h>
#include <thrift/lib/cpp2/type/Traits.h>

namespace apache::thrift::op::detail {
template <typename Tag>
struct Clear; // Forward declare.

template <typename Tag>
struct EqualTo {
  template <typename T = type::standard_type<Tag>>
  constexpr bool operator()(const T& lhs, const T& rhs) const {
    // All standard types implement this via the native c++ operator.
    static_assert(type::is_standard_type<Tag, T>::value);
    return lhs == rhs;
  }
};

struct StringEqualTo {
  template <typename T>
  bool operator()(const T& lhs, const T& rhs) const {
    return StringTraits<T>::isEqual(lhs, rhs);
  }
};
template <>
struct EqualTo<type::string_t> : StringEqualTo {};
template <>
struct EqualTo<type::binary_t> : StringEqualTo {};

template <typename Tag>
struct IdenticalTo : EqualTo<Tag> {
  // Identical is the same as equal for integral and string types.
  static_assert(
      type::integral_types::contains<Tag> ||
      type::string_types::contains<Tag> ||
      // TODO(afuller): Implement proper specializations for all container
      // types.
      type::container_types::contains<Tag> ||
      // TODO(afuller): Implement proper specializations for all structured
      // types.
      type::structured_types::contains<Tag>);
};

template <typename F, typename I>
struct FloatIdenticalTo {
  bool operator()(F lhs, F rhs) const {
    // NOTE: Thrift specifies that all NaN variations are considered
    // 'identical'; however, we do not implement that here for performance
    // reasons.
    return folly::bit_cast<I>(lhs) == folly::bit_cast<I>(rhs);
  }
};
template <>
struct IdenticalTo<type::float_t> : FloatIdenticalTo<float, int32_t> {};
template <>
struct IdenticalTo<type::double_t> : FloatIdenticalTo<double, int64_t> {};

template <typename ValTag>
struct IdenticalTo<type::list<ValTag>> {
  template <typename T = type::standard_type<type::list<ValTag>>>
  bool operator()(const T& lhs, const T& rhs) const {
    if (&lhs == &rhs) {
      return true;
    }
    if (lhs.size() != rhs.size()) {
      return false;
    }
    return std::equal(
        lhs.begin(), lhs.end(), rhs.begin(), IdenticalTo<ValTag>());
  }
};

template <typename Tag, typename T = type::standard_type<Tag>>
struct DefaultOf {
  static_assert(type::is_standard_type<Tag, T>::value);
  // C++'s intrinsic default for the underlying native type, is the intrisitic
  // default for for all unstructured types.
  static_assert(!type::structured_types::contains<Tag>);
  constexpr static T get() { return {}; }
};

template <typename T>
struct StringDefaultOf {
  constexpr static T get() { return StringTraits<T>::fromStringLiteral(""); }
};
template <typename T>
struct DefaultOf<type::string_t, T> : StringDefaultOf<T> {};
template <typename T>
struct DefaultOf<type::binary_t, T> : StringDefaultOf<T> {};

template <typename Tag, typename T>
struct StructureDefaultOf {
  static_assert(type::is_standard_type<Tag, T>::value);
  FOLLY_EXPORT static const T& get() {
    const static T* kDefault = []() {
      auto* value = new T{};
      // The default construct respects 'custom' defaults on fields, but
      // clearing any instance of a structured type, sets it to the
      // 'intrinsic' default.
      Clear<Tag>()(*value);
      return value;
    }();
    return *kDefault;
  }
};
template <typename T>
struct DefaultOf<type::struct_t<T>> : StructureDefaultOf<type::struct_t<T>, T> {
};
template <typename T>
struct DefaultOf<type::union_t<T>> : StructureDefaultOf<type::union_t<T>, T> {};
template <typename T>
struct DefaultOf<type::exception_t<T>>
    : StructureDefaultOf<type::exception_t<T>, T> {};

template <typename Tag>
struct Empty {
  template <typename T = type::standard_type<Tag>>
  constexpr bool operator()(const T& value) const {
    static_assert(type::is_standard_type<Tag, T>::value);
    // All unstructured values are 'empty' if they are equal to their intrinsic
    // default.
    //
    // TODO(afuller): Implement a specialization for structured types that
    // can serialize to an empty buffer.
    // static_assert(!type::structured_types::contains<Tag>);
    return EqualTo<Tag>()(value, DefaultOf<Tag, T>::get());
  }
};

struct StringEmpty {
  template <typename T>
  bool operator()(const T& value) const {
    return StringTraits<T>::isEmpty(value);
  }
};
template <>
struct Empty<type::string_t> : StringEmpty {};
template <>
struct Empty<type::binary_t> : StringEmpty {};

template <typename Tag>
struct ContainerEmpty {
  template <typename T = type::standard_type<Tag>>
  constexpr bool operator()(const T& value) const {
    return value.empty();
  }
};
template <typename ValTag>
struct Empty<type::list<ValTag>> : ContainerEmpty<type::list<ValTag>> {};
template <typename ValTag>
struct Empty<type::set<ValTag>> : ContainerEmpty<type::set<ValTag>> {};
template <typename KeyTag, typename ValTag>
struct Empty<type::map<KeyTag, ValTag>>
    : ContainerEmpty<type::map<KeyTag, ValTag>> {};

template <typename Tag>
struct Clear {
  template <typename T = type::standard_type<Tag>>
  constexpr void operator()(T& value) const {
    // All unstructured types can be cleared by assigning to the intrinsic
    // default.
    //
    // TODO(afuller): Implement specializations for structured types.
    static_assert(!type::structured_types::contains<Tag>);
    value = DefaultOf<Tag, T>::get();
  }
};

template <typename Tag>
struct ContainerClear {
  template <typename T = type::standard_type<Tag>>
  constexpr void operator()(T& value) const {
    value.clear();
  }
};
template <typename ValTag>
struct Clear<type::list<ValTag>> : ContainerClear<type::list<ValTag>> {};
template <typename ValTag>
struct Clear<type::set<ValTag>> : ContainerClear<type::set<ValTag>> {};
template <typename KeyTag, typename ValTag>
struct Clear<type::map<KeyTag, ValTag>>
    : ContainerClear<type::map<KeyTag, ValTag>> {};

} // namespace apache::thrift::op::detail
