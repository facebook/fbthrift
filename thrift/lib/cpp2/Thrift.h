/*
 * Copyright 2014 Facebook, Inc.
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

#include <initializer_list>
#include <utility>

#include <cstdint>

namespace apache { namespace thrift {

enum FragileConstructor {
  FRAGILE,
};

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

/**
 * Class template (specialized for each type in generated code) that allows
 * access to write / read / serializedSize / serializedSizeZC functions in
 * a generic way.
 *
 * For native Cpp2 structs, one could call the corresponding methods
 * directly, but structs generated in compatibility mode (ie. typedef'ed
 * to the Thrift1 version) don't have them; they are defined as free
 * functions named <type>_read, <type>_write, etc, so they can't be accessed
 * generically (because the type name is part of the function name).
 *
 * Cpp2Ops bridges to either struct methods (for native Cpp2 structs)
 * or the corresponding free functions (for structs in compatibility mode).
 */
template <class T, class = void>
class Cpp2Ops {
 public:
  static void clear(T* );

  template <class P>
  static uint32_t write(P*, const T*);

  template <class P>
  static uint32_t read(P*, T*);

  template <class P>
  static uint32_t serializedSize(P*, const T*);

  template <class P>
  static uint32_t serializedSizeZC(P*, const T*);

  static constexpr apache::thrift::protocol::TType thriftType();
};

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

namespace detail {

template <std::intmax_t Id, typename T>
struct argument_wrapper {
  static_assert(
    std::is_rvalue_reference<T&&>::value,
    "this wrapper handles only rvalues and initializer_list"
  );

  template <typename U>
  explicit argument_wrapper(U&& value):
    argument_(std::move(value))
  {
    static_assert(
      std::is_rvalue_reference<U&&>::value,
      "this wrapper handles only rvalues and initializer_list"
    );
  }

  T&& move() { return std::move(argument_); }

private:
  T argument_;
};

} // detail

template <std::intmax_t Id, typename T>
detail::argument_wrapper<Id, std::initializer_list<T>> wrap_argument(
  std::initializer_list<T> value
) {
  return detail::argument_wrapper<Id, std::initializer_list<T>>(
    std::move(value)
  );
}

template <std::intmax_t Id, typename T>
detail::argument_wrapper<Id, T&&> wrap_argument(T&& value) {
  static_assert(std::is_rvalue_reference<T&&>::value, "internal thrift error");
  return detail::argument_wrapper<Id, T&&>(std::forward<T>(value));
}

}} // apache::thrift

#endif // #ifndef THRIFT_CPP2_H_
