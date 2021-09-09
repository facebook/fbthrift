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

#include <thrift/compiler/ast/source_range.h>

#include <ostream>
#include <thrift/compiler/ast/t_program.h>

namespace apache {
namespace thrift {
namespace compiler {

namespace {

template <typename T>
constexpr int cmp(const T& lhs, const T& rhs) noexcept {
  return lhs == rhs ? 0 : (lhs < rhs ? -1 : 1);
}

// TODO(afuller): Make constexpr when
// std::basic_string<CharT,Traits,Allocator>::compare
// becomes constexpr in c++20.
int compare_programs(const t_program* lhs, const t_program* rhs) noexcept {
  if (lhs == rhs) {
    return 0;
  }

  // nullptr comes first.
  if (lhs == nullptr) {
    return rhs == nullptr ? 0 : -1;
  }
  if (rhs == nullptr) {
    return 1;
  }

  // Then order by path.
  if (int result = lhs->path().compare(rhs->path())) {
    return result;
  }

  // Then by pointer, to break ties.
  return cmp(lhs, rhs);
}

} // namespace

// Note: Must be defined here because requires t_program's definition.
size_t source_loc::offset() const noexcept {
  if (program_ == nullptr) {
    return t_program::noffset;
  }
  // TODO(afuller): Take into account multi-byte UTF-8 characters. The current
  // logic assumes every 'column' is one byte wide, which is only true for
  // single byte unicode characters.
  return program_->get_byte_offset(line_, col_ > 0 ? col_ - 1 : 0);
}

int source_loc::compare(const source_loc& lhs, const source_loc& rhs) noexcept {
  // Order by program.
  if (auto result = compare_programs(lhs.program_, rhs.program_)) {
    return result;
  }
  // Then line.
  if (auto result = cmp(lhs.line_, rhs.line_)) {
    return result;
  }
  // Then column.
  return cmp(lhs.col_, rhs.col_);
}

std::ostream& operator<<(std::ostream& os, const source_loc& rhs) {
  if (rhs.program_ != nullptr) {
    os << rhs.program_->path();
    if (rhs.line_ > 0) {
      os << ":" << rhs.line_;
      if (rhs.col_ > 0) {
        os << ":" << rhs.col_;
      }
    }
  }
  return os;
}

source_range::source_range(const source_loc& begin, const source_loc& end)
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

int source_range::compare(
    const source_range& lhs, const source_range& rhs) noexcept {
  // Order by program.
  if (auto result = compare_programs(lhs.program_, rhs.program_)) {
    return result;
  }
  // Then begin line.
  if (auto result = cmp(lhs.begin_line_, rhs.begin_line_)) {
    return result;
  }
  // Then begin column.
  if (auto result = cmp(lhs.begin_col_, rhs.begin_col_)) {
    return result;
  }
  // Then end line.
  if (auto result = cmp(lhs.end_line_, rhs.end_line_)) {
    return result;
  }
  // Then end column.
  return cmp(lhs.end_col_, rhs.end_col_);
}

} // namespace compiler
} // namespace thrift
} // namespace apache
