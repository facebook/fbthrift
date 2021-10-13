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

#include <stdexcept>

#include <folly/Likely.h>
#include <folly/Portability.h>
#include <folly/Utility.h>
#include <folly/lang/Exception.h>
#include <folly/synchronization/RelaxedAtomic.h>

/**
 * This provides a simple framework for defining functions in core thrift
 * that may be overridden by other modules that get linked in. Note that only
 * one override can be present for each function.
 *
 * Consider the following example:
 *
 *   // MyCoreThriftLibrary.h
 *   THRIFT_PLUGGABLE_FUNC_DECLARE(int, myPluggableFunction, int a, int b);
 *
 *   // MyCoreThriftLibrary.cpp
 *   THRIFT_PLUGGABLE_FUNC_REGISTER(int, myPluggableFunction, int a, int b) {
 *     return a + b;
 *   }
 *
 *   ...
 *
 *   void foo() {
 *     auto result = myPluggableFunction(1, 2);
 *     ...
 *   }
 *
 *   // MyCustomModule.cpp
 *   THRIFT_PLUGGABLE_FUNC_SET(int, myPluggableFunction, int a, int b) {
 *     return a * b;
 *   }
 *
 * If MyCustomModule.cpp is linked in, result in foo() will be 2, otherwise it
 * will be 3.
 */

namespace apache {
namespace thrift {
namespace detail {

template <typename Ret, typename... Args>
class PluggableFunctionSetter;

template <typename Ret, typename... Args>
class PluggableFunction {
 public:
  using signature = Ret(Args...);

  constexpr explicit PluggableFunction(signature& init) noexcept
      : init_{init} {}

  template <typename... A>
  FOLLY_ERASE auto operator()(A&&... a) const
      -> decltype(FOLLY_DECLVAL(signature&)(static_cast<A&&>(a)...)) {
    auto impl = impl_.load();
    // MSVC 2017 dislikes the terse ternary form
    if (FOLLY_UNLIKELY(!impl)) {
      impl = &choose_slow();
    }
    return impl(static_cast<A&&>(a)...);
  }

 private:
  friend class PluggableFunctionSetter<Ret, Args...>;

  FOLLY_NOINLINE signature& choose_slow() const {
    auto impl = impl_.load();
    while (!impl) {
      if (impl_.compare_exchange_weak(impl, &init_)) {
        return init_;
      }
    }
    return *impl;
  }

  PluggableFunction const& operator=(signature& next) const noexcept {
    if (auto prev = impl_.exchange(&next)) {
      auto msg = prev == &init_
          ? "pluggable function: override after invocation"
          : "pluggable function: override after override";
      folly::terminate_with<std::logic_error>(msg);
    }
    return *this;
  }

  //  impl_ should be first to avoid extra arithmetic in the fast path
  mutable folly::relaxed_atomic<signature*> impl_{nullptr};
  signature& init_;
};

template <typename Ret, typename... Args>
class PluggableFunctionSetter {
 public:
  using signature = Ret(Args...);

  PluggableFunctionSetter(
      PluggableFunction<Ret, Args...> const& plug, signature& next) noexcept {
    plug = next;
  }
};

template <typename>
struct pluggable_function_type_;
template <typename Ret, typename... Args>
struct pluggable_function_type_<Ret (*)(Args...)> {
  using type = PluggableFunction<Ret, Args...>;
};
template <typename F>
using pluggable_function_type_t = typename pluggable_function_type_<F>::type;

template <typename>
struct pluggable_function_setter_type_;
template <typename Ret, typename... Args>
struct pluggable_function_setter_type_<Ret (*)(Args...)> {
  using type = PluggableFunctionSetter<Ret, Args...>;
};
template <typename F>
using pluggable_function_setter_type_t =
    typename pluggable_function_setter_type_<F>::type;

} // namespace detail
} // namespace thrift
} // namespace apache

#define THRIFT_PLUGGABLE_FUNC_DECLARE(_ret, _name, ...)             \
  _ret THRIFT__PLUGGABLE_FUNC_DEFAULT_##_name(__VA_ARGS__);         \
  using THRIFT__PLUGGABLE_FUNC_TYPE_##_name =                       \
      decltype(&THRIFT__PLUGGABLE_FUNC_DEFAULT_##_name);            \
  extern const ::apache::thrift::detail::pluggable_function_type_t< \
      THRIFT__PLUGGABLE_FUNC_TYPE_##_name>                          \
      _name

#define THRIFT_PLUGGABLE_FUNC_REGISTER(_ret, _name, ...)             \
  FOLLY_STORAGE_CONSTEXPR const ::apache::thrift::detail::           \
      pluggable_function_type_t<THRIFT__PLUGGABLE_FUNC_TYPE_##_name> \
          _name{THRIFT__PLUGGABLE_FUNC_DEFAULT_##_name};             \
  _ret THRIFT__PLUGGABLE_FUNC_DEFAULT_##_name(__VA_ARGS__)

#define THRIFT_PLUGGABLE_FUNC_SET(_ret, _name, ...)                  \
  _ret THRIFT__PLUGGABLE_FUNC_IMPL_##_name(__VA_ARGS__);             \
  static ::apache::thrift::detail::pluggable_function_setter_type_t< \
      THRIFT__PLUGGABLE_FUNC_TYPE_##_name>                           \
      THRIFT__PLUGGABLE_FUNC_SETTER_##_name{                         \
          _name, THRIFT__PLUGGABLE_FUNC_IMPL_##_name};               \
  _ret THRIFT__PLUGGABLE_FUNC_IMPL_##_name(__VA_ARGS__)
