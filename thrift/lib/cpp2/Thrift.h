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

#ifndef THRIFT_CPP2_H_
#define THRIFT_CPP2_H_

#include <thrift/lib/cpp/Thrift.h>
#include <thrift/lib/cpp2/TypeClass.h>

#include <initializer_list>
#include <utility>
#include <folly/Traits.h>
#include <folly/Utility.h>
#include <folly/functional/Invoke.h>

#include <cstdint>

namespace apache {
namespace thrift {

enum FragileConstructor {
  FRAGILE,
};

// re-definition of the same enums from
// thrift/compiler/ast/t_exception.h
enum class ExceptionKind {
  UNSPECIFIED = 0,
  TRANSIENT = 1, // The associated RPC may succeed if retried.
  STATEFUL = 2, // Server state must be change for the associated RPC to have
                // any chance of succeeding.
  PERMANENT =
      3, // The associated RPC can never succeed, and should not be retried.
};

enum class ExceptionBlame {
  UNSPECIFIED = 0,
  SERVER = 1, // The error was the fault of the server.
  CLIENT = 2, // The error was the fault of the client's request.
};

enum class ExceptionSafety {
  UNSPECIFIED = 0,
  SAFE = 1, // It is guarneteed the associated RPC failed completely, and no
            // significant server state changed while trying to process the
            // RPC.
};

namespace detail {
namespace st {

//  struct_private_access
//
//  Thrift structures have private members but it may be necessary for the
//  Thrift support library to access those private members.
struct struct_private_access {
  //  These should be alias templates but Clang has a bug where it does not
  //  permit member alias templates of a friend struct to access private
  //  members of the type to which it is a friend. Making these function
  //  templates is a workaround.
  template <typename T>
  static folly::bool_constant<T::__fbthrift_cpp2_gen_json> //
  __fbthrift_cpp2_gen_json();

  template <typename T>
  static folly::bool_constant<T::__fbthrift_cpp2_gen_nimble> //
  __fbthrift_cpp2_gen_nimble();

  template <typename T>
  static folly::bool_constant<T::__fbthrift_cpp2_gen_has_thrift_uri> //
  __fbthrift_cpp2_gen_has_thrift_uri();

  template <typename T>
  static const char* __fbthrift_cpp2_gen_thrift_uri() {
    return T::__fbthrift_cpp2_gen_thrift_uri();
  }

  template <typename T>
  static constexpr ExceptionSafety __fbthrift_cpp2_gen_exception_safety() {
    return T::__fbthrift_cpp2_gen_exception_safety;
  }

  template <typename T>
  static constexpr ExceptionKind __fbthrift_cpp2_gen_exception_kind() {
    return T::__fbthrift_cpp2_gen_exception_kind;
  }

  template <typename T>
  static constexpr ExceptionBlame __fbthrift_cpp2_gen_exception_blame() {
    return T::__fbthrift_cpp2_gen_exception_blame;
  }
};

template <typename T, typename = void>
struct IsThriftClass : std::false_type {};

template <typename T>
struct IsThriftClass<T, folly::void_t<typename T::__fbthrift_cpp2_type>>
    : std::true_type {};

template <typename T, typename = void>
struct IsThriftUnion : std::false_type {};

template <typename T>
struct IsThriftUnion<T, folly::void_t<typename T::__fbthrift_cpp2_type>>
    : folly::bool_constant<T::__fbthrift_cpp2_is_union> {};

} // namespace st
} // namespace detail

template <typename T>
constexpr bool is_thrift_class_v =
    apache::thrift::detail::st::IsThriftClass<T>::value;

template <typename T>
constexpr bool is_thrift_union_v =
    apache::thrift::detail::st::IsThriftUnion<T>::value;

template <typename T>
constexpr bool is_thrift_exception_v = is_thrift_class_v<T>&&
    std::is_base_of<apache::thrift::TException, T>::value;

template <typename T>
constexpr bool is_thrift_struct_v =
    is_thrift_class_v<T> && !is_thrift_union_v<T> && !is_thrift_exception_v<T>;

template <typename T, typename Fallback>
using type_class_of_thrift_class_or_t = //
    folly::conditional_t<
        is_thrift_union_v<T>,
        type_class::variant,
        folly::conditional_t<
            is_thrift_class_v<T>, // struct or exception
            type_class::structure,
            Fallback>>;

template <typename T, typename Fallback>
using type_class_of_thrift_class_enum_or_t = //
    folly::conditional_t<
        std::is_enum<T>::value,
        type_class::enumeration,
        type_class_of_thrift_class_or_t<T, Fallback>>;

template <typename T>
using type_class_of_thrift_class_t = type_class_of_thrift_class_or_t<T, void>;

template <typename T>
using type_class_of_thrift_class_enum_t =
    type_class_of_thrift_class_enum_or_t<T, void>;

namespace detail {

template <typename T>
struct enum_hash {
  size_t operator()(T t) const {
    using underlying_t = typename std::underlying_type<T>::type;
    return std::hash<underlying_t>()(underlying_t(t));
  }
};

} // namespace detail

namespace detail {

// Adapted from Fatal (https://github.com/facebook/fatal/)
// Inlined here to keep the amount of mandatory dependencies at bay
// For more context, see http://ericniebler.com/2013/08/07/
// - Universal References and the Copy Constructor
template <typename, typename...>
struct is_safe_overload {
  using type = std::true_type;
};
template <typename Class, typename T>
struct is_safe_overload<Class, T> {
  using type = std::integral_constant<
      bool,
      !std::is_same<
          Class,
          typename std::remove_cv<
              typename std::remove_reference<T>::type>::type>::value>;
};

} // namespace detail

template <typename Class, typename... Args>
using safe_overload_t = typename std::enable_if<
    apache::thrift::detail::is_safe_overload<Class, Args...>::type::value>::
    type;

namespace detail {
FOLLY_CREATE_MEMBER_INVOKER(clear_fn, __clear);
}
FOLLY_INLINE_VARIABLE constexpr apache::thrift::detail::clear_fn clear;
} // namespace thrift
} // namespace apache

