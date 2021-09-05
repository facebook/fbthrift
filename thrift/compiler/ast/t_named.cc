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

#include <thrift/compiler/ast/t_named.h>

#include <thrift/compiler/ast/t_const.h>
#include <thrift/compiler/ast/t_program.h>
#include <thrift/compiler/ast/t_type.h>

namespace apache {
namespace thrift {
namespace compiler {

// NOTE: Must be defined here for t_const's destructor's defintion.
t_named::~t_named() = default;

void t_named::add_structured_annotation(std::unique_ptr<t_const> annot) {
  structured_annotations_raw_.emplace_back(annot.get());
  structured_annotations_.emplace_back(std::move(annot));
}

const t_const* t_named::get_structured_annotation_or_null(
    const char* uri) const {
  for (const auto* annot : structured_annotations_raw_) {
    if (const std::string* p =
            annot->get_type()->get_annotation_or_null("thrift.uri")) {
      if (*p == uri) {
        return annot;
      }
    }
  }

  return nullptr;
}

} // namespace compiler
} // namespace thrift
} // namespace apache
