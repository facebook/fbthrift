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

#include <thrift/compiler/ast/t_structured.h>

namespace apache {
namespace thrift {
namespace compiler {

namespace {

template <typename C>
auto find_by_id(const C& fields_id_order, const t_field& field) {
  return std::equal_range(
      fields_id_order.begin(),
      fields_id_order.end(),
      &field,
      // Comparator to sort fields in ascending order by key.
      [](const t_field* a, const t_field* b) {
        return a->get_key() < b->get_key();
      });
}

} // namespace

bool t_structured::try_append_field(std::unique_ptr<t_field>&& elem) {
  auto elem_ptr = elem.get();

  auto bounds = find_by_id(fields_id_order_, *elem_ptr);
  if (bounds.first != bounds.second) {
    return false;
  }

  // Take ownership.
  if (!fields_.empty()) {
    fields_.back()->set_next(elem_ptr);
  }
  fields_.push_back(std::move(elem));

  // Index the new field.
  if (!elem_ptr->get_name().empty()) {
    fields_by_name_.put(elem_ptr);
  }
  fields_ordinal_order_.emplace_back(elem_ptr);
  fields_id_order_.emplace(bounds.second, elem_ptr);

  fields_raw_.push_back(elem_ptr);
  fields_raw_id_order_.insert(
      find_by_id(fields_raw_id_order_, *elem_ptr).second, elem_ptr);
  return true;
}

const t_field* t_structured::get_field_by_id(int32_t id) const {
  t_field dummy(nullptr, {}, id);
  auto bounds = find_by_id(fields_id_order_, dummy);
  return bounds.first == bounds.second ? nullptr : *bounds.first;
}

void t_structured::append(std::unique_ptr<t_field> elem) {
  if (try_append_field(std::move(elem))) {
    return;
  }

  if (elem->get_key() != 0) {
    throw std::runtime_error(
        "Field identifier " + std::to_string(elem->get_key()) + " for \"" +
        elem->get_name() + "\" has already been used");
  }

  // TODO(afuller): Figure out why some code relies on adding multiple id:0
  // fields, fix the code, and remove this hack.
  if (!fields_raw_.empty()) {
    fields_raw_.back()->set_next(elem.get());
  }
  if (!elem->get_name().empty()) {
    fields_by_name_.put(elem.get());
  }
  fields_ordinal_order_.push_back(elem.get());
  fields_raw_.push_back(elem.get());
  fields_.push_back(std::move(elem));
}

} // namespace compiler
} // namespace thrift
} // namespace apache
