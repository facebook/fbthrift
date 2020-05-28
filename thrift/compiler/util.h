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

#include <iosfwd>
#include <stack>
#include <string>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

namespace apache {
namespace thrift {
namespace compiler {

#if defined(__cpp_lib_exchange_function) || _LIBCPP_STD_VER > 11 || _MSC_VER

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

//  topological_sort
//
//  Given a container of objects and a function to obtain dependencies,
//  produces a vector of those nodes in a topologicaly sorted order.
template <typename T, typename ForwardIt, typename Edges>
std::vector<T> topological_sort(ForwardIt begin, ForwardIt end, Edges edges) {
  struct IterState {
    T node;
    std::vector<T> edges;
    typename std::vector<T>::const_iterator pos;

    IterState(T n, std::vector<T> e)
        : node(std::move(n)), edges(std::move(e)), pos(edges.begin()) {}

    // Prevent accidental move/copy, because the iterator needs to be properly
    // updated.
    IterState(const IterState&) = delete;
    IterState(IterState&&) = delete;
    IterState& operator=(const IterState&) = delete;
    IterState& operator=(IterState&&) = delete;
  };

  std::unordered_set<T> visited;
  std::vector<T> output;

  for (auto it = begin; it != end; ++it) {
    if (visited.count(*it) != 0) {
      continue;
    }
    std::stack<IterState> st;
    st.emplace(*it, edges(*it));
    visited.insert(*it);
    while (!st.empty()) {
      IterState& s = st.top();
      if (s.pos == s.edges.end()) {
        output.emplace_back(s.node);
        st.pop();
        continue;
      }

      if (visited.find(*s.pos) == visited.end()) {
        st.emplace(*s.pos, edges(*s.pos));
        visited.insert(*s.pos);
      }
      ++s.pos;
    }
  }
  return output;
}

} // namespace compiler
} // namespace thrift
} // namespace apache
