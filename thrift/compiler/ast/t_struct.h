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

  void set_xception(bool is_xception) {
    is_xception_ = is_xception;
  }

  void set_union(bool is_union) {
    is_union_ = is_union;
  }

  // Tries to append the given field, the argument is not untouched on failure.
  bool try_append(std::unique_ptr<t_field>&& elem);
  // Tries to append the gieven field, throwing an exception on failure.
  void append(std::unique_ptr<t_field> elem);

  const std::vector<t_field*>& get_members() const {
    return members_raw_;
  }

  const t_field* get_member(const std::string& name) const {
    auto const result =
        std::find_if(members_.begin(), members_.end(), [&name](auto const& m) {
          return m->get_name() == name;
        });
    return result == members_.end() ? nullptr : result->get();
  }

  const std::vector<t_field*>& get_sorted_members() const {
    return members_in_id_order_;
  }

  const t_field* get_field_named(const char* name) const {
    assert(has_field_named(name));
    for (auto& member : members_) {
      if (member->get_name() == name) {
        return member.get();
      }
    }

    assert(false);
    return nullptr;
  }

  bool is_struct() const override {
    return !is_xception_;
  }

  bool is_union() const override {
    return is_union_;
  }

  bool is_xception() const override {
    return is_xception_;
  }

  void set_view_parent(const t_struct* p) {
    view_parent_ = p;
  }

  const t_struct* get_view_parent() const {
    return view_parent_ != nullptr ? view_parent_->get_view_parent() : this;
  }

  bool is_view() const {
    return view_parent_ != nullptr;
  }

  bool has_field_named(const char* name) const {
    for (auto const& member : members_) {
      if (member->get_name() == name) {
        return true;
      }
    }

    return false;
  }

  bool validate_field(t_field* field) {
    int key = field->get_key();
    for (auto const& member : members_) {
      if (member->get_key() == key) {
        return false;
      }
    }
    return true;
  }

  std::string get_full_name() const override {
    return make_full_name("struct");
  }

  std::string get_impl_full_name() const override {
    return get_view_parent()->get_full_name();
  }

  TypeValue get_type_value() const override {
    return TypeValue::TYPE_STRUCT;
  }

  /**
   * Thrift AST nodes are meant to be non-copyable and non-movable, and should
   * never be cloned. This method exists to grand-father specific uses in the
   * target language generators. Do NOT add any new usage of this method.
   */
  template <typename S>
  static std::unique_ptr<S> clone_DO_NOT_USE(const S* sval) {
    return std::unique_ptr<S>(sval->clone_DO_NOT_USE());
  }

 protected:
  std::vector<std::unique_ptr<t_field>> members_;
  std::vector<t_field*> members_raw_;
  std::vector<t_field*> members_in_id_order_;

  bool is_xception_{false};
  bool is_union_{false};

  const t_struct* view_parent_ = nullptr;

  virtual t_struct* clone_DO_NOT_USE() const {
    auto clone = std::make_unique<t_struct>(program_, name_);
    cloneStruct(clone.get());
    return clone.release();
  }

  void cloneStruct(t_struct* clone) const {
    clone->set_xception(is_xception_);
    clone->set_union(is_union_);
    clone->set_view_parent(view_parent_);
    for (auto const& field : members_) {
      clone->append(field->clone_DO_NOT_USE());
    }
  }
};

} // namespace compiler
} // namespace thrift
} // namespace apache
