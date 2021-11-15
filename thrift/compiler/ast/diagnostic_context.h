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

#include <array>
#include <cstdio>
#include <functional>
#include <stack>
#include <string>
#include <system_error>
#include <type_traits>
#include <typeindex>
#include <utility>

#include <thrift/compiler/ast/ast_visitor.h>
#include <thrift/compiler/ast/diagnostic.h>
#include <thrift/compiler/ast/t_node.h>
#include <thrift/compiler/ast/t_program.h>

namespace apache {
namespace thrift {
namespace compiler {

struct diagnostic_params {
  bool debug = false;
  bool info = false;
  int warn_level = 1;

  /**
   * Whether or not negative enum values.
   */
  bool allow_neg_enum_vals = false;

  bool should_report(diagnostic_level level) const {
    switch (level) {
      case diagnostic_level::warning:
        return warn_level > 0;
      case diagnostic_level::debug:
        return debug;
      case diagnostic_level::info:
        return info;
      default:
        return true;
    }
  }

  // Params that only collect failures.
  static diagnostic_params only_failures() { return {false, false, 0}; }
  static diagnostic_params strict() { return {false, false, 2}; }
  static diagnostic_params keep_all() { return {true, true, 2}; }
};

// A cache for metadata associated with an AST node.
//
// Useful for avoiding re-computation during a traversal of an AST.
class node_metadata_cache {
  template <typename F>
  using element_type = typename decltype(std::declval<F>()())::element_type;
  template <typename T, typename... Args>
  using if_is_constructible =
      std::enable_if_t<std::is_constructible<T, Args...>::value, T&>;
  using key_type = std::pair<const t_node*, std::type_index>;

 public:
  // Gets or creates a cache entry of the given type T for the specified node.
  //
  // If the entry is not already present in the cache, a new entry is created
  // using the provided loader, which must return a std::unique_ptr<T>.
  template <typename..., typename N, typename F>
  element_type<F>& get(N& node, const F& loader) { // `node` must be an lvalue.
    using T = element_type<F>;
    key_type key(&node, typeid(T));
    auto itr = data_.find(key);
    if (itr == data_.end()) {
      itr = data_.emplace(std::move(key), to_any_data(loader())).first;
    }
    return *static_cast<T*>(itr->second.get());
  }

  // Gets or creates a cache entry of the given type T for the specified node.
  //
  // T must be constructible in exactly one of the following ways:
  // - T()
  // - T(const N&)
  // - T(node_metadata_cache&, const N&)
  template <typename T, typename N>
  if_is_constructible<T> get(N& node) { // `node` must be an lvalue.
    return get(node, []() { return std::make_unique<T>(); });
  }
  template <typename T, typename N>
  if_is_constructible<T, const N&> get(N& node) { // `node` must be an lvalue.
    return get(node, [&node]() {
      return std::make_unique<T>(static_cast<const N&>(node));
    });
  }
  template <typename T, typename N>
  if_is_constructible<T, node_metadata_cache&, const N&> get(
      N& node) { // `node` must be an lvalue.
    return get(node, [this, &node]() {
      return std::make_unique<T>(*this, static_cast<const N&>(node));
    });
  }

 private:
  // TODO(afuller): Use std::any when c++17 can be used.
  using any_data = std::unique_ptr<void, void (*const)(void*)>;
  std::map<key_type, any_data> data_;

  template <typename T>
  any_data to_any_data(std::unique_ptr<T> value) {
    return {value.release(), [](void* ptr) { delete static_cast<T*>(ptr); }};
  }
};

// A context aware reporter for diagnostic results.
class diagnostic_context : public const_visitor_context {
 public:
  explicit diagnostic_context(
      std::function<void(diagnostic)> report_cb, diagnostic_params params = {})
      : report_cb_(std::move(report_cb)), params_(std::move(params)) {}
  explicit diagnostic_context(
      diagnostic_results& results, diagnostic_params params = {})
      : diagnostic_context(
            [&results](diagnostic diag) { results.add(std::move(diag)); },
            std::move(params)) {}

  static diagnostic_context ignore_all() {
    return diagnostic_context{
        [](const diagnostic&) {}, diagnostic_params::only_failures()};
  }

  diagnostic_params& params() { return params_; }
  const diagnostic_params& params() const { return params_; }

  // The program currently being analyzed.
  // TODO(afuller): Consider having every t_node know what program it was
  // defined in instead.
  const t_program* program() {
    assert(!programs_.empty());
    return programs_.top();
  }
  void start_program(const t_program* program) {
    assert(program != nullptr);
    programs_.push(program);
  }
  void end_program(const t_program*) { programs_.pop(); }

