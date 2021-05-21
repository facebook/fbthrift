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
#include <utility>

#include <thrift/compiler/ast/t_node.h>
#include <thrift/compiler/ast/t_program.h>
#include <thrift/compiler/sema/diagnostic.h>

namespace apache {
namespace thrift {
namespace compiler {

struct diagnostic_params {
  bool debug = false;
  bool info = false;
  int warn_level = 1;

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

// A context aware reporter for diagnostic results.
class diagnostic_context {
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

  // Helpers for adding diagnostic results.
  // TODO(afuller): The variadic template doesn't actually provide additional
  // type safety. Update this function to use varargs with the format attribute
  // instead.
  template <typename... Arg>
  void report(
      diagnostic_level level,
      int lineno,
      std::string token,
      const char* fmt,
      Arg&&... arg);

  template <typename... Args>
  void report(
      diagnostic_level level,
      const t_node* node,
      const char* fmt,
      Args&&... args) {
    report(level, node->lineno(), {}, fmt, std::forward<Args>(args)...);
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

  std::stack<const t_program*> programs_;
  diagnostic_params params_;
};

namespace detail {
// This elaborate dance is required to avoid triggering -Wformat-security in the
// case where we have no format arguments.
template <typename... Arg>
int snprintf_with_param_pack(
    char* str, size_t size, const char* fmt, Arg&&... arg) {
  return snprintf(str, size, fmt, std::forward<Arg>(arg)...);
}

template <>
inline int snprintf_with_param_pack(char* str, size_t size, const char* fmt) {
  return snprintf(str, size, "%s", fmt);
}

} // namespace detail

template <typename... Arg>
void diagnostic_context::report(
    diagnostic_level level,
    int lineno,
    std::string token,
    const char* fmt,
    Arg&&... arg) {
  if (!params_.should_report(level)) {
    return;
  }
  const size_t buffer_size = 1024;
  std::array<char, buffer_size> buffer;
  std::string message;

  int ret = detail::snprintf_with_param_pack(
      buffer.data(), buffer_size, fmt, std::forward<Arg>(arg)...);
  if (ret < 0) {
    // Technically we could be OOM here, so the following line would fail.
    // But...
    throw std::system_error(errno, std::generic_category(), "In snprintf(...)");
  }

  auto full_length = static_cast<size_t>(ret);
  if (full_length < buffer_size) {
    message = std::string{buffer.data()};
  } else {
    // In the (extremely) unlikely case that the message is 1024-char or
    // longer, we do dynamic allocation.
    //
    // "+ 1" for the NULL-terminator.
    std::vector<char> dyn_buffer(static_cast<size_t>(full_length + 1), '\0');

    ret = detail::snprintf_with_param_pack(
        dyn_buffer.data(), dyn_buffer.size(), fmt, std::forward<Arg>(arg)...);
    if (ret < 0) {
      throw std::system_error(
          errno, std::generic_category(), "In second snprintf(...)");
    }

    assert(static_cast<size_t>(ret) < dyn_buffer.size());

    message = std::string{dyn_buffer.data()};
  }

  report_cb_({
      level,
      std::move(message),
      program()->path(),
      lineno,
      std::move(token),
  });
}

} // namespace compiler
} // namespace thrift
} // namespace apache
