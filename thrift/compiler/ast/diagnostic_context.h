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
#include <thrift/compiler/ast/t_node.h>
#include <thrift/compiler/ast/t_program.h>
#include <thrift/compiler/diagnostic.h>

namespace apache {
namespace thrift {
namespace compiler {

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
    return get(node, [] { return std::make_unique<T>(); });
  }
  template <typename T, typename N>
  if_is_constructible<T, N&> get(N& node) { // `node` must be an lvalue.
    return get(node, [&node] { return std::make_unique<T>(node); });
  }
  template <typename T, typename N>
  if_is_constructible<T, node_metadata_cache&, const N&> get(
      N& node) { // `node` must be an lvalue.
    return get(node, [this, &node] {
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
class diagnostic_context : public diagnostics_engine,
                           public const_visitor_context {
 private:
  template <typename With>
  using if_with =
      decltype(std::declval<With&>()(std::declval<std::ostream&>()));

 public:
  explicit diagnostic_context(
      source_manager& sm,
      std::function<void(diagnostic)> report_cb,
      diagnostic_params params = {})
      : diagnostics_engine(sm, std::move(report_cb), std::move(params)) {}
  explicit diagnostic_context(
      source_manager& sm,
      diagnostic_results& results,
      diagnostic_params params = {})
      : diagnostics_engine(sm, results, std::move(params)) {}

  static diagnostic_context ignore_all(source_manager& sm) {
    return diagnostic_context(
        sm, [](const diagnostic&) {}, diagnostic_params::only_failures());
  }

  // A cache for traversal-specific metadata.
  node_metadata_cache& cache() { return cache_; }

  using diagnostics_engine::report;

  // Helpers for adding diagnostic results.
  void report(
      diagnostic_level level,
      int lineno,
      std::string token,
      std::string text,
      std::string name = "") {
    report(
        {level,
         std::move(text),
         program().path(),
         lineno,
         std::move(token),
         std::move(name)});
  }

  template <typename With, typename = if_with<With>>
  void report(
      diagnostic_level level,
      int lineno,
      std::string token,
      std::string name,
      With&& with) {
    std::ostringstream o;
    with(static_cast<std::ostream&>(o));
    report(
        {level,
         o.str(),
         program().path(),
         lineno,
         std::move(token),
         std::move(name)});
  }

  template <typename With, typename = if_with<With>>
  void report(
      diagnostic_level level, int lineno, std::string token, With&& with) {
    report(level, lineno, std::move(token), {}, std::forward<With>(with));
  }

  void report(diagnostic_level level, const t_node& node, std::string msg) {
    report(level, node.lineno(), {}, std::move(msg));
  }

  template <typename With, typename = if_with<With>>
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
    std::ostringstream o;
    with(static_cast<std::ostream&>(o));
    report({level, o.str(), std::move(path), node.lineno(), {}, {}});
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
  bool failure_if(bool cond, Args&&... args) {
    if (cond) {
      report(diagnostic_level::failure, std::forward<Args>(args)...);
    }
    return cond;
  }
  template <typename... Args>
  void check(bool cond, Args&&... args) {
    failure_if(!cond, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void warning(Args&&... args) {
    report(diagnostic_level::warning, std::forward<Args>(args)...);
  }

  template <typename... Args>
  void warning_legacy_strict(Args&&... args) {
    if (params().warn_level < 2) {
      return;
    }
    warning(std::forward<Args>(args)...);
  }

 private:
  node_metadata_cache cache_;
};

} // namespace compiler
} // namespace thrift
} // namespace apache
