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

#include <cassert>
#include <stdexcept>

namespace apache {
namespace thrift {
namespace compiler {

class t_program;

/**
 * class source_loc
 *
 * Source location information of a parsed element.
 */
class source_loc final {
 public:
  constexpr source_loc() noexcept = default;
  constexpr source_loc(source_loc const&) noexcept = default;

  /**
   * Constructor for source_loc
   *
   * @param line - The 1-based line of the location.
   * @param column - The 1-based column of the location.
   * @param program - The Thrift program this location belongs to.
   */
  constexpr source_loc(int line, int column, const t_program& program) noexcept
      : line_(line), column_(column), program_(&program) {}

  constexpr int line() const noexcept { return line_; }
  constexpr int column() const noexcept { return column_; }
  constexpr const t_program& program() const {
    assert(program_ != nullptr);
    return *program_;
  }

  constexpr explicit operator bool() const noexcept {
    return program_ != nullptr;
  }
  constexpr source_loc& operator=(const source_loc& loc) noexcept = default;

 private:
  int line_ = 0; // 1-based
  int column_ = 0; // Code units (bytes), 1-based
  const t_program* program_ = nullptr;
};

/**
 * class source_range
 *
 * Source range information of a parsed element.
 */
class source_range final {
 public:
  constexpr source_range() noexcept = default;
  constexpr source_range(source_range const&) noexcept = default;

  constexpr source_range(
      int begin_line,
      int begin_column,
      int end_line,
      int end_column,
      const t_program& program) noexcept
      : begin_line_(begin_line),
        begin_column_(begin_column),
        end_line_(end_line),
        end_column_(end_column),
        program_(&program) {}

  source_range(const source_loc& begin, const source_loc& end)
      : source_range(
            begin.line(),
            begin.column(),
            end.line(),
            end.column(),
            begin.program()) {
    if (&begin.program() != &end.program()) {
      throw std::invalid_argument(
          "Construction error: Cannot construct from begin source_loc and end "
          "source_loc with different programs to a single source_range.");
    }
  }

  constexpr source_loc begin() const noexcept {
    return {begin_line_, begin_column_, *program_};
  }
  constexpr source_loc end() const noexcept {
    return {end_line_, end_column_, *program_};
  }
  constexpr const t_program& program() const {
    assert(program_ != nullptr);
    return *program_;
  }

  constexpr explicit operator bool() const noexcept {
    return program_ != nullptr;
  }
  constexpr source_range& operator=(const source_range& range) noexcept =
      default;

 private:
  int begin_line_ = 0; // 1-based
  int begin_column_ = 0; // Code units (bytes), 1-based
  int end_line_ = 0; // 1-based
  int end_column_ = 0; // Code units (bytes), 1-based
  const t_program* program_ = nullptr;
};

} // namespace compiler
} // namespace thrift
} // namespace apache
