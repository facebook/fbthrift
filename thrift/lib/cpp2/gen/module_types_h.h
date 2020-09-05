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

#include <cstdint>
#include <initializer_list>
#include <memory>
#include <type_traits>

#include <thrift/lib/cpp2/OptionalField.h>
#include <thrift/lib/cpp2/Thrift.h>
#include <thrift/lib/cpp2/TypeClass.h>
#include <thrift/lib/cpp2/protocol/Cpp2Ops.h>
#include <thrift/lib/cpp2/protocol/Protocol.h>

#include <folly/CPortability.h>
#include <folly/Traits.h>

//  all members are logically private to fbthrift; external use is deprecated
//
//  access_field would use decltype((static_cast<T&&>(t).name)) with the extra
//  parens, except that clang++ -fms-extensions fails to parse it correctly
#define APACHE_THRIFT_DEFINE_ACCESSOR(name)                                   \
  template <>                                                                 \
  struct access_field<::apache::thrift::tag::name> {                          \
    template <typename T>                                                     \
    FOLLY_ERASE constexpr auto operator()(T&& t) noexcept                     \
        -> ::folly::like_t<T&&, decltype(::folly::remove_cvref_t<T>::name)> { \
      return static_cast<T&&>(t).name;                                        \
    }                                                                         \
  };                                                                          \
  template <>                                                                 \
  struct invoke_reffer<::apache::thrift::tag::name> {                         \
    template <typename T, typename... A>                                      \
    FOLLY_ERASE constexpr auto operator()(T&& t, A&&... a) noexcept(          \
        noexcept(static_cast<T&&>(t).name##_ref(static_cast<A&&>(a)...)))     \
        -> decltype(static_cast<T&&>(t).name##_ref(static_cast<A&&>(a)...)) { \
      return static_cast<T&&>(t).name##_ref(static_cast<A&&>(a)...);          \
    }                                                                         \
  };                                                                          \
  template <>                                                                 \
  struct invoke_setter<::apache::thrift::tag::name> {                         \
    template <typename T, typename... A>                                      \
    FOLLY_ERASE constexpr auto operator()(T&& t, A&&... a) noexcept(          \
        noexcept(static_cast<T&&>(t).set_##name(static_cast<A&&>(a)...)))     \
        -> decltype(static_cast<T&&>(t).set_##name(static_cast<A&&>(a)...)) { \
      return static_cast<T&&>(t).set_##name(static_cast<A&&>(a)...);          \
    }                                                                         \
  }

// Temporarily disable warnings about internal deprecated use of __isset.
#define THRIFT_IGNORE_ISSET_USE_WARNING_BEGIN \
  FOLLY_PUSH_WARNING                          \
  FOLLY_GNU_DISABLE_WARNING("-Wdeprecated-declarations")
#define THRIFT_IGNORE_ISSET_USE_WARNING_END FOLLY_POP_WARNING

namespace apache {
namespace thrift {

namespace detail {

template <typename Tag>
struct access_field;
template <typename Tag>
struct invoke_reffer;
template <typename Tag>
struct invoke_setter;

template <typename Tag>
struct invoke_reffer_thru {
  template <typename... A>
  FOLLY_ERASE constexpr auto operator()(A&&... a) noexcept(
      noexcept(invoke_reffer<Tag>{}(static_cast<A&&>(a)...).value_unchecked()))
      -> decltype(
          invoke_reffer<Tag>{}(static_cast<A&&>(a)...).value_unchecked()) {
    return invoke_reffer<Tag>{}(static_cast<A&&>(a)...).value_unchecked();
  }

  template <typename... A>
  FOLLY_ERASE constexpr auto operator()(A&&... a) noexcept(
      noexcept(*invoke_reffer<Tag>{}(static_cast<A&&>(a)...)))
      -> decltype(
          invoke_reffer<Tag>{}(static_cast<A&&>(a)...).is_set(),
          *invoke_reffer<Tag>{}(static_cast<A&&>(a)...)) {
    return *invoke_reffer<Tag>{}(static_cast<A&&>(a)...);
  }
};

template <typename Tag>
struct invoke_reffer_thru_or_access_field {
  template <typename... A>
  using impl = folly::conditional_t<
      folly::is_invocable_v<invoke_reffer_thru<Tag>, A...>,
      invoke_reffer_thru<Tag>,
      access_field<Tag>>;
  template <typename... A>
  FOLLY_ERASE constexpr auto operator()(A&&... a) noexcept(
      noexcept(impl<A...>{}(static_cast<A&&>(a)...)))
      -> decltype(impl<A...>{}(static_cast<A&&>(a)...)) {
    return impl<A...>{}(static_cast<A&&>(a)...);
  }
};

template <typename A, typename Ref>
struct wrapped_struct_argument {
  static_assert(std::is_reference<Ref>::value, "not a reference");
  Ref ref;
  FOLLY_ERASE explicit wrapped_struct_argument(Ref ref_)
      : ref(static_cast<Ref>(ref_)) {}
};

template <typename A, typename T>
struct wrapped_struct_argument<A, std::initializer_list<T>> {
  std::initializer_list<T> ref;
  FOLLY_ERASE explicit wrapped_struct_argument(std::initializer_list<T> list)
      : ref(list) {}
};

template <typename A, typename T>
FOLLY_ERASE wrapped_struct_argument<A, std::initializer_list<T>>
wrap_struct_argument(std::initializer_list<T> value) {
  return wrapped_struct_argument<A, std::initializer_list<T>>(value);
}

template <typename A, typename T>
FOLLY_ERASE wrapped_struct_argument<A, T&&> wrap_struct_argument(T&& value) {
  return wrapped_struct_argument<A, T&&>(static_cast<T&&>(value));
}

template <typename A, bool>
struct assign_isset_;
template <typename A>
struct assign_isset_<A, false> {
  template <typename S>
  FOLLY_ERASE constexpr bool operator()(S&, bool b) const noexcept {
    return b;
  }
};

THRIFT_IGNORE_ISSET_USE_WARNING_BEGIN
template <typename A>
struct assign_isset_<A, true> {
  template <typename S>
  FOLLY_ERASE constexpr auto operator()(S& s, bool b) const
      noexcept(noexcept(access_field<A>{}(s.__isset) = b))
          -> decltype(access_field<A>{}(s.__isset) = b) {
    return access_field<A>{}(s.__isset) = b;
  }
};
THRIFT_IGNORE_ISSET_USE_WARNING_END

template <typename A, typename S>
using assign_isset =
    assign_isset_<A, folly::is_invocable_v<assign_isset_<A, true>, S&, bool>>;

template <typename F, typename T>
FOLLY_ERASE void assign_struct_field(F& f, T&& t) {
  f = static_cast<T&&>(t);
}
template <typename F, typename T>
FOLLY_ERASE void assign_struct_field(std::unique_ptr<F>& f, T&& t) {
  f = std::make_unique<folly::remove_cvref_t<T>>(static_cast<T&&>(t));
}
template <typename F, typename T>
FOLLY_ERASE void assign_struct_field(std::shared_ptr<F>& f, T&& t) {
  f = std::make_shared<folly::remove_cvref_t<T>>(static_cast<T&&>(t));
}

template <typename S, typename... A, typename... T>
FOLLY_ERASE constexpr S make_constant(
    type_class::structure,
    wrapped_struct_argument<A, T>... arg) {
  using _ = int[];
  S s;
  void(_{0, (void(assign_isset<A, S>{}(s, true)), 0)...});
  void(_{0,
         (void(assign_struct_field(
              invoke_reffer_thru_or_access_field<A>{}(s),
              static_cast<T>(arg.ref))),
          0)...});
  return s;
}

template <typename S, typename... A, typename... T>
FOLLY_ERASE constexpr S make_constant(
    type_class::variant,
    wrapped_struct_argument<A, T>... arg) {
  using _ = int[];
  S s;
  void(_{0, (void(invoke_setter<A>{}(s, static_cast<T>(arg.ref))), 0)...});
  return s;
}

} // namespace detail
} // namespace thrift
} // namespace apache
