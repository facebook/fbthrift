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

#include <stddef.h>
#include <set>
#include <stdexcept>

#include <folly/FileUtil.h>

#include <thrift/compiler/ast/source_range.h>
#include <thrift/compiler/ast/t_program.h>

namespace apache {
namespace thrift {
namespace compiler {
namespace codemod {

struct replacement {
  // TODO(urielrivas): Revisit this whether we want to consider using a
  // source_range directly instead of offset begin_pos, end_pos.
  size_t begin_pos;
  size_t end_pos;
  std::string new_content;

  constexpr bool operator<(const replacement& replace) const noexcept {
    if (begin_pos != replace.begin_pos) {
      return begin_pos < replace.begin_pos;
    }
    return end_pos < replace.end_pos;
  }
};

/**
 * Class file_manager
 *
 * A manager to control all replacements needed to do for a Thrift file.
 */
class file_manager {
 public:
  explicit file_manager(const t_program& program) : program_(&program) {
    if (!folly::readFile(program_->path().c_str(), old_content_)) {
      throw std::runtime_error("Could not read file: " + program_->path());
    }
  }

  // Adds a given replacement to the set of replacements.
  void add(replacement replace) { replacements_.insert(std::move(replace)); }

 private:
  const t_program* program_;
  std::string old_content_;
  std::set<replacement> replacements_;
};

} // namespace codemod
} // namespace compiler
} // namespace thrift
} // namespace apache
