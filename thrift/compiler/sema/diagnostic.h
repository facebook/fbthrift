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
#include <memory>
#include <string>
#include <vector>

#include <thrift/compiler/ast/t_node.h>
#include <thrift/compiler/ast/t_program.h>

namespace apache {
namespace thrift {
namespace compiler {

// Diagnostic level.
enum class diagnostic_level {
  failure,
  parse_error,
  warning,
  info,
  debug,
};

/**
 * A diagnostic message
 */
class diagnostic {
 public:
  /**
   * Constructor for diagnostic
   *
   * @param level      - diagnostic level
   * @param message    - detailed diagnostic message
   * @param file       - file path location of diagnostic
   * @param line       - line location of diagnostic in the file, if known
   * @param token      - the last token, if applicable.
   */
  diagnostic(
      diagnostic_level level,
      std::string message,
      std::string file,
      int line = 0,
      std::string token = {})
      : level_(level),
        message_(std::move(message)),
        file_(std::move(file)),
        line_(line),
        token_(std::move(token)) {}
  diagnostic(
      diagnostic_level level,
      std::string message,
      const t_node* node,
      const t_program* program)
      : diagnostic(level, std::move(message), program->path(), node->lineno()) {
  }

  diagnostic_level level() const { return level_; }
  const std::string& message() const { return message_; }
  const std::string& file() const { return file_; }
  int lineno() const { return line_; }
  const std::string& token() const { return token_; }

  std::string str();

 private:
  diagnostic_level level_;
  std::string message_;
  std::string file_;
  int line_;
  std::string token_;

  friend bool operator==(const diagnostic& lhs, const diagnostic& rhs) {
    return lhs.level_ == rhs.level_ && lhs.line_ == rhs.line_ &&
        lhs.message_ == rhs.message_ && lhs.file_ == rhs.file_ &&
        lhs.token_ == rhs.token_;
  }
  friend bool operator!=(const diagnostic& lhs, const diagnostic& rhs) {
    return !(lhs == rhs);
  }
};

std::ostream& operator<<(std::ostream& os, const diagnostic& e);

// A container of diagnostic results.
class diagnostic_results {
 public:
  explicit diagnostic_results(std::vector<diagnostic> initial_diagnostics);
  diagnostic_results() = default;
  diagnostic_results(const diagnostic_results&) = default;
  diagnostic_results(diagnostic_results&&) noexcept = default;

  diagnostic_results& operator=(diagnostic_results&&) noexcept = default;
  diagnostic_results& operator=(const diagnostic_results&) = default;

  const std::vector<diagnostic>& diagnostics() const& { return diagnostics_; }
  void add(diagnostic diag);
  template <typename C>
  void add_all(C&& diags);

  bool has_failure() const { return count(diagnostic_level::failure) != 0; }
  std::size_t count(diagnostic_level level) const {
    return counts_.at(static_cast<size_t>(level));
  }

 private:
  std::vector<diagnostic> diagnostics_;
  std::array<int, static_cast<size_t>(diagnostic_level::debug) + 1> counts_{};

  void increment(diagnostic_level level) {
    ++counts_.at(static_cast<size_t>(level));
  }
};

template <typename C>
void diagnostic_results::add_all(C&& diags) {
  for (auto&& diag : std::forward<C>(diags)) {
    add(std::forward<decltype(diag)>(diag));
  }
}

} // namespace compiler
} // namespace thrift
} // namespace apache
