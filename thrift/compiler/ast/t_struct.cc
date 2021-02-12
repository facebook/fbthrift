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

#include <stdexcept>

#include <thrift/compiler/ast/t_struct.h>

namespace apache {
namespace thrift {
namespace compiler {

bool t_struct::try_append(std::unique_ptr<t_field>&& elem) {
  auto elem_ptr = elem.get();

  auto bounds = std::equal_range(
      members_in_id_order_.begin(),
      members_in_id_order_.end(),
      elem_ptr,
      // Comparator to sort fields in ascending order by key.
      [](const t_field* a, const t_field* b) {
        return a->get_key() < b->get_key();
      });
  if (bounds.first != bounds.second) {
    return false;
  }

  // Take ownership.
  members_.push_back(std::move(elem));
  // Index the new field.
  if (!members_raw_.empty()) {
    members_raw_.back()->set_next(elem_ptr);
  }
  members_raw_.push_back(elem_ptr);
  members_in_id_order_.insert(bounds.second, elem_ptr);
  return true;
}

void t_struct::append(std::unique_ptr<t_field> elem) {
  if (try_append(std::move(elem))) {
    return;
  }

  if (elem->get_key() != 0) {
    throw std::runtime_error(
        "Field identifier " + std::to_string(elem->get_key()) + " for \"" +
        elem->get_name() + "\" has already been used");
  }

  // TODO(afuller): Figure out why some code relies on adding multiple id:0
  // fields, fix the code, and remove this hack.
  if (!members_raw_.empty()) {
    members_raw_.back()->set_next(elem.get());
  }
  members_raw_.push_back(elem.get());
  members_.push_back(std::move(elem));
}

} // namespace compiler
} // namespace thrift
} // namespace apache
