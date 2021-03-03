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

#include <algorithm>
#include <cassert>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <thrift/compiler/ast/t_field.h>
#include <thrift/compiler/ast/t_type.h>

namespace apache {
namespace thrift {
namespace compiler {

// Forward declare that puppy
class t_program;

/**
 * A struct is a container for a set of member fields that has a name. Structs
 * are also used to implement exception and union types.
 *
 */
class t_struct : public t_type {
 public:
  explicit t_struct(t_program* program) : t_type(program) {}

  t_struct(t_program* program, const std::string& name)
      : t_type(program, name) {}

  // Tries to append the given field, the argument is untouched on failure.
  bool try_append_field(std::unique_ptr<t_field>&& elem);

  // Get the fields, in the order they were added.
  const std::vector<const t_field*>& fields() const {
    return fields_ordinal_order_;
  }
  // Get the fields, ordered by id.
  const std::vector<const t_field*>& fields_id_order() const {
    return fields_id_order_;
  }

  // If this struct has any fields.
  bool has_fields() const {
    return !fields_.empty();
  }

  // Access the field by index, id, or name.
  t_field* get_field(size_t index) {
    return fields_.at(index).get();
  }
  const t_field* get_field(size_t index) const {
    return fields_.at(index).get();
  }
  const t_field* get_field_by_id(int32_t id) const;
  const t_field* get_field_by_name(const std::string& name) const {
    auto itr = fields_by_name_.find(name);
    return itr == fields_by_name_.end() ? nullptr : itr->second;
  }

  bool is_struct() const override {
    return !is_xception();
  }

  std::string get_full_name() const override {
    return make_full_name("struct");
  }

  type get_type_value() const override {
    return type::t_struct;
  }

 protected:
  std::vector<std::unique_ptr<t_field>> fields_;
  std::vector<const t_field*> fields_ordinal_order_;
  std::vector<const t_field*> fields_id_order_;
  std::map<std::string, const t_field*> fields_by_name_;

  ////
  // Everyting below here is for backwards compatiblity, and will be removed.
  ////
 public:
  /**
   * Thrift AST nodes are meant to be non-copyable and non-movable, and should
   * never be cloned. This method exists to grand-father specific uses in the
   * target language generators. Do NOT add any new usage of this method.
   */
  template <typename S>
  static std::unique_ptr<S> clone_DO_NOT_USE(const S* sval) {
    return std::unique_ptr<S>(sval->clone_DO_NOT_USE());
  }

  // Tries to append the gieven field, throwing an exception on failure.
  void append(std::unique_ptr<t_field> elem);

  const t_field* get_field_named(const std::string& name) const {
    const auto* result = get_field_by_name(name);
    assert(result != nullptr);
    return result;
  }

  const std::vector<t_field*>& get_members() const {
    return fields_raw_;
  }

  const std::vector<t_field*>& get_sorted_members() const {
    return fields_raw_id_order_;
  }

  const t_field* get_member(const std::string& name) const {
    return get_field_by_name(name);
  }

  bool has_field_named(const std::string& name) const {
    return get_field_by_name(name) != nullptr;
  }

  bool validate_field(t_field* field) {
    return get_field_by_id(field->get_key()) == nullptr;
  }

 protected:
  std::vector<t_field*> fields_raw_;
  std::vector<t_field*> fields_raw_id_order_;

  virtual t_struct* clone_DO_NOT_USE() const {
    auto clone = std::make_unique<t_struct>(program_, name_);
    cloneStruct(clone.get());
    return clone.release();
  }

  void cloneStruct(t_struct* clone) const {
    for (auto const& field : fields_) {
      clone->append(field->clone_DO_NOT_USE());
    }
  }
};

} // namespace compiler
} // namespace thrift
} // namespace apache
