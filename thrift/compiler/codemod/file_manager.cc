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

#include <folly/FileUtil.h>

#include <thrift/compiler/codemod/file_manager.h>

namespace apache {
namespace thrift {
namespace compiler {
namespace codemod {

// Write all existing replacements back to file.
void file_manager::apply_replacements() {
  size_t prev_end = 0;
  std::string new_content;

  // Perform the replacements.
  for (const auto& r : replacements_) {
    // Only apply replacements that are not overlapped with previous one.
    if (prev_end <= r.begin_pos) {
      new_content.append(old_content_, prev_end, r.begin_pos - prev_end);
      new_content += r.new_content;
      prev_end = r.end_pos;
    }
  }

  // Get the last part of the file.
  new_content += old_content_.substr(prev_end);

  // No need for catching nor throwing here since if file doesn't exist
  // the constructor itself will throw an exception.
  folly::writeFile(new_content, program_->path().c_str());
}

} // namespace codemod
} // namespace compiler
} // namespace thrift
} // namespace apache
