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
 private:
  struct mixin_member {
    t_field* mixin;
    t_field* member;
  };

 public:
  explicit t_struct(t_program* program) : t_type(program) {}

  t_struct(t_program* program, const std::string& name)
      : t_type(program, name) {}

  void set_name(const std::string& name) override {
    name_ = name;
  }

  void set_xception(bool is_xception) {
    is_xception_ = is_xception;
  }

  void set_union(bool is_union) {
    is_union_ = is_union;
  }

  t_field* get_stream_field() {
    return stream_field_;
  }
  void set_stream_field(std::unique_ptr<t_field> stream_field) {
    assert(is_paramlist_);
    assert(!stream_field_);
    assert(stream_field->get_type()->is_streamresponse());

    stream_field_ = stream_field.get();
    members_raw_.insert(members_raw_.begin(), stream_field_);
    members_.insert(members_.begin(), std::move(stream_field));
    members_in_id_order_.insert(members_in_id_order_.begin(), stream_field_);
  }

  void set_paramlist(bool is_paramlist) {
    is_paramlist_ = is_paramlist;
    assert(!is_xception_);
    assert(!is_union_);
  }

  bool append(std::unique_ptr<t_field> elem) {
    if (!members_.empty()) {
      members_.back()->set_next(elem.get());
    }
    members_raw_.push_back(elem.get());
    members_.push_back(std::move(elem));

    auto bounds = std::equal_range(
        members_in_id_order_.begin(),
        members_in_id_order_.end(),
        members_.back().get(),
        // Comparator to sort fields in ascending order by key.
        [](const t_field* a, const t_field* b) {
          return a->get_key() < b->get_key();
        });
    if (bounds.first != bounds.second) {
      return false;
    }
    members_in_id_order_.insert(bounds.second, members_.back().get());
    return true;
  }

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

  bool is_paramlist() const {
    return is_paramlist_;
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
    if (view_parent_ != nullptr) {
      return view_parent_->get_view_parent();
    } else {
      return this;
    }
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
  std::unique_ptr<t_struct> clone_DO_NOT_USE() {
    auto clone = std::make_unique<t_struct>(program_, name_);

    clone->set_xception(is_xception_);
    clone->set_union(is_union_);
    clone->set_paramlist(is_paramlist_);

    clone->set_view_parent(view_parent_);

    for (auto const& field : members_) {
      if (field.get() != stream_field_) {
        clone->append(field->clone_DO_NOT_USE());
      }
    }

    if (!!stream_field_) {
      clone->set_stream_field(stream_field_->clone_DO_NOT_USE());
    }

    return clone;
  }

  /**
   * Returns a list of pairs of mixin and mixin's members
   * e.g. for Thrift IDL
   *
   * struct Mixin1 { 1: i32 m1; }
   * struct Mixin2 { 1: i32 m2; }
   * struct Strct {
   *   1: mixin Mixin1 f1;
   *   2: mixin Mixin2 f2;
   *   3: i32 m3;
   * }
   *
   * this returns {{.mixin="f1", .member="m1"}, {.mixin="f2", .member="m2"}}
   */
  std::vector<mixin_member> get_mixins_and_members() const;

 private:
  void get_mixins_and_members_impl(t_field*, std::vector<mixin_member>&) const;

  std::vector<std::unique_ptr<t_field>> members_;
  std::vector<t_field*> members_raw_;
  std::vector<t_field*> members_in_id_order_;
  // only if is_paramlist_
  // not stored as a normal member, as it's not serialized like a normal
  // field into the pargs struct
  t_field* stream_field_ = nullptr;

  bool is_xception_{false};
  bool is_union_{false};
  bool is_paramlist_{false};

  const t_struct* view_parent_ = nullptr;
};

} // namespace compiler
} // namespace thrift
} // namespace apache
