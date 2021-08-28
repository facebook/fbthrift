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

} // namespace compiler
} // namespace thrift
} // namespace apache
