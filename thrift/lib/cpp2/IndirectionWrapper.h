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

#include <memory>
#include <utility>

#include <folly/Overload.h>
#include <thrift/lib/cpp2/Thrift.h>

namespace apache {
namespace thrift {

//  IndirectionWrapper
//
//  Owns a value of type T, which may be incomplete at instantiation time.
//
//  Provides access to the T and works with cpp.indirection.
//
//  Expected to be used as a base class. Enforced by marking the dtor protected.
template <typename Derived, typename T>
class IndirectionWrapper {
 private:
  std::unique_ptr<T> raw_{new T()};

 protected:
  ~IndirectionWrapper() = default;

 public:
  FBTHRIFT_CPP_DEFINE_MEMBER_INDIRECTION_FN(__fbthrift_data());

  IndirectionWrapper() = default;

  IndirectionWrapper(const IndirectionWrapper& other)
      : raw_(new T(other.__fbthrift_data())) {}

  IndirectionWrapper(IndirectionWrapper&& other) noexcept(false)
      : raw_(new T(std::move(other.__fbthrift_data()))) {}

  IndirectionWrapper& operator=(const IndirectionWrapper& other) noexcept(
      std::is_nothrow_copy_assignable_v<T>) {
    __fbthrift_data() = other.__fbthrift_data();
    return *this;
  }

  IndirectionWrapper& operator=(IndirectionWrapper&& other) noexcept(
      std::is_nothrow_move_assignable_v<T>) {
    __fbthrift_data() = std::move(other.__fbthrift_data());
    return *this;
  }

  friend bool operator==(const Derived& lhs, const Derived& rhs) {
    return lhs.__fbthrift_data() == rhs.__fbthrift_data();
  }

  friend bool operator<(const Derived& lhs, const Derived& rhs) {
    return lhs.__fbthrift_data() < rhs.__fbthrift_data();
  }

  void __clear() { apache::thrift::clear(__fbthrift_data()); }

  T& __fbthrift_data() & { return *raw_; }

  const T& __fbthrift_data() const& { return *raw_; }

  T&& __fbthrift_data() && { return std::move(*raw_); }

  const T&& __fbthrift_data() const&& { return std::move(*raw_); }

 private:
  friend struct apache::thrift::detail::st::struct_private_access;

  void __fbthrift_clear() { apache::thrift::clear(__fbthrift_data()); }
};
} // namespace thrift
} // namespace apache
