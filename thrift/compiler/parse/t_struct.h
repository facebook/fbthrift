/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#ifndef T_STRUCT_H
#define T_STRUCT_H

#include <algorithm>
#include <vector>
#include <utility>
#include <string>

#include <thrift/compiler/parse/t_type.h>
#include <thrift/compiler/parse/t_field.h>

// Forward declare that puppy
class t_program;

/**
 * A struct is a container for a set of member fields that has a name. Structs
 * are also used to implement exception types.
 *
 */
class t_struct : public t_type {
 public:
  typedef std::vector<t_field*> members_type;

  explicit t_struct(t_program* program) :
    t_type(program),
    is_xception_(false),
    is_union_(false),
    view_parent_(nullptr),
    xsd_all_(false) {}

  t_struct(t_program* program, const std::string& name) :
    t_type(program, name),
    is_xception_(false),
    is_union_(false),
    view_parent_(nullptr),
    xsd_all_(false) {}

  void set_name(const std::string& name) override { name_ = name; }

  void set_xception(bool is_xception) {
    is_xception_ = is_xception;
  }

  void set_union(bool is_union) {
    is_union_ = is_union;
  }

  void set_xsd_all(bool xsd_all) {
    xsd_all_ = xsd_all;
  }

  bool get_xsd_all() const {
    return xsd_all_;
  }

  bool append(t_field* elem) {
    members_.push_back(elem);

    typedef members_type::iterator iter_type;
    std::pair<iter_type, iter_type> bounds = std::equal_range(
        members_in_id_order_.begin(),
        members_in_id_order_.end(),
        elem,
        t_field::key_compare()
    );
    if (bounds.first != bounds.second) {
      return false;
    }
    members_in_id_order_.insert(bounds.second, elem);
    return true;
  }

  const std::vector<t_field*>& get_members() const {
    return members_;
  }

  const t_field* get_member(const std::string& name) const {
    auto const result = std::find_if(
      members_.begin(), members_.end(),
      [&name](const t_field* m) { return m->get_name() == name; }
    );
    return result == members_.end() ? nullptr : *result;
  }

  const members_type& get_sorted_members() const {
    return members_in_id_order_;
  }

  bool is_union() const {
    return is_union_;
  }

  const t_field* get_field_named(const char* name) const {
    assert(has_field_named(name));
    for (auto& member: members_) {
      if (member->get_name() == name) {
        return member;
      }
    }

    assert(false);
    return nullptr;
  }

  bool is_struct() const override { return !is_xception_; }

  bool is_xception() const override { return is_xception_; }

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
    std::vector<t_field*>::const_iterator m_iter;
    for (m_iter = members_.begin(); m_iter != members_.end(); ++m_iter) {
      if ((*m_iter)->get_name() == name) {
        return true;
      }
    }

    return false;
  }

  bool validate_field(t_field* field) {
    int key = field->get_key();
    std::vector<t_field*>::const_iterator m_iter;
    for (m_iter = members_.begin(); m_iter != members_.end(); ++m_iter) {
      if ((*m_iter)->get_key() == key) {
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

  TypeValue get_type_value() const override { return t_types::TYPE_STRUCT; }

 private:

  members_type members_;
  members_type members_in_id_order_;
  bool is_xception_;
  bool is_union_;

  const t_struct* view_parent_;

  bool xsd_all_;
};

#endif
