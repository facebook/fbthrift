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

#include <thrift/compiler/ast/t_program.h>

namespace apache {
namespace thrift {
namespace compiler {

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

int source_loc::cmp(const t_program* lhs, const t_program* rhs) {
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
  return cmp<const t_program*>(lhs, rhs);
}

int source_loc::cmp(const source_loc& lhs, const source_loc& rhs) {
  // Order by program.
  if (auto result = cmp(lhs.program_, rhs.program_)) {
    return result;
  }
  // Then line.
  if (auto result = cmp(lhs.line_, rhs.line_)) {
    return result;
  }
  // Then column.
  return cmp(lhs.col_, rhs.col_);
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

} // namespace compiler
} // namespace thrift
} // namespace apache