  void report(diagnostic diag) {
    if (params_.should_report(diag.level())) {
      report_cb_(std::move(diag));
    }
  }

  template <typename C>
  void report_all(C&& diags) {
    for (auto&& diag : std::forward<C>(diags)) {
      report(std::forward<decltype(diag)>(diag));
    }
  }

  // A cache for traversal-specific metadata.
  node_metadata_cache& cache() { return cache_; }

  // Helpers for adding diagnostic results.
  void report(
      diagnostic_level level,
      int lineno,
      std::string token,
      std::string text,
      std::string name = "") {
    if (!params_.should_report(level)) {
      return;
    }

    report_cb_(
        {level,
         std::move(text),
         program()->path(),
         lineno,
         std::move(token),
         std::move(name)});
  }

  template <
      typename With,
      typename = decltype(std::declval<With&>()(std::declval<std::ostream&>()))>
  void report(
      diagnostic_level level,
      std::string path,
      int lineno,
      std::string token,
      std::string name,
      With&& with) {
    if (!params_.should_report(level)) {
      return;
    }

    std::ostringstream o;
    with(static_cast<std::ostream&>(o));
    report_cb_(
        {level,
         o.str(),
         std::move(path),
         lineno,
         std::move(token),
         std::move(name)});
  }

  template <
      typename With,
      typename = decltype(std::declval<With&>()(std::declval<std::ostream&>()))>
  void report(
      diagnostic_level level,
      std::string path,
      int lineno,
      std::string token,
      With&& with) {
    if (!params_.should_report(level)) {
      return;
    }

    std::ostringstream o;
    with(static_cast<std::ostream&>(o));
    report_cb_({
        level,
        o.str(),
        std::move(path),
        lineno,
        std::move(token),
    });
  }

  template <
      typename With,
      typename = decltype(std::declval<With&>()(std::declval<std::ostream&>()))>
  void report(
      diagnostic_level level,
      int lineno,
      std::string token,
      std::string name,
      With&& with) {
    report(
        level,
        program()->path(),
        lineno,
        std::move(token),
        std::move(name),
        std::forward<With>(with));
  }

  template <
      typename With,
      typename = decltype(std::declval<With&>()(std::declval<std::ostream&>()))>
  void report(
      diagnostic_level level, int lineno, std::string token, With&& with) {
    report(
        level,
        program()->path(),
        lineno,
        std::move(token),
        "",
        std::forward<With>(with));
  }

  void report(diagnostic_level level, const t_node& node, std::string msg) {
    report(level, node.lineno(), {}, std::move(msg));
  }

  template <
      typename With,
      typename = decltype(std::declval<With&>()(std::declval<std::ostream&>()))>
  void report(diagnostic_level level, const t_node& node, With&& with) {
    report(level, node.lineno(), {}, std::forward<With>(with));
  }

  void report(
      diagnostic_level level,
      std::string name,
      const t_node& node,
      std::string msg) {
    report(level, node.lineno(), {}, std::move(msg), std::move(name));
  }

  template <typename With>
  void report(
      diagnostic_level level,
      std::string name,
      const t_node& node,
      std::string msg,
      With&& with) {
    report(
        level,
        node.lineno(),
        {},
        std::move(msg),
        std::move(name),
        std::forward<With>(with));
  }

  // TODO(ytj): use path in t_node and delete this function overload
  template <typename With>
  void report(
      diagnostic_level level,
      const t_node& node,
      std::string path,
      With&& with) {
    report(level, std::move(path), node.lineno(), {}, std::forward<With>(with));
  }

  template <typename With>
  void report(diagnostic_level level, With&& with) {
    assert(current() != nullptr);
    report(level, current()->lineno(), {}, std::forward<With>(with));
  }

  template <typename... Args>
  void failure(Args&&... args) {
    report(diagnostic_level::failure, std::forward<Args>(args)...);
  }
  template <typename... Args>
  void warning(Args&&... args) {
    report(diagnostic_level::warning, std::forward<Args>(args)...);
  }
  template <typename... Args>
  void warning_strict(Args&&... args) {
    if (params_.warn_level < 2) {
      return;
    }
    warning(std::forward<Args>(args)...);
  }
  template <typename... Args>
  void info(Args&&... args) {
    report(diagnostic_level::info, std::forward<Args>(args)...);
  }
  template <typename... Args>
  void debug(Args&&... args) {
    report(diagnostic_level::debug, std::forward<Args>(args)...);
  }

 private:
  std::function<void(diagnostic)> report_cb_;
  node_metadata_cache cache_;

  std::stack<const t_program*> programs_;
  diagnostic_params params_;
};

} // namespace compiler
} // namespace thrift
} // namespace apache
