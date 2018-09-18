/*
 * Copyright 2018-present Facebook, Inc.
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#pragma once

#include <iosfwd>
#include <string>
#include <type_traits>
#include <utility>

namespace apache {
namespace thrift {
namespace compiler {

#if __cpp_lib_exchange_function || _LIBCPP_STD_VER > 11 || _MSC_VER

/* using override */ using std::exchange;

#else

//  mimic: std::exchange, C++14
//  from: http://en.cppreference.com/w/cpp/utility/exchange, CC-BY-SA
template <class T, class U = T>
T exchange(T& obj, U&& new_value) {
  T old_value = std::move(obj);
  obj = std::forward<U>(new_value);
  return old_value;
}

#endif

//  strip_left_margin
//
//  Looks for the least indented non-whitespace-only line and removes its amount
//  of leading whitespace from every line. Assumes leading whitespace is either
//  all spaces or all tabs.
//
//  The leading line is removed if it is whitespace-only. The trailing line is
//  kept but its content removed if it is whitespace-only.
//
//  Purpose: including a multiline string literal in source code, indented to
//  the level expected from context.
//
//  mimic: folly::stripLeftMargin
std::string strip_left_margin(std::string const& s);

//  json_quote_ascii
//
//  Emits a json quoted-string given an input ascii string.
std::ostream& json_quote_ascii(std::ostream& o, std::string const& s);

namespace detail {

template <typename F>
class scope_guard {
 public:
  explicit scope_guard(F&& f) noexcept
      : full_(true), func_(std::forward<F>(f)) {}
  scope_guard(scope_guard&& that) noexcept
      : full_(std::exchange(that.full_, false)), func_(std::move(that.func_)) {}
  scope_guard& operator=(scope_guard&& that) = delete;
  ~scope_guard() {
    full_ ? void(func_()) : void();
  }
  void dismiss() {
    full_ = false;
  }

 private:
  using Func = std::decay_t<F>;
  static_assert(std::is_nothrow_move_constructible<Func>::value, "invalid");
  static_assert(std::is_nothrow_constructible<Func, F&&>::value, "invalid");

  bool full_;
  Func func_;
};

} // namespace detail

//  make_scope_guard
//
//  Invokes "cleanup" code unconditionally at end-of-scope, unless dismissed.
//
//  Cleanup must be nothrow-decay-copyable, decayed nothrow-move-constructible,
//  and decayed invocable.
template <typename F>
auto make_scope_guard(F&& f) {
  return detail::scope_guard<F>(std::forward<F>(f));
}

} // namespace compiler
} // namespace thrift
} // namespace apache
