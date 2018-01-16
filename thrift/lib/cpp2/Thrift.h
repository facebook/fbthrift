/*
 * Copyright 2011-present Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef THRIFT_CPP2_H_
#define THRIFT_CPP2_H_

#include <thrift/lib/cpp/Thrift.h>
#include <thrift/lib/cpp/protocol/TType.h>
#include <thrift/lib/cpp2/TypeClass.h>
#include <thrift/lib/cpp2/protocol/Cpp2Ops.h>

#include <initializer_list>
#include <utility>

#include <cstdint>

namespace apache { namespace thrift {

enum FragileConstructor {
  FRAGILE,
};

namespace detail { namespace st {

/**
 *  Like boost::totally_ordered, but does not cause boost functions always to
 *  be included in overload resolution sets due to ADL.
 */
template <typename T>
struct ComparisonOperators {
  friend bool operator !=(const T& x, const T& y) { return !(x == y); }
  friend bool operator > (const T& x, const T& y) { return y < x; }
  friend bool operator <=(const T& x, const T& y) { return !(y < x); }
  friend bool operator >=(const T& x, const T& y) { return !(x < y); }
};

}}

namespace detail {

template <typename T>
struct enum_hash {
  size_t operator()(T t) const {
    using underlying_t = typename std::underlying_type<T>::type;
    return std::hash<underlying_t>()(underlying_t(t));
  }
};

template <typename T>
struct enum_equal_to {
  bool operator()(T t0, T t1) const { return t0 == t1; }
};

}

namespace detail {

// Adapted from Fatal (https://github.com/facebook/fatal/)
// Inlined here to keep the amount of mandatory dependencies at bay
// For more context, see http://ericniebler.com/2013/08/07/
// - Universal References and the Copy Constructor
template <typename, typename...>
struct is_safe_overload { using type = std::true_type; };
template <typename Class, typename T>
struct is_safe_overload<Class, T> {
  using type = std::integral_constant<
    bool,
    !std::is_same<
      Class,
      typename std::remove_cv<typename std::remove_reference<T>::type>::type
    >::value
  >;
};

} // detail

template <typename Class, typename... Args>
using safe_overload_t = typename std::enable_if<
  detail::is_safe_overload<Class, Args...>::type::value
>::type;

// HACK: Disable the default merge() for cpp2-generated types.
//
// The default implementation is wrong - it just copies/moves the source
// wholesale - and blindly upgrading from legacy cpp to cpp2 would change the
// behavior of merge over generated types to the default implementation without
// any warning.
//
// If there is code that needs to mimick the behavior of merge(), there is
// another implementation called merge_into that may be used. It requires static
// reflection metatypes. For more details, see: thrift/lib/cpp2/fatal/merge.h.
//
// If there is code that needs merge() available via ADL, an overload of merge()
// should be made in the namespace of the argument types, and its implementation
// may simply forward to merge_into.
template <typename T>
struct MergeTrait<T, typename std::enable_if<
      std::is_base_of<detail::st::ComparisonOperators<T>, T>::value>::type>;

}} // apache::thrift

#endif // #ifndef THRIFT_CPP2_H_
