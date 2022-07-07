/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
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

#include <iosfwd>
#include <string>
#include <type_traits>
#include <utility>

namespace apache {
namespace thrift {
namespace compiler {

//  json_quote_ascii
//
//  Emits a json quoted-string given an input ascii string.
std::string json_quote_ascii(std::string const& s);
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
  ~scope_guard() { full_ ? void(func_()) : void(); }
  void dismiss() { full_ = false; }

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
