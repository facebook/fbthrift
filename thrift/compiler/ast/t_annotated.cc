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

#include <thrift/compiler/ast/t_annotated.h>
#include <thrift/compiler/ast/t_const.h>

namespace apache {
namespace thrift {
namespace compiler {

const std::string t_annotated::kEmptyString{};

// NOTE: Must be defined here for t_const's destructor's defintion.
t_annotated::~t_annotated() = default;

const std::string* t_annotated::get_annotation_or_null(alias_span name) const {
  for (const auto& alias : name) {
    auto itr = annotations_.find(alias);
    if (itr != annotations_.end()) {
      return &itr->second;
    }
  }
  return nullptr;
}

void t_annotated::add_structured_annotation(std::unique_ptr<t_const> annot) {
  structured_annotations_raw_.emplace_back(annot.get());
  structured_annotations_.emplace_back(std::move(annot));
}

} // namespace compiler
} // namespace thrift
} // namespace apache
