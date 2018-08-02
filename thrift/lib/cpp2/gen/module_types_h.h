/*
 * Copyright 2018-present Facebook, Inc.
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

#pragma once

#include <initializer_list>
#include <type_traits>
#include <utility>

namespace apache {
namespace thrift {
namespace detail {

template <std::intmax_t Id, typename T>
struct argument_wrapper {
  static_assert(
      std::is_rvalue_reference<T&&>::value,
      "this wrapper handles only rvalues and initializer_list");

  template <typename U>
  explicit argument_wrapper(U&& value) : argument_(std::forward<U>(value)) {
    static_assert(
        std::is_rvalue_reference<U&&>::value,
        "this wrapper handles only rvalues and initializer_list");
  }

  explicit argument_wrapper(const char* str) : argument_(str) {}

  T&& move() {
    return std::move(argument_);
  }

 private:
  T argument_;
};

template <std::intmax_t Id, typename T>
argument_wrapper<Id, std::initializer_list<T>> wrap_argument(
    std::initializer_list<T> value) {
  return argument_wrapper<Id, std::initializer_list<T>>(std::move(value));
}

template <std::intmax_t Id, typename T>
argument_wrapper<Id, T&&> wrap_argument(T&& value) {
  static_assert(std::is_rvalue_reference<T&&>::value, "internal thrift error");
  return argument_wrapper<Id, T&&>(std::forward<T>(value));
}

template <std::intmax_t Id>
argument_wrapper<Id, const char*> wrap_argument(const char* str) {
  return argument_wrapper<Id, const char*>(str);
}

template <typename S, std::intmax_t... Id, typename... T>
constexpr S make_constant(argument_wrapper<Id, T>&&... arg) {
  using _ = int[];
  S s;
  void(_{0, (void(s.__set_field(std::move(arg))), 0)...});
  return s;
}

} // namespace detail
} // namespace thrift
} // namespace apache
