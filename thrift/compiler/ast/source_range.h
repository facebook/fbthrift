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
   * @param program - The program this location belongs to.
   * @param line - The 1-based line number.
   * @param column - The 1-based column number.
   */
  constexpr source_loc(
      const t_program& program, size_t line, size_t column) noexcept
      : program_(&program), line_(line), col_(column) {}

  // If the location is specified/known.
  constexpr bool has_loc() const noexcept { return program_ != nullptr; }

  // The program associated with the location.
  //
  // UB if `has_loc()` returns false.
  constexpr const t_program& program() const {
    assert(program_ != nullptr);
    return *program_;
  }

  // The 1-based line number.
  //
  // Returns 0 if unknown/not specified.
  size_t line() const noexcept { return line_; }
  // The 1-based column number.
  //
  // Returns 0 if unknown/not specified.
  size_t column() const noexcept { return col_; }

  // The 0-based byte offset from the beginning of the program.
  //
  // Returns t_program::noffset if unknown/not specified.
  size_t offset() const noexcept;

  constexpr explicit operator bool() const noexcept { return has_loc(); }
  constexpr source_loc& operator=(const source_loc& loc) noexcept = default;

 private:
  const t_program* program_ = nullptr;
  size_t line_ = 0; // 1-based
  size_t col_ = 0; // Code units (bytes), 1-based
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
      const t_program& program,
      size_t begin_line,
      size_t begin_column,
      size_t end_line,
      size_t end_column) noexcept
      : program_(&program),
        begin_line_(begin_line),
        begin_col_(begin_column),
        end_line_(end_line),
        end_col_(end_column) {}

  source_range(const source_loc& begin, const source_loc& end)
      : source_range(
            begin.program(),
            begin.line(),
            begin.column(),
            end.line(),
            end.column()) {
    if (&begin.program() != &end.program()) {
      throw std::invalid_argument("A source_range cannot span programs/files.");
    }
  }

  constexpr source_loc begin() const noexcept {
    return {*program_, begin_line_, begin_col_};
  }
  constexpr source_loc end() const noexcept {
    return {*program_, end_line_, end_col_};
  }
  constexpr const t_program& program() const {
    assert(program_ != nullptr);
    return *program_;
  }

  // If the range is specified/known.
  constexpr bool has_range() const noexcept { return program_ != nullptr; }

  constexpr explicit operator bool() const noexcept { return has_range(); }
  constexpr source_range& operator=(const source_range& range) noexcept =
      default;

 private:
  const t_program* program_ = nullptr;
  size_t begin_line_ = 0; // 1-based
  size_t begin_col_ = 0; // Code units (bytes), 1-based
  size_t end_line_ = 0; // 1-based
  size_t end_col_ = 0; // Code units (bytes), 1-based
};

} // namespace compiler
} // namespace thrift
} // namespace apache
