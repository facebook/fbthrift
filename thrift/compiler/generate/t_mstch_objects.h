/*
 * Copyright 2004-present Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <mstch/mstch.hpp>
#include <unordered_map>

#include <thrift/compiler/generate/t_generator.h>

class mstch_base;
class mstch_generators;

enum ELEMENT_POSITION {
  NONE = 0,
  FIRST = 1,
  LAST = 2,
  FIRST_AND_LAST = 3,
};

struct mstch_cache {
  std::unordered_map<std::string, std::shared_ptr<mstch_base>> enums_;
  std::unordered_map<std::string, std::shared_ptr<mstch_base>> structs_;
};

class enum_value_generator {
 public:
  virtual ~enum_value_generator() = default;
  virtual std::shared_ptr<mstch_base> generate(
      t_enum_value const* enum_value,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t /*index*/ = 0) const;
};

class enum_generator {
 public:
  virtual ~enum_generator() = default;
  virtual std::shared_ptr<mstch_base> generate(
      t_enum const* enm,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t /*index*/ = 0) const;
};

class type_generator {
 public:
  type_generator() = default;
  virtual ~type_generator() = default;
  virtual std::shared_ptr<mstch_base> generate(
      t_type const* type,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t /*index*/ = 0) const;
};

class field_generator {
 public:
  field_generator() = default;
  virtual ~field_generator() = default;
  virtual std::shared_ptr<mstch_base> generate(
      t_field const* field,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t /*index*/ = 0) const;
};

class struct_generator {
 public:
  struct_generator() = default;
  virtual ~struct_generator() = default;
  virtual std::shared_ptr<mstch_base> generate(
      t_struct const* strct,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t /*index*/ = 0) const;
};

class mstch_generators {
 public:
  mstch_generators()
      : enum_value_generator_(std::make_unique<enum_value_generator>()),
        enum_generator_(std::make_unique<enum_generator>()),
        type_generator_(std::make_unique<type_generator>()),
        field_generator_(std::make_unique<field_generator>()),
        struct_generator_(std::make_unique<struct_generator>()) {}
  ~mstch_generators() = default;

  void set_enum_value_generator(std::unique_ptr<enum_value_generator> g) {
    enum_value_generator_ = std::move(g);
  }

  void set_enum_generator(std::unique_ptr<enum_generator> g) {
    enum_generator_ = std::move(g);
  }

  void set_type_generator(std::unique_ptr<type_generator> g) {
    type_generator_ = std::move(g);
  }

  void set_field_generator(std::unique_ptr<field_generator> g) {
    field_generator_ = std::move(g);
  }

  void set_struct_generator(std::unique_ptr<struct_generator> g) {
    struct_generator_ = std::move(g);
  }

  std::unique_ptr<enum_value_generator> enum_value_generator_;
  std::unique_ptr<enum_generator> enum_generator_;
  std::unique_ptr<type_generator> type_generator_;
  std::unique_ptr<field_generator> field_generator_;
  std::unique_ptr<struct_generator> struct_generator_;
};

class mstch_base : public mstch::object {
 public:
  mstch_base(
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos)
      : generators_(generators), cache_(cache), pos_(pos) {
    register_methods(
        this,
        {
            {"first?", &mstch_base::first}, {"last?", &mstch_base::last},
        });
  }
  mstch::node first() {
    return pos_ == ELEMENT_POSITION::FIRST ||
        pos_ == ELEMENT_POSITION::FIRST_AND_LAST;
  }
  mstch::node last() {
    return pos_ == ELEMENT_POSITION::LAST ||
        pos_ == ELEMENT_POSITION::FIRST_AND_LAST;
  }

  static t_type const* resolve_typedef(t_type const* type) {
    while (type->is_typedef()) {
      type = dynamic_cast<t_typedef const*>(type)->get_type();
    }
    return type;
  }

  static ELEMENT_POSITION element_position(size_t index, size_t length) {
    ELEMENT_POSITION pos = ELEMENT_POSITION::NONE;
    if (index == 0) {
      pos = ELEMENT_POSITION::FIRST;
    }
    if (index == length - 1) {
      pos = ELEMENT_POSITION::LAST;
    }
    if (length == 1) {
      pos = ELEMENT_POSITION::FIRST_AND_LAST;
    }
    return pos;
  }