#define FBTHRIFT_CPP_DEFINE_MEMBER_INDIRECTION_FN(...)                       \
  struct __fbthrift_cpp2_indirection_fn {                                    \
    template <typename __fbthrift_t>                                         \
    FOLLY_ERASE constexpr auto operator()(__fbthrift_t&& __fbthrift_v) const \
        noexcept(                                                            \
            noexcept(static_cast<__fbthrift_t&&>(__fbthrift_v).__VA_ARGS__)) \
            -> decltype((                                                    \
                static_cast<__fbthrift_t&&>(__fbthrift_v).__VA_ARGS__)) {    \
      return static_cast<__fbthrift_t&&>(__fbthrift_v).__VA_ARGS__;          \
    }                                                                        \
  }

namespace apache {
namespace thrift {

template <typename T>
using detect_indirection_fn_t = typename T::__fbthrift_cpp2_indirection_fn;

template <typename T>
using indirection_fn_t =
    folly::detected_or_t<folly::identity_fn, detect_indirection_fn_t, T>;

namespace detail {
struct apply_indirection_fn {
 private:
  template <typename T>
  using i = indirection_fn_t<folly::remove_cvref_t<T>>;

 public:
  template <typename T>
  FOLLY_ERASE constexpr auto operator()(T&& t) const
      noexcept(noexcept(i<T>{}(static_cast<T&&>(t))))
          -> decltype(i<T>{}(static_cast<T&&>(t))) {
    return i<T>{}(static_cast<T&&>(t));
  }
};
} // namespace detail

FOLLY_INLINE_VARIABLE constexpr detail::apply_indirection_fn apply_indirection;

class ExceptionMetadataOverrideBase {
 public:
  virtual ~ExceptionMetadataOverrideBase() {}

  ExceptionKind errorKind() const { return errorKind_; }

  ExceptionBlame errorBlame() const { return errorBlame_; }

  ExceptionSafety errorSafety() const { return errorSafety_; }

  virtual const std::type_info* type() const = 0;

 protected:
  ExceptionKind errorKind_{ExceptionKind::UNSPECIFIED};
  ExceptionBlame errorBlame_{ExceptionBlame::UNSPECIFIED};
  ExceptionSafety errorSafety_{ExceptionSafety::UNSPECIFIED};
};

template <typename T>
class ExceptionMetadataOverride : public T,
                                  public ExceptionMetadataOverrideBase {
 public:
  explicit ExceptionMetadataOverride(const T& t) : T(t) {}
  explicit ExceptionMetadataOverride(T&& t) : T(std::move(t)) {}

  const std::type_info* type() const override {
#if FOLLY_HAS_RTTI
    return &typeid(T);
#else
    return nullptr;
#endif
  }

  // ExceptionKind
  ExceptionMetadataOverride& setTransient() {
    errorKind_ = ExceptionKind::TRANSIENT;
    return *this;
  }
  ExceptionMetadataOverride& setPermanent() {
    errorKind_ = ExceptionKind::PERMANENT;
    return *this;
  }
  ExceptionMetadataOverride& setStateful() {
    errorKind_ = ExceptionKind::STATEFUL;
    return *this;
  }

  // ExceptionBlame
  ExceptionMetadataOverride& setClient() {
    errorBlame_ = ExceptionBlame::CLIENT;
    return *this;
  }
  ExceptionMetadataOverride& setServer() {
    errorBlame_ = ExceptionBlame::SERVER;
    return *this;
  }

  // ExceptionSafety
  ExceptionMetadataOverride& setSafe() {
    errorSafety_ = ExceptionSafety::SAFE;
    return *this;
  }
};

template <typename T>
ExceptionMetadataOverride<std::decay_t<T>> overrideExceptionMetadata(T&& ex) {
  return ExceptionMetadataOverride<std::decay_t<T>>(std::forward<T>(ex));
}

namespace detail {

enum LazyDeserializationState : uint8_t { // Bitfield.
  UNTAINTED = 1 << 0,
  DESERIALIZED = 1 << 1,
};

} // namespace detail

} // namespace thrift
} // namespace apache

#endif // #ifndef THRIFT_CPP2_H_
