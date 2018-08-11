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

#include <cstdint>
#include <initializer_list>
#include <type_traits>
#include <utility>

namespace apache {
namespace thrift {
namespace detail {

template <std::intmax_t Id, typename Ref>
struct argument_wrapper {
  explicit argument_wrapper(Ref ref) : ptr_(&ref) {}

  Ref extract() const {
    return static_cast<Ref>(*ptr_);
  }

 private:
  std::remove_reference_t<Ref>* ptr_;
};

template <std::intmax_t Id, typename T>
struct argument_wrapper<Id, std::initializer_list<T>> {
  explicit argument_wrapper(std::initializer_list<T> list) : list_(list) {}

  std::initializer_list<T> extract() const {
    return list_;
  }

 private:
  std::initializer_list<T> list_;
};

template <std::intmax_t Id, typename T>
argument_wrapper<Id, std::initializer_list<T>> wrap_argument(
    std::initializer_list<T> value) {
  return argument_wrapper<Id, std::initializer_list<T>>(
      static_cast<std::initializer_list<T>>(value));
}

template <std::intmax_t Id, typename T>
argument_wrapper<Id, T&&> wrap_argument(T&& value) {
  return argument_wrapper<Id, T&&>(static_cast<T&&>(value));
}

template <typename S, std::intmax_t... Id, typename... T>
constexpr S make_constant(argument_wrapper<Id, T>... arg) {
  using _ = int[];
  S s;
  void(_{0, (void(s.__set_field(arg)), 0)...});
  return s;
}

} // namespace detail
} // namespace thrift
} // namespace apache