  template <typename Container, typename Generator>
  static mstch::array generate_elements(
      Container const& container,
      Generator const* generator,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache) {
    mstch::array a{};
    for (size_t i = 0; i < container.size(); ++i) {
      auto pos = element_position(i, container.size());
      a.push_back(generator->generate(container[i], generators, cache, pos, i));
    }
    return a;
  }

  template <typename Container, typename Generator, typename Cache>
  static mstch::array generate_elements_cached(
      Container const& container,
      Generator const* generator,
      Cache& c,
      std::string const& id,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache) {
    mstch::array a{};
    for (size_t i = 0; i < container.size(); ++i) {
      auto pos = element_position(i, container.size());
      std::string elem_id = id + container[i]->get_name();
      if (!c.count(elem_id)) {
        c[elem_id] =
            generator->generate(container[i], generators, cache, pos, i);
      }
      a.push_back(c[elem_id]);
    }
    return a;
  }

 protected:
  std::shared_ptr<mstch_generators const> generators_;
  std::shared_ptr<mstch_cache> cache_;
  ELEMENT_POSITION const pos_;
};

class mstch_enum_value : public mstch_base {
 public:
  using node_type = t_enum_value;
  mstch_enum_value(
      t_enum_value const* enm_value,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos)
      : mstch_base(generators, cache, pos), enm_value_(enm_value) {
    register_methods(
        this,
        {
            {"enumValue:name", &mstch_enum_value::name},
            {"enumValue:value", &mstch_enum_value::value},
        });
  }
  mstch::node name() {
    return enm_value_->get_name();
  }
  mstch::node value() {
    return std::to_string(enm_value_->get_value());
  }

 protected:
  t_enum_value const* enm_value_;
};

class mstch_enum : public mstch_base {
 public:
  using node_type = t_enum;
  mstch_enum(
      t_enum const* enm,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos)
      : mstch_base(generators, cache, pos), enm_(enm) {
    register_methods(
        this,
        {
            {"enum:name", &mstch_enum::name},
            {"enum:values", &mstch_enum::values},
        });
  }
  ~mstch_enum() = default;

  mstch::node name() {
    return enm_->get_name();
  }
  mstch::node values();

 protected:
  t_enum const* enm_;
};

class mstch_type : public mstch_base {
 public:
  mstch_type(
      t_type const* type,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos)
      : mstch_base(generators, cache, pos), type_(type) {
    register_methods(
        this,
        {
            {"type:name", &mstch_type::name},
            {"type:void?", &mstch_type::is_void},
            {"type:string?", &mstch_type::is_string},
            {"type:binary?", &mstch_type::is_binary},
            {"type:bool?", &mstch_type::is_bool},
            {"type:byte?", &mstch_type::is_byte},
            {"type:i16?", &mstch_type::is_i16},
            {"type:i32?", &mstch_type::is_i32},
            {"type:i64?", &mstch_type::is_i64},
            {"type:double?", &mstch_type::is_double},
            {"type:float?", &mstch_type::is_float},
            {"type:struct?", &mstch_type::is_struct},
            {"type:enum?", &mstch_type::is_enum},
            {"type:stream?", &mstch_type::is_stream},
            {"type:service?", &mstch_type::is_service},
            {"type:base?", &mstch_type::is_base},
            {"type:container?", &mstch_type::is_container},
            {"type:list?", &mstch_type::is_list},
            {"type:set?", &mstch_type::is_set},
            {"type:map?", &mstch_type::is_map},
            {"type:typedef?", &mstch_type::is_typedef},
            {"type:struct", &mstch_type::get_struct},
            {"type:enum", &mstch_type::get_enum},
            {"type:listElemType", &mstch_type::get_list_type},
            {"type:setElemType", &mstch_type::get_set_type},
            {"type:keyType", &mstch_type::get_key_type},
            {"type:valueType", &mstch_type::get_value_type},
            {"type:typedefType", &mstch_type::get_typedef_type},
        });
  }
  virtual ~mstch_type() = default;
  mstch::node name() {
    return type_->get_name();
  }
  mstch::node is_void() {
    return type_->is_void();
  }
  mstch::node is_string() {
    return type_->is_string() && !type_->is_binary();
  }
  mstch::node is_binary() {
    return type_->is_string() && type_->is_binary();
  }
  mstch::node is_bool() {
    return type_->is_bool();
  }
  mstch::node is_byte() {
    return type_->is_byte();
  }
  mstch::node is_i16() {
    return type_->is_i16();
  }
  mstch::node is_i32() {
    return type_->is_i32();
  }
  mstch::node is_i64() {
    return type_->is_i64();
  }
  mstch::node is_double() {
    return type_->is_double();
  }
  mstch::node is_float() {
    return type_->is_float();
  }
  mstch::node is_struct() {
    return type_->is_struct() || type_->is_xception();
  }
  mstch::node is_enum() {
    return type_->is_enum();
  }
  mstch::node is_stream() {
    return type_->is_stream();
  }
  mstch::node is_service() {
    return type_->is_service();
  }
  mstch::node is_base() {
    return type_->is_base_type();
  }
  mstch::node is_container() {
    return type_->is_container();
  }
  mstch::node is_list() {
    return type_->is_list();
  }
  mstch::node is_set() {
    return type_->is_set();
  }
  mstch::node is_map() {
    return type_->is_map();
  }
  mstch::node is_typedef() {
    return type_->is_typedef();
  }
  virtual std::string get_type_namespace(t_program const*) {
    return "";
  }
  mstch::node get_struct();
  mstch::node get_enum();
  mstch::node get_list_type();
  mstch::node get_set_type();
  mstch::node get_key_type();
  mstch::node get_value_type();
  mstch::node get_typedef_type();

