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

#include <cmath>

#include <thrift/lib/cpp2/op/Compare.h>
#include <thrift/lib/cpp2/type/ThriftType.h>
#include <thrift/lib/cpp2/type/Traits.h>

namespace apache::thrift::op::detail {
template <typename Tag>
struct Clear; // Forward declare.

template <typename Tag, typename T = type::native_type<Tag>>
struct DefaultOf {
  // C++'s intrinsic default for the underlying native type, is the intrisitic
  // default for for all unstructured types.
  static_assert(!type::structured_types::contains<Tag>());
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
  template <typename T = type::native_type<Tag>>
  constexpr bool operator()(const T& value) const {
    // All unstructured values are 'empty' if they are identical to their
    // intrinsic default.
    //
    // TODO(afuller): Implement a specialization for structured types that
    // can serialize to an empty buffer.
    // static_assert(!type::structured_types::contains<Tag>());
    return op::identical<Tag>(value, DefaultOf<Tag, T>::get());
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

struct ContainerEmpty {
  template <typename T>
  constexpr bool operator()(const T& value) const {
    return value.empty();
  }
};
template <typename ValTag, template <typename...> typename ListT>
struct Empty<type::list<ValTag, ListT>> : ContainerEmpty {};
template <typename KeyTag, template <typename...> typename SetT>
struct Empty<type::set<KeyTag, SetT>> : ContainerEmpty {};
template <
    typename KeyTag,
    typename ValTag,
    template <typename...>
    typename MapT>
struct Empty<type::map<KeyTag, ValTag, MapT>> : ContainerEmpty {};

template <typename Tag>
struct Clear {
  template <typename T = type::native_type<Tag>>
  constexpr void operator()(T& value) const {
    // All unstructured types can be cleared by assigning to the intrinsic
    // default.
    static_assert(!type::structured_types::contains<Tag>());
    value = DefaultOf<Tag, T>::get();
  }
};

struct StructuredClear {
  template <typename T>
  constexpr void operator()(T& value) const {
    value.__clear();
  }
};
template <typename T>
struct Clear<type::struct_t<T>> : StructuredClear {};
template <typename T>
struct Clear<type::union_t<T>> : StructuredClear {};
template <typename T>
struct Clear<type::exception_t<T>> : StructuredClear {};

struct ContainerClear {
  template <typename T>
  constexpr void operator()(T& value) const {
    value.clear();
  }
};

template <typename ValTag, template <typename...> typename ListT>
struct Clear<type::list<ValTag, ListT>> : ContainerClear {};
template <typename KeyTag, template <typename...> typename SetT>
struct Clear<type::set<KeyTag, SetT>> : ContainerClear {};
template <
    typename KeyTag,
    typename ValTag,
    template <typename...>
    typename MapT>
struct Clear<type::map<KeyTag, ValTag, MapT>> : ContainerClear {};

} // namespace apache::thrift::op::detail
