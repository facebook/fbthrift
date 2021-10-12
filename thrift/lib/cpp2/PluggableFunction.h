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

#include <atomic>
#include <typeindex>

#include <folly/Range.h>

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
struct PluggableFunctionTag {};

class PluggableFunctionMetadata;

PluggableFunctionMetadata* registerPluggableFunction(
    folly::StringPiece name,
    std::type_index tag,
    std::type_index functionTag,
    intptr_t defaultImpl);

void setPluggableFunction(
    folly::StringPiece name,
    std::type_index tag,
    std::type_index functionTag,
    intptr_t defaultImpl,
    intptr_t impl);

intptr_t getPluggableFunction(PluggableFunctionMetadata* metadata);

template <typename Ret, typename... Args>
struct PluggableFunction {
  using Func = Ret (*)(Args...);

  PluggableFunction(
      folly::StringPiece name, std::type_index tag, Func defaultImpl)
      : metadata_{registerPluggableFunction(
            name,
            tag,
            typeid(PluggableFunctionTag<Ret, Args...>),
            reinterpret_cast<intptr_t>(defaultImpl))} {}

  Ret operator()(Args... args) {
    auto impl = impl_.load(std::memory_order_acquire);
    if (!impl) {
      impl = reinterpret_cast<Func>(getPluggableFunction(metadata_));
      impl_.store(impl, std::memory_order_release);
    }
    return impl(std::forward<Args>(args)...);
  }

 private:
  PluggableFunctionMetadata* metadata_;
  std::atomic<Func> impl_{};
};

template <typename Ret, typename... Args>
struct SetterPluggableFunction {
  using Func = Ret (*)(Args...);

  SetterPluggableFunction(
      folly::StringPiece name,
      std::type_index tag,
      Func defaultImpl,
      Func impl) {
    setPluggableFunction(
        name,
        tag,
        typeid(PluggableFunctionTag<Ret, Args...>),
        reinterpret_cast<intptr_t>(defaultImpl),
        reinterpret_cast<intptr_t>(impl));
  }
};

template <typename Tag, typename Ret, typename... Args>
auto registerPluggableFunction(
    folly::StringPiece name, Tag*, Ret (*defaultImpl)(Args...)) {
  return apache::thrift::detail::PluggableFunction(
      name, typeid(Tag*), defaultImpl);
}

template <typename Tag, typename Ret, typename... Args>
auto setPluggableFunction(
    folly::StringPiece name,
    Tag*,
    Ret (*defaultImpl)(Args...),
    Ret (*impl)(Args...)) {
  return apache::thrift::detail::SetterPluggableFunction(
      name, typeid(Tag*), defaultImpl, impl);
}

template <typename Ret, typename... Args>
PluggableFunction<Ret, Args...> getPluggableFunctionType(Ret (*)(Args...));

template <typename>
struct pluggable_function_type_;
template <typename Ret, typename... Args>
struct pluggable_function_type_<Ret (*)(Args...)> {
  using type = PluggableFunction<Ret, Args...>;
};
template <typename F>
using pluggable_function_type_t = typename pluggable_function_type_<F>::type;

} // namespace detail
} // namespace thrift
} // namespace apache

#define THRIFT_PLUGGABLE_FUNC_DECLARE(_ret, _name, ...)                       \
  struct THRIFT__PLUGGABLE_FUNC_TAG_##_name;                                  \
  _ret THRIFT__PLUGGABLE_FUNC_DEFAULT_##_name(__VA_ARGS__);                   \
  extern ::apache::thrift::detail::pluggable_function_type_t<                 \
      decltype(&THRIFT__PLUGGABLE_FUNC_DEFAULT_##_name)>                      \
      THRIFT__PLUGGABLE_FUNC_##_name;                                         \
  struct THRIFT__PLUGGABLE_FUNC_OBJECT_##_name {                              \
    template <typename... A>                                                  \
    auto operator()(A&&... a) const                                           \
        -> decltype(THRIFT__PLUGGABLE_FUNC_##_name(static_cast<A&&>(a)...)) { \
      return THRIFT__PLUGGABLE_FUNC_##_name(static_cast<A&&>(a)...);          \
    }                                                                         \
  };                                                                          \
  FOLLY_INLINE_VARIABLE constexpr THRIFT__PLUGGABLE_FUNC_OBJECT_##_name _name {}

#define THRIFT_PLUGGABLE_FUNC_REGISTER(_ret, _name, ...)                 \
  ::apache::thrift::detail::pluggable_function_type_t<                   \
      decltype(&THRIFT__PLUGGABLE_FUNC_DEFAULT_##_name)>                 \
      THRIFT__PLUGGABLE_FUNC_##_name =                                   \
          ::apache::thrift::detail::registerPluggableFunction(           \
              #_name,                                                    \
              static_cast<THRIFT__PLUGGABLE_FUNC_TAG_##_name*>(nullptr), \
              THRIFT__PLUGGABLE_FUNC_DEFAULT_##_name);                   \
  _ret THRIFT__PLUGGABLE_FUNC_DEFAULT_##_name(__VA_ARGS__)

#define THRIFT_PLUGGABLE_FUNC_SET(_ret, _name, ...)                  \
  _ret THRIFT__PLUGGABLE_FUNC_IMPL_##_name(__VA_ARGS__);             \
  static auto THRIFT__PLUGGABLE_FUNC_SETTER##_name =                 \
      ::apache::thrift::detail::setPluggableFunction(                \
          #_name,                                                    \
          static_cast<THRIFT__PLUGGABLE_FUNC_TAG_##_name*>(nullptr), \
          THRIFT__PLUGGABLE_FUNC_DEFAULT_##_name,                    \
          THRIFT__PLUGGABLE_FUNC_IMPL_##_name);                      \
  _ret THRIFT__PLUGGABLE_FUNC_IMPL_##_name(__VA_ARGS__)