 protected:
  const t_type* type_;
};

class mstch_field : public mstch_base {
 public:
  mstch_field(
      t_field const* field,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      int32_t index)
      : mstch_base(generators, cache, pos), field_(field), index_(index) {
    register_methods(
        this,
        {
            {"field:name", &mstch_field::name},
            {"field:key", &mstch_field::key},
            {"field:value", &mstch_field::value},
            {"field:type", &mstch_field::type},
            {"field:index", &mstch_field::index},
            {"field:index_plus_one", &mstch_field::index_plus_one},
            {"field:required?", &mstch_field::is_required},
            {"field:optional?", &mstch_field::is_optional},
            {"field:optInReqOut?", &mstch_field::is_optInReqOut},
        });
  }
  mstch::node name() {
    return field_->get_name();
  }
  mstch::node key() {
    return std::to_string(field_->get_key());
  }
  mstch::node value();
  mstch::node type();
  mstch::node index() {
    return std::to_string(index_);
  }
  mstch::node index_plus_one() {
    return std::to_string(index_ + 1);
  }
  mstch::node is_required() {
    return field_->get_req() == t_field::e_req::T_REQUIRED;
  }
  mstch::node is_optional() {
    return field_->get_req() == t_field::e_req::T_OPTIONAL;
  }
  mstch::node is_optInReqOut() {
    return field_->get_req() == t_field::e_req::T_OPT_IN_REQ_OUT;
  }

 protected:
  t_field const* field_;
  int32_t index_;
};

class mstch_struct : public mstch_base {
 public:
  mstch_struct(
      t_struct const* strct,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos)
      : mstch_base(generators, cache, pos), strct_(strct) {
    register_methods(
        this,
        {
            {"struct:name", &mstch_struct::name},
            {"struct:fields?", &mstch_struct::has_fields},
            {"struct:fields", &mstch_struct::fields},
            {"struct:exception?", &mstch_struct::is_exception},
            {"struct:union?", &mstch_struct::is_union},
            {"struct:plain?", &mstch_struct::is_plain},
        });
  }
  mstch::node name() {
    return strct_->get_name();
  }
  mstch::node has_fields() {
    return !strct_->get_members().empty();
  }
  mstch::node fields();
  mstch::node is_exception() {
    return strct_->is_xception();
  }
  mstch::node is_union() {
    return strct_->is_union();
  }
  mstch::node is_plain() {
    return !strct_->is_xception() && !strct_->is_union();
  }

 protected:
  t_struct const* strct_;
};
