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
#include <set>
#include <stack>
#include <string>
#include <type_traits>
#include <unordered_map>
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
//  This is a stable sort i.e. it preserves the relative order of elements that
//  don't depend on each other, either directly or indirectly.
template <
    typename T,
    typename ForwardIt,
    typename Edges,
    typename Compare = std::less<T>>
std::vector<T> topological_sort(
    ForwardIt begin,
    ForwardIt end,
    Edges edges,
    Compare comp = {}) {
  struct QueueItem {
    T value;
    size_t numOutEdges;
  };

  auto less = [comp](const QueueItem& lhs, const QueueItem& rhs) {
    return lhs.numOutEdges != rhs.numOutEdges
        ? lhs.numOutEdges < rhs.numOutEdges
        : comp(lhs.value, rhs.value);
  };

  // A "priority queue" of nodes with lexicographical ordering on
  // (number-of-out-edges, value). We use set instead of priority_queue
  // because the latter doesn't permit removal of non-top elements.
  auto queue = std::set<QueueItem, decltype(less)>(less);

  struct NodeInfo {
    std::vector<T> inEdges;
    size_t numOutEdges;
  };

  // Get incoming edges for each node and populate queue.
  auto nodeInfo = std::unordered_map<T, NodeInfo>();
  for (auto it = begin; it != end; ++it) {
    auto outEdges = edges(*it);
    auto node = QueueItem{*it, outEdges.size()};
    queue.insert(node);
    nodeInfo[*it].numOutEdges = outEdges.size();
    for (const auto& edge : outEdges) {
      nodeInfo[edge].inEdges.push_back(*it);
    }
  }

  auto output = std::vector<T>();
  while (!queue.empty()) {
    auto top = queue.begin();
    const auto& info = nodeInfo[top->value];
    for (const auto& neighbor : info.inEdges) {
      auto& neighborInfo = nodeInfo[neighbor];
      auto it = queue.find(QueueItem{neighbor, neighborInfo.numOutEdges});
      if (it != queue.end()) {
        auto item = *it;
        --item.numOutEdges;
        queue.erase(it);
        queue.insert(item);
      }
      --neighborInfo.numOutEdges;
    }
    output.push_back(top->value);
    queue.erase(top);
  }
  return output;
}

} // namespace compiler
} // namespace thrift
} // namespace apache
