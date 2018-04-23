/*
 * Copyright 2016-present Facebook, Inc.
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
#include <array>
#include <memory>
#include <vector>

#include <thrift/compiler/generate/common.h>
#include <thrift/compiler/generate/t_mstch_generator.h>
#include <thrift/compiler/generate/t_mstch_objects.h>

namespace {

bool is_reserved_macro_name(const std::string& name) {
  static const std::unordered_set<std::string> kReservedMacros({
      // The following 3 macros are pulled from 'sys/sysmacros.h' that is
      // transitively included via '<iterator>' from libstdc++
      // https://bugzilla.redhat.com/show_bug.cgi?id=130601
      "major",
      "minor",
      "makedev",
  });

  return kReservedMacros.find(name) != kReservedMacros.end();
}

std::string const& map_find_first(
    std::map<std::string, std::string> const& m,
    std::initializer_list<char const*> keys) {
  for (auto const& key : keys) {
    auto const it = m.find(key);
    if (it != m.end()) {
      return it->second;
    }
  }
  static auto const& empty = *new std::string();
  return empty;
}

bool is_cpp_ref(const t_field* f) {
  return f->annotations_.count("cpp.ref") ||
      f->annotations_.count("cpp2.ref") ||
      (f->annotations_.count("cpp.ref_type") &&
       f->annotations_.at("cpp.ref_type") == "unique") ||
      (f->annotations_.count("cpp2.ref_type") &&
       f->annotations_.at("cpp2.ref_type") == "unique");
}

std::string const& get_cpp_type(const t_type* type) {
  return map_find_first(
      type->annotations_,
      {
          "cpp.type",
          "cpp2.type",
      });
}

std::string const& get_cpp_template(const t_type* type) {
  return map_find_first(
      type->annotations_,
      {
          "cpp.template",
          "cpp2.template",
      });
}

bool same_types(const t_type* a, const t_type* b) {
  if (!a || !b) {
    return false;
  }

  if (get_cpp_template(a) != get_cpp_template(b) ||
      get_cpp_type(a) != get_cpp_type(b)) {
    return false;
  }

  const auto* resolved_a = mstch_base::resolve_typedef(a);
  const auto* resolved_b = mstch_base::resolve_typedef(b);

  if (resolved_a->get_type_value() != resolved_b->get_type_value()) {
    return false;
  }

  switch (resolved_a->get_type_value()) {
    case t_types::TypeValue::TYPE_LIST: {
      const auto* list_a = dynamic_cast<const t_list*>(resolved_a);
      const auto* list_b = dynamic_cast<const t_list*>(resolved_b);
      return same_types(list_a->get_elem_type(), list_b->get_elem_type());
    }
    case t_types::TypeValue::TYPE_SET: {
      const auto* set_a = dynamic_cast<const t_set*>(resolved_a);
      const auto* set_b = dynamic_cast<const t_set*>(resolved_b);
      return same_types(set_a->get_elem_type(), set_b->get_elem_type());
    }
    case t_types::TypeValue::TYPE_MAP: {
      const auto* map_a = dynamic_cast<const t_map*>(resolved_a);
      const auto* map_b = dynamic_cast<const t_map*>(resolved_b);
      return same_types(map_a->get_key_type(), map_b->get_key_type()) &&
          same_types(map_a->get_val_type(), map_b->get_val_type());
    }
    default:;
  }
  return true;
}

class t_mstch_cpp2_generator : public t_mstch_generator {
 public:
  t_mstch_cpp2_generator(
      t_program* program,
      const std::map<std::string, std::string>& parsed_options,
      const std::string& /*option_string*/);

  void generate_program() override;
  static std::string get_cpp2_namespace(t_program const* program);
  static mstch::array get_namespace_array(t_program const* program);
  static mstch::node cpp_includes(t_program const* program);
  static mstch::node include_prefix(
      t_program const* program,
      std::string& include_prefix);

 private:
  void set_mstch_generators();
  void generate_constants(t_program const* program);
  void generate_structs(t_program const* program);
  void generate_service(t_service const* service);
};

class mstch_cpp2_enum : public mstch_enum {
 public:
  mstch_cpp2_enum(
      t_enum const* enm,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos)
      : mstch_enum(enm, generators, cache, pos) {
    register_methods(
        this,
        {
            {"enum:empty?", &mstch_cpp2_enum::is_empty},
            {"enum:size", &mstch_cpp2_enum::size},
            {"enum:min", &mstch_cpp2_enum::min},
            {"enum:max", &mstch_cpp2_enum::max},
            {"enum:cpp_is_unscoped", &mstch_cpp2_enum::cpp_is_unscoped},
            {"enum:cpp_enum_type", &mstch_cpp2_enum::cpp_enum_type},
            {"enum:cpp_declare_bitwise_ops",
             &mstch_cpp2_enum::cpp_declare_bitwise_ops},
            {"enum:struct_list", &mstch_cpp2_enum::struct_list},
            {"enum:has_zero", &mstch_cpp2_enum::has_zero},
        });
  }
  mstch::node is_empty() {
    return enm_->get_constants().empty();
  }
  mstch::node size() {
    return std::to_string(enm_->get_constants().size());
  }
  mstch::node min() {
    if (!enm_->get_constants().empty()) {
      auto e_min = std::min_element(
          enm_->get_constants().begin(),
          enm_->get_constants().end(),
          [](t_enum_value* a, t_enum_value* b) {
            return a->get_value() < b->get_value();
          });
      return (*e_min)->get_name();
    }
    return mstch::node();
  }
  mstch::node max() {
    if (!enm_->get_constants().empty()) {
      auto e_max = std::max_element(
          enm_->get_constants().begin(),
          enm_->get_constants().end(),
          [](t_enum_value* a, t_enum_value* b) {
            return a->get_value() < b->get_value();
          });
      return (*e_max)->get_name();
    }
    return mstch::node();
  }
  std::string const& cpp_is_unscoped_() {
    return map_find_first(
        enm_->annotations_,
        {
            "cpp2.deprecated_enum_unscoped",
            "cpp.deprecated_enum_unscoped",
        });
  }
  mstch::node cpp_is_unscoped() {
    return cpp_is_unscoped_();
  }
  mstch::node cpp_enum_type() {
    if (enm_->annotations_.count("cpp.enum_type")) {
      return enm_->annotations_.at("cpp.enum_type");
    } else if (enm_->annotations_.count("cpp2.enum_type")) {
      return enm_->annotations_.at("cpp2.enum_type");
    }
    if (!cpp_is_unscoped_().empty()) {
      return std::string("int");
    }
    return std::string();
  }
  mstch::node cpp_declare_bitwise_ops() {
    if (enm_->annotations_.count("cpp.declare_bitwise_ops")) {
      return enm_->annotations_.at("cpp.declare_bitwise_ops");
    } else if (enm_->annotations_.count("cpp2.declare_bitwise_ops")) {
      return enm_->annotations_.at("cpp2.declare_bitwise_ops");
    }
    return std::string();
  }
  mstch::node struct_list() {
    mstch::array a;
    for (const auto* strct : enm_->get_program()->get_objects()) {
      mstch::map m;
      m.emplace("struct_name", strct->get_name());
      a.push_back(m);
    }
    return a;
  }
  mstch::node has_zero() {
    auto* enm_value = enm_->find_value(0);
    if (enm_value != nullptr) {
      return generators_->enum_value_generator_->generate(
          enm_value, generators_, cache_, pos_);
    }
    return mstch::node();
  }
};

class mstch_cpp2_const_value : public mstch_const_value {
 public:
  mstch_cpp2_const_value(
      t_const_value const* const_value,
      t_const const* current_const,
      t_type const* expected_type,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      int32_t index)
      : mstch_const_value(
            const_value,
            current_const,
            expected_type,
            generators,
            cache,
            pos,
            index) {}

 private:
  virtual bool same_type_as_expected() const override {
    return const_value_->get_owner() &&
        same_types(expected_type_, const_value_->get_owner()->get_type());
  }
};

class mstch_cpp2_type : public mstch_type {
 public:
  mstch_cpp2_type(
      t_type const* type,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos)
      : mstch_type(type, generators, cache, pos) {
    register_methods(
        this,
        {
            {"type:resolves_to_base?", &mstch_cpp2_type::resolves_to_base},
            {"type:resolves_to_base_or_enum?",
             &mstch_cpp2_type::resolves_to_base_or_enum},
            {"type:resolves_to_container?",
             &mstch_cpp2_type::resolves_to_container},
            {"type:resolves_to_container_or_struct?",
             &mstch_cpp2_type::resolves_to_container_or_struct},
            {"type:resolves_to_container_or_enum?",
             &mstch_cpp2_type::resolves_to_container_or_enum},
            {"type:resolves_to_complex_return?",
             &mstch_cpp2_type::resolves_to_complex_return},
            {"type:resolves_to_stream?", &mstch_cpp2_type::resolves_to_stream},
            {"type:cpp_type", &mstch_cpp2_type::cpp_type},
            {"type:cpp_custom_type", &mstch_cpp2_type::cpp_custom_type},
            {"type:string_or_binary?", &mstch_cpp2_type::is_string_or_binary},
            {"type:cpp_template", &mstch_cpp2_type::cpp_template},
            {"type:cpp_indirection", &mstch_cpp2_type::cpp_indirection},
            {"type:non_empty_struct?", &mstch_cpp2_type::is_non_empty_struct},
            {"type:namespace_cpp2", &mstch_cpp2_type::namespace_cpp2},
            {"type:stack_arguments?", &mstch_cpp2_type::stack_arguments},
            {"type:cpp_declare_hash", &mstch_cpp2_type::cpp_declare_hash},
            {"type:cpp_declare_equal_to",
             &mstch_cpp2_type::cpp_declare_equal_to},
            {"type:optionals?", &mstch_cpp2_type::optionals},
            {"type:forward_compatibility?",
             &mstch_cpp2_type::forward_compatibility},
        });
  }
  virtual std::string get_type_namespace(t_program const* program) override {
    auto ns = program->get_namespace("cpp2");
    if (ns.empty()) {
      ns = program->get_namespace("cpp");
      if (ns.empty()) {
        ns = "cpp2";
      }
    }
    return ns;
  }
  mstch::node resolves_to_base() {
    return resolved_type_->is_base_type();
  }
  mstch::node resolves_to_base_or_enum() {
    return resolved_type_->is_base_type() || resolved_type_->is_enum();
  }
  mstch::node resolves_to_container() {
    return resolved_type_->is_container();
  }
  mstch::node resolves_to_container_or_struct() {
    return resolved_type_->is_container() || resolved_type_->is_struct() ||
        resolved_type_->is_xception();
  }
  mstch::node resolves_to_container_or_enum() {
    return resolved_type_->is_container() || resolved_type_->is_enum();
  }
  mstch::node resolves_to_stream() {
    return resolved_type_->is_pubsub_stream();
  }
  mstch::node resolves_to_complex_return() {
    if (resolved_type_->is_pubsub_stream()) {
      return false;
    }
    return resolved_type_->is_container() || resolved_type_->is_string() ||
        resolved_type_->is_struct() || resolved_type_->is_xception();
  }
  mstch::node cpp_type() {
    return get_cpp_type(type_);
  }
  mstch::node cpp_custom_type() {
    if (resolved_type_->annotations_.count("cpp.indirection") ||
        resolved_type_->annotations_.count("cpp2.indirection")) {
      // If the field is indirected, the field's cpp type is assumed
      // to be the base type.
      return std::string();
    }

    return get_cpp_type(resolved_type_);
  }
  mstch::node is_string_or_binary() {
    return resolved_type_->is_string() || resolved_type_->is_binary();
  }
  mstch::node forward_compatibility() {
    return resolved_type_->annotations_.count("forward_compatibility") != 0;
  }
  mstch::node cpp_template() {
    return get_cpp_template(type_);
  }
  mstch::node cpp_indirection() {
    if (resolved_type_->annotations_.count("cpp.indirection")) {
      return resolved_type_->annotations_.at("cpp.indirection");
    } else if (resolved_type_->annotations_.count("cpp2.indirection")) {
      return resolved_type_->annotations_.at("cpp2.indirection");
    }
    return std::string();
  }
  mstch::node cpp_declare_hash() {
    return resolved_type_->annotations_.count("cpp.declare_hash") ||
        resolved_type_->annotations_.count("cpp2.declare_hash");
  }
  mstch::node cpp_declare_equal_to() {
    return resolved_type_->annotations_.count("cpp.declare_equal_to") ||
        resolved_type_->annotations_.count("cpp2.declare_equal_to");
  }
  mstch::node is_non_empty_struct() {
    if (resolved_type_->is_struct() || resolved_type_->is_xception()) {
      auto as_struct = dynamic_cast<t_struct const*>(resolved_type_);
      return !as_struct->get_members().empty();
    }
    return false;
  }
  mstch::node namespace_cpp2() {
    return t_mstch_cpp2_generator::get_namespace_array(type_->get_program());
  }
  mstch::node stack_arguments() {
    return cache_->parsed_options_.count("stack_arguments") != 0;
  }
  mstch::node optionals() {
    return cache_->parsed_options_.count("optionals") != 0;
  }
};

class mstch_cpp2_field : public mstch_field {
 public:
  mstch_cpp2_field(
      t_field const* field,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos,
      int32_t index)
      : mstch_field(field, generators, cache, pos, index) {
    register_methods(
        this,
        {
            {"field:cpp_ref?", &mstch_cpp2_field::cpp_ref},
            {"field:cpp_ref_unique?", &mstch_cpp2_field::cpp_ref_unique},
            {"field:cpp_ref_shared?", &mstch_cpp2_field::cpp_ref_shared},
            {"field:cpp_ref_shared_const?",
             &mstch_cpp2_field::cpp_ref_shared_const},
            {"field:enum_has_value", &mstch_cpp2_field::enum_has_value},
            {"field:optionals?", &mstch_cpp2_field::optionals},
            {"field:terse_writes?", &mstch_cpp2_field::terse_writes},
        });
  }
  mstch::node cpp_ref() {
    return has_annotation("cpp.ref") || has_annotation("cpp2.ref") ||
        has_annotation("cpp.ref_type") || has_annotation("cpp2.ref_type");
  }
  mstch::node cpp_ref_unique() {
    return has_annotation("cpp.ref") || has_annotation("cpp2.ref") ||
        get_annotation("cpp.ref_type") == "unique" ||
        get_annotation("cpp2.ref_type") == "unique";
  }
  mstch::node cpp_ref_shared() {
    return get_annotation("cpp.ref_type") == "shared" ||
        get_annotation("cpp2.ref_type") == "shared";
  }
  mstch::node cpp_ref_shared_const() {
    return get_annotation("cpp.ref_type") == "shared_const" ||
        get_annotation("cpp2.ref_type") == "shared_const";
  }
  mstch::node enum_has_value() {
    if (field_->get_type()->is_enum()) {
      auto const* enm = dynamic_cast<t_enum const*>(field_->get_type());
      auto const* const_value = field_->get_value();
      using cv = t_const_value::t_const_value_type;
      if (const_value->get_type() == cv::CV_INTEGER) {
        auto* enm_value = enm->find_value(const_value->get_integer());
        if (enm_value != nullptr) {
          return generators_->enum_value_generator_->generate(
              enm_value, generators_, cache_, pos_);
        }
      }
    }
    return mstch::node();
  }
  mstch::node optionals() {
    return field_->get_req() == t_field::e_req::T_OPTIONAL &&
        cache_->parsed_options_.count("optionals");
  }
  mstch::node terse_writes() {
    // Add terse writes for unqualified fields when comparison is cheap:
    // (e.g. i32/i64, empty strings/list/map)
    auto t = resolve_typedef(field_->get_type());
    return cache_->parsed_options_.count("terse_writes") != 0 &&
        field_->get_req() != t_field::e_req::T_OPTIONAL &&
        field_->get_req() != t_field::e_req::T_REQUIRED &&
        (is_cpp_ref(field_) || (!t->is_struct() && !t->is_xception()));
  }
};

class mstch_cpp2_struct : public mstch_struct {
 public:
  mstch_cpp2_struct(
      t_struct const* strct,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos)
      : mstch_struct(strct, generators, cache, pos) {
    register_methods(
        this,
        {
            {"struct:getters_setters?", &mstch_cpp2_struct::getters_setters},
            {"struct:base_field_or_struct?",
             &mstch_cpp2_struct::has_base_field_or_struct},
            {"struct:filtered_fields", &mstch_cpp2_struct::filtered_fields},
            {"struct:is_struct_orderable?",
             &mstch_cpp2_struct::is_struct_orderable},
            {"struct:fields_contain_cpp_ref?", &mstch_cpp2_struct::has_cpp_ref},
            {"struct:cpp_methods", &mstch_cpp2_struct::cpp_methods},
            {"struct:cpp_declare_hash", &mstch_cpp2_struct::cpp_declare_hash},
            {"struct:cpp_declare_equal_to",
             &mstch_cpp2_struct::cpp_declare_equal_to},
            {"struct:cpp_noncopyable", &mstch_cpp2_struct::cpp_noncopyable},
            {"struct:cpp_noncomparable", &mstch_cpp2_struct::cpp_noncomparable},
            {"struct:cpp_noexcept_move_ctor",
             &mstch_cpp2_struct::cpp_noexcept_move_ctor},
            {"struct:virtual", &mstch_cpp2_struct::cpp_virtual},
            {"struct:message", &mstch_cpp2_struct::message},
            {"struct:struct_list", &mstch_cpp2_struct::struct_list},
            {"struct:non_req_fields?", &mstch_cpp2_struct::has_non_req_fields},
            {"struct:isset_fields?", &mstch_cpp2_struct::has_isset_fields},
            {"struct:isset_fields", &mstch_cpp2_struct::isset_fields},
            {"struct:optionals?", &mstch_cpp2_struct::optionals},
            {"struct:is_large?", &mstch_cpp2_struct::is_large},
        });
  }
  mstch::node getters_setters() {
    for (auto const* field : strct_->get_members()) {
      auto const* resolved_typedef = resolve_typedef(field->get_type());
      if (resolved_typedef->is_base_type() || resolved_typedef->is_enum() ||
          resolved_typedef->is_struct() ||
          field->get_req() == t_field::e_req::T_OPTIONAL) {
        return true;
      }
    }
    return false;
  }
  mstch::node has_base_field_or_struct() {
    for (auto const* field : strct_->get_members()) {
      auto const* resolved_typedef = resolve_typedef(field->get_type());
      if (resolved_typedef->is_base_type() || resolved_typedef->is_enum() ||
          resolved_typedef->is_struct() ||
          field->annotations_.count("cpp.ref") ||
          field->annotations_.count("cpp2.ref") ||
          field->annotations_.count("cpp.ref_type") ||
          field->annotations_.count("cpp2.ref_type") ||
          field->get_req() == t_field::e_req::T_OPTIONAL) {
        return true;
      }
    }
    return false;
  }
  mstch::node filtered_fields() {
    auto has_annotation = [](t_field const* field, std::string const& name) {
      return field->annotations_.count(name);
    };
    // Filter fields according to the following criteria:
    // Get all base_types but strings (empty and non-empty)
    // Get all non empty strings
    // Get all non empty containers
    // Get all enums
    // Get all containers with references
    // Remove all optional fields when optionals flag is enabled
    std::vector<t_field const*> filtered_fields;
    for (auto const* field : strct_->get_members()) {
      if (field->get_req() == t_field::e_req::T_OPTIONAL &&
          cache_->parsed_options_.count("optionals")) {
        continue;
      }
      const t_type* type = resolve_typedef(field->get_type());
      if ((type->is_base_type() && !type->is_string()) ||
          (type->is_string() && field->get_value() != nullptr) ||
          (type->is_container() && field->get_value() != nullptr &&
           !field->get_value()->is_empty()) ||
          (type->is_struct() && field->get_value() &&
           !field->get_value()->is_empty()) ||
          type->is_enum() ||
          (type->is_container() &&
           (has_annotation(field, "cpp.ref") ||
            has_annotation(field, "cpp2.ref") ||
            has_annotation(field, "cpp.ref_type") ||
            has_annotation(field, "cpp2.ref_type")))) {
        filtered_fields.push_back(field);
      }
    }
    return generate_elements(
        filtered_fields,
        generators_->field_generator_.get(),
        generators_,
        cache_);
  }
  bool is_orderable(t_type const* type) {
    if (type->is_base_type()) {
      return true;
    }
    if (type->is_enum()) {
      return true;
    }
    if (type->is_struct() || type->is_xception()) {
      for (auto const* f : dynamic_cast<t_struct const*>(type)->get_members()) {
        if (f->get_req() == t_field::e_req::T_OPTIONAL ||
            f->get_type()->annotations_.count("cpp.template") ||
            f->get_type()->annotations_.count("cpp2.template") ||
            !is_orderable(f->get_type())) {
          return false;
        }
      }
      return true;
    }
    if (type->is_list()) {
      return is_orderable(dynamic_cast<t_list const*>(type)->get_elem_type());
    }
    if (type->is_set()) {
      return is_orderable(dynamic_cast<t_set const*>(type)->get_elem_type());
    }
    if (type->is_map()) {
      return is_orderable(dynamic_cast<t_map const*>(type)->get_key_type()) &&
          is_orderable(dynamic_cast<t_map const*>(type)->get_val_type());
    }
    return false;
  }
  mstch::node is_struct_orderable() {
    return is_orderable(strct_) &&
        !strct_->annotations_.count("no_default_comparators");
  }
  mstch::node has_cpp_ref() {
    for (auto const* f : strct_->get_members()) {
      if (is_cpp_ref(f)) {
        return true;
      }
    }
    return false;
  }
  mstch::node cpp_methods() {
    if (strct_->annotations_.count("cpp.methods")) {
      return strct_->annotations_.at("cpp.methods");
    } else if (strct_->annotations_.count("cpp2.methods")) {
      return strct_->annotations_.at("cpp2.methods");
    }
    return std::string();
  }
  mstch::node cpp_declare_hash() {
    return strct_->annotations_.count("cpp.declare_hash") ||
        strct_->annotations_.count("cpp2.declare_hash");
  }
  mstch::node cpp_declare_equal_to() {
    return strct_->annotations_.count("cpp.declare_equal_to") ||
        strct_->annotations_.count("cpp2.declare_equal_to");
  }
  mstch::node cpp_noncopyable() {
    return bool(strct_->annotations_.count("cpp2.noncopyable"));
  }
  mstch::node cpp_noncomparable() {
    return bool(strct_->annotations_.count("cpp2.noncomparable"));
  }
  mstch::node cpp_noexcept_move_ctor() {
    return strct_->annotations_.count("cpp.noexcept_move_ctor") ||
        strct_->annotations_.count("cpp2.noexcept_move_ctor");
  }
  mstch::node cpp_virtual() {
    return strct_->annotations_.count("cpp.virtual") > 0 ||
        strct_->annotations_.count("cpp2.virtual") > 0;
  }
  mstch::node message() {
    if (strct_->annotations_.count("message")) {
      return strct_->annotations_.at("message");
    }
    return std::string();
  }
  mstch::node struct_list() {
    mstch::array a;
    for (const auto* strct : strct_->get_program()->get_objects()) {
      mstch::map m;
      m.emplace("struct_name", strct->get_name());
      a.push_back(m);
    }
    return a;
  }
  mstch::node has_non_req_fields() {
    return std::any_of(
        strct_->get_members().begin(),
        strct_->get_members().end(),
        [](const auto* field) {
          return field->get_req() != t_field::e_req::T_REQUIRED;
        });
  }
  mstch::node has_isset_fields() {
    for (const auto* field : strct_->get_members()) {
      if (field->get_req() != t_field::e_req::T_REQUIRED &&
          !field->annotations_.count("cpp.ref") &&
          !field->annotations_.count("cpp2.ref") &&
          !field->annotations_.count("cpp.ref_type") &&
          !field->annotations_.count("cpp2.ref_type")) {
        return true;
      }
    }
    return false;
  }
  mstch::node isset_fields() {
    std::vector<t_field const*> fields;
    for (const auto* field : strct_->get_members()) {
      if (field->get_req() != t_field::e_req::T_REQUIRED &&
          !field->annotations_.count("cpp.ref") &&
          !field->annotations_.count("cpp2.ref") &&
          !field->annotations_.count("cpp.ref_type") &&
          !field->annotations_.count("cpp2.ref_type")) {
        fields.push_back(field);
      }
    }
    if (fields.empty()) {
      return mstch::node();
    }
    return generate_elements(
        fields, generators_->field_generator_.get(), generators_, cache_);
  }
  mstch::node optionals() {
    return cache_->parsed_options_.count("optionals") != 0;
  }
  mstch::node is_large() {
    // Outline constructors and destructors if the struct has
    // enough members and at least one has a non-trivial destructor
    // (involving at least a branch and a likely deallocation).
    // TODO(ott): Support unions.
    constexpr size_t kLargeStructThreshold = 4;
    if (strct_->get_members().size() <= kLargeStructThreshold) {
      return false;
    }
    for (auto const* field : strct_->get_members()) {
      auto const* resolved_typedef = resolve_typedef(field->get_type());
      if (is_cpp_ref(field) || resolved_typedef->is_string() ||
          resolved_typedef->is_container()) {
        return true;
      }
    }
    return false;
  }
};

class mstch_cpp2_function : public mstch_function {
 public:
  mstch_cpp2_function(
      t_function const* function,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos)
      : mstch_function(function, generators, cache, pos) {
    register_methods(
        this,
        {
            {"function:cache_key?", &mstch_cpp2_function::has_cache_key},
            {"function:cache_key_id", &mstch_cpp2_function::cache_key_id},
        });
  }
  mstch::node has_cache_key() {
    return get_cache_key_arg() != nullptr;
  }
  mstch::node cache_key_id() {
    auto field = get_cache_key_arg();
    return field ? field->get_key() : 0;
  }

 private:
  const t_field* get_cache_key_arg() const {
    const t_field* result = nullptr;
    for (auto* arg : function_->get_arglist()->get_members()) {
      auto iter = arg->annotations_.find("cpp.cache");
      if (iter != arg->annotations_.end()) {
        if (!arg->get_type()->is_string()) {
          printf(
              "Cache annotation is only allowed on string types, "
              "got '%s' for argument '%s' in function '%s' at line %d",
              arg->get_type()->get_name().data(),
              arg->get_name().data(),
              function_->get_name().data(),
              arg->get_lineno());
          exit(1);
        }
        if (result) {
          printf(
              "Multiple cache annotations are not allowed! (See argument '%s' "
              "in function '%s' at line %d)",
              arg->get_name().data(),
              function_->get_name().data(),
              arg->get_lineno());
          exit(1);
        }
        result = arg;
      }
    }
    return result;
  }
};

class mstch_cpp2_service : public mstch_service {
 public:
  mstch_cpp2_service(
      t_service const* service,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos)
      : mstch_service(service, generators, cache, pos) {
    register_methods(
        this,
        {
            {"service:program_name", &mstch_cpp2_service::program_name},
            {"service:include_prefix", &mstch_cpp2_service::include_prefix},
            {"service:thrift_includes", &mstch_cpp2_service::thrift_includes},
            {"service:namespace_cpp2", &mstch_cpp2_service::namespace_cpp2},
            {"service:oneway_functions", &mstch_cpp2_service::oneway_functions},
            {"service:oneways?", &mstch_cpp2_service::has_oneway},
            {"service:cache_keys?", &mstch_cpp2_service::has_cache_keys},
            {"service:cpp_includes", &mstch_cpp2_service::cpp_includes},
            {"service:frozen2?", &mstch_cpp2_service::frozen2},
        });
  }
  virtual std::string get_service_namespace(t_program const* program) override {
    return t_mstch_cpp2_generator::get_cpp2_namespace(program);
  }
  mstch::node program_name() {
    return service_->get_program()->get_name();
  }
  mstch::node cpp_includes() {
    return t_mstch_cpp2_generator::cpp_includes(service_->get_program());
  }
  mstch::node include_prefix() {
    return t_mstch_cpp2_generator::include_prefix(
        service_->get_program(), cache_->parsed_options_["include_prefix"]);
  }
  mstch::node thrift_includes() {
    mstch::array a{};
    for (auto const* program : service_->get_program()->get_includes()) {
      const auto& program_id = program->get_path();
      if (!cache_->programs_.count(program_id)) {
        cache_->programs_[program_id] =
            generators_->program_generator_->generate(
                program, generators_, cache_);
      }
      a.push_back(cache_->programs_[program_id]);
    }
    return a;
  }
  mstch::node namespace_cpp2() {
    return t_mstch_cpp2_generator::get_namespace_array(service_->get_program());
  }
  mstch::node oneway_functions() {
    std::vector<t_function const*> oneway_functions;
    for (auto const* function : service_->get_functions()) {
      if (function->is_oneway()) {
        oneway_functions.push_back(function);
      }
    }
    return generate_elements(
        oneway_functions,
        generators_->function_generator_.get(),
        generators_,
        cache_);
  }
  mstch::node has_oneway() {
    for (auto const* function : service_->get_functions()) {
      if (function->is_oneway()) {
        return true;
      }
    }
    return false;
  }
  mstch::node has_cache_keys() {
    for (auto const* function : service_->get_functions()) {
      for (auto const* arg : function->get_arglist()->get_members()) {
        if (arg->annotations_.find("cpp.cache") != arg->annotations_.end()) {
          return true;
        }
      }
    }
    return false;
  }
  mstch::node frozen2() {
    return cache_->parsed_options_.count("frozen2") != 0;
  }
};

class mstch_cpp2_const : public mstch_const {
 public:
  mstch_cpp2_const(
      t_const const* cnst,
      t_const const* current_const,
      t_type const* expected_type,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos,
      int32_t index)
      : mstch_const(
            cnst,
            current_const,
            expected_type,
            generators,
            cache,
            pos,
            index) {
    register_methods(
        this,
        {
            {"constant:enum_value?", &mstch_cpp2_const::has_enum_value},
            {"constant:enum_value", &mstch_cpp2_const::enum_value},
        });
  }
  mstch::node has_enum_value() {
    if (cnst_->get_type()->is_enum()) {
      auto const* enm = static_cast<t_enum const*>(cnst_->get_type());
      if (enm->find_value(cnst_->get_value()->get_integer())) {
        return true;
      }
    }
    return false;
  }
  mstch::node enum_value() {
    if (cnst_->get_type()->is_enum()) {
      auto const* enm = static_cast<t_enum const*>(cnst_->get_type());
      auto const* enm_val = enm->find_value(cnst_->get_value()->get_integer());
      if (enm_val) {
        return enm_val->get_name();
      } else {
        return std::to_string(cnst_->get_value()->get_integer());
      }
    }
    return mstch::node();
  }
};

class mstch_cpp2_program : public mstch_program {
 public:
  mstch_cpp2_program(
      t_program const* program,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos)
      : mstch_program(program, generators, cache, pos) {
    register_methods(
        this,
        {
            {"program:cpp_includes", &mstch_cpp2_program::cpp_includes},
            {"program:namespace_cpp2", &mstch_cpp2_program::namespace_cpp2},
            {"program:include_prefix", &mstch_cpp2_program::include_prefix},
            {"program:enums?", &mstch_cpp2_program::has_enums},
            {"program:typedefs?", &mstch_cpp2_program::has_typedefs},
            {"program:cpp_declare_hash?",
             &mstch_cpp2_program::cpp_declare_hash},
            {"program:thrift_includes", &mstch_cpp2_program::thrift_includes},
            {"program:frozen?", &mstch_cpp2_program::frozen},
            {"program:frozen_packed?", &mstch_cpp2_program::frozen_packed},
            {"program:frozen2?", &mstch_cpp2_program::frozen2},
            {"program:indirection?", &mstch_cpp2_program::has_indirection},
            {"program:json?", &mstch_cpp2_program::json},
            {"program:optionals?", &mstch_cpp2_program::optionals},
            {"program:colliding_macro_names",
             &mstch_cpp2_program::colliding_macro_names},
        });
  }
  virtual std::string get_program_namespace(t_program const* program) override {
    return t_mstch_cpp2_generator::get_cpp2_namespace(program);
  }

  mstch::node namespace_cpp2() {
    return t_mstch_cpp2_generator::get_namespace_array(program_);
  }
  mstch::node cpp_includes() {
    return t_mstch_cpp2_generator::cpp_includes(program_);
  }
  mstch::node include_prefix() {
    return t_mstch_cpp2_generator::include_prefix(
        program_, cache_->parsed_options_["include_prefix"]);
  }
  mstch::node has_enums() {
    return !program_->get_enums().empty();
  }
  mstch::node has_typedefs() {
    return !program_->get_typedefs().empty();
  }
  mstch::node cpp_declare_hash() {
    bool cpp_declare_in_structs = std::any_of(
        program_->get_structs().begin(),
        program_->get_structs().end(),
        [](const auto* strct) {
          return strct->annotations_.count("cpp.declare_hash") ||
              strct->annotations_.count("cpp2.declare_hash");
        });
    bool cpp_declare_in_typedefs = std::any_of(
        program_->get_typedefs().begin(),
        program_->get_typedefs().end(),
        [](const auto* typedf) {
          return typedf->get_type()->annotations_.count("cpp.declare_hash") ||
              typedf->get_type()->annotations_.count("cpp2.declare_hash");
        });
    return cpp_declare_in_structs || cpp_declare_in_typedefs;
  }
  mstch::node thrift_includes() {
    mstch::array a{};
    for (auto const* program : program_->get_includes()) {
      const auto& program_id = program->get_path();
      if (!cache_->programs_.count(program_id)) {
        cache_->programs_[program_id] =
            generators_->program_generator_->generate(
                program, generators_, cache_);
      }
      a.push_back(cache_->programs_[program_id]);
    }
    return a;
  }
  mstch::node frozen() {
    return cache_->parsed_options_.count("frozen") != 0;
  }
  mstch::node frozen_packed() {
    auto iter = cache_->parsed_options_.find("frozen");
    return iter != cache_->parsed_options_.end() && iter->second == "packed";
  }
  mstch::node frozen2() {
    return cache_->parsed_options_.count("frozen2") != 0;
  }
  mstch::node has_indirection() {
    for (auto const* typedf : program_->get_typedefs()) {
      if (typedf->get_type()->annotations_.count("cpp.indirection")) {
        return true;
      }
    }
    return false;
  }
  mstch::node json() {
    return cache_->parsed_options_.count("json") != 0;
  }
  mstch::node optionals() {
    return cache_->parsed_options_.count("optionals") != 0;
  }
  mstch::node colliding_macro_names() {
    if (colliding_macro_names_) {
      return *colliding_macro_names_;
    }

    std::set<std::string> names;
    for (const auto* s : program_->get_structs()) {
      if (is_reserved_macro_name(s->get_name())) {
        names.emplace(s->get_name());
      }

      for (const auto* field : s->get_members()) {
        if (is_reserved_macro_name(field->get_name())) {
          names.emplace(field->get_name());
        }
      }
    }

    for (const auto* t : program_->get_typedefs()) {
      if (is_reserved_macro_name(t->get_symbolic())) {
        names.emplace(t->get_symbolic());
      }
    }

    colliding_macro_names_ = std::make_unique<mstch::array>();
    for (const auto& name : names) {
      mstch::map m;
      m.emplace("macro_name", name);
      colliding_macro_names_->emplace_back(std::move(m));
    }
    return *colliding_macro_names_;
  }

 private:
  std::unique_ptr<mstch::array> colliding_macro_names_;
};

class enum_cpp2_generator : public enum_generator {
 public:
  enum_cpp2_generator() = default;
  virtual ~enum_cpp2_generator() = default;
  virtual std::shared_ptr<mstch_base> generate(
      t_enum const* enm,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t /*index*/ = 0) const override {
    return std::make_shared<mstch_cpp2_enum>(enm, generators, cache, pos);
  }
};

class type_cpp2_generator : public type_generator {
 public:
  type_cpp2_generator() = default;
  virtual ~type_cpp2_generator() = default;
  virtual std::shared_ptr<mstch_base> generate(
      t_type const* type,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t /*index*/ = 0) const override {
    return std::make_shared<mstch_cpp2_type>(type, generators, cache, pos);
  }
};

class field_cpp2_generator : public field_generator {
 public:
  field_cpp2_generator() = default;
  virtual ~field_cpp2_generator() = default;
  virtual std::shared_ptr<mstch_base> generate(
      t_field const* field,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t index = 0) const override {
    return std::make_shared<mstch_cpp2_field>(
        field, generators, cache, pos, index);
  }
};

class function_cpp2_generator : public function_generator {
 public:
  function_cpp2_generator() = default;
  virtual ~function_cpp2_generator() = default;
  virtual std::shared_ptr<mstch_base> generate(
      t_function const* function,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t /*index*/ = 0) const override {
    return std::make_shared<mstch_cpp2_function>(
        function, generators, cache, pos);
  }
};

class struct_cpp2_generator : public struct_generator {
 public:
  struct_cpp2_generator() = default;
  virtual ~struct_cpp2_generator() = default;
  virtual std::shared_ptr<mstch_base> generate(
      t_struct const* strct,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t /*index*/ = 0) const override {
    return std::make_shared<mstch_cpp2_struct>(strct, generators, cache, pos);
  }
};

class service_cpp2_generator : public service_generator {
 public:
  service_cpp2_generator() = default;
  virtual ~service_cpp2_generator() = default;
  virtual std::shared_ptr<mstch_base> generate(
      t_service const* service,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t /*index*/ = 0) const override {
    return std::make_shared<mstch_cpp2_service>(
        service, generators, cache, pos);
  }
};

class const_cpp2_generator : public const_generator {
 public:
  const_cpp2_generator() = default;
  virtual ~const_cpp2_generator() = default;
  virtual std::shared_ptr<mstch_base> generate(
      t_const const* cnst,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t index = 0,
      t_const const* current_const = nullptr,
      t_type const* expected_type = nullptr) const override {
    return std::make_shared<mstch_cpp2_const>(
        cnst, current_const, expected_type, generators, cache, pos, index);
  }
};

class const_value_cpp2_generator : public const_value_generator {
 public:
  const_value_cpp2_generator() = default;
  virtual ~const_value_cpp2_generator() = default;
  virtual std::shared_ptr<mstch_base> generate(
      t_const_value const* const_value,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t index = 0,
      t_const const* current_const = nullptr,
      t_type const* expected_type = nullptr) const override {
    return std::make_shared<mstch_cpp2_const_value>(
        const_value,
        current_const,
        expected_type,
        generators,
        cache,
        pos,
        index);
  }
};

class program_cpp2_generator : public program_generator {
 public:
  program_cpp2_generator() = default;
  virtual ~program_cpp2_generator() = default;
  virtual std::shared_ptr<mstch_base> generate(
      t_program const* program,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t /*index*/ = 0) const override {
    return std::make_shared<mstch_cpp2_program>(
        program, generators, cache, pos);
  }
};

t_mstch_cpp2_generator::t_mstch_cpp2_generator(
    t_program* program,
    const std::map<std::string, std::string>& parsed_options,
    const std::string& /*option_string*/)
    : t_mstch_generator(program, "cpp2", parsed_options, true) {
  out_dir_base_ = "gen-cpp2";
}

void t_mstch_cpp2_generator::generate_program() {
  // disable mstch escaping
  mstch::config::escape = [](const std::string& s) { return s; };

  auto const* program = get_program();
  set_mstch_generators();

  generate_structs(program);
  generate_constants(program);
  for (const auto* service : program->get_services()) {
    generate_service(service);
  }
}

void t_mstch_cpp2_generator::set_mstch_generators() {
  generators_->set_enum_generator(std::make_unique<enum_cpp2_generator>());
  generators_->set_type_generator(std::make_unique<type_cpp2_generator>());
  generators_->set_field_generator(std::make_unique<field_cpp2_generator>());
  generators_->set_function_generator(
      std::make_unique<function_cpp2_generator>());
  generators_->set_struct_generator(std::make_unique<struct_cpp2_generator>());
  generators_->set_service_generator(
      std::make_unique<service_cpp2_generator>());
  generators_->set_const_generator(std::make_unique<const_cpp2_generator>());
  generators_->set_const_value_generator(
      std::make_unique<const_value_cpp2_generator>());
  generators_->set_program_generator(
      std::make_unique<program_cpp2_generator>());
}

void t_mstch_cpp2_generator::generate_constants(t_program const* program) {
  std::string name = program->get_name();
  const auto& id = program->get_path();
  if (!cache_->programs_.count(id)) {
    cache_->programs_[id] =
        generators_->program_generator_->generate(program, generators_, cache_);
  }
  render_to_file(
      cache_->programs_[id], "module_constants.h", name + "_constants.h");
  render_to_file(
      cache_->programs_[id], "module_constants.cpp", name + "_constants.cpp");
}

void t_mstch_cpp2_generator::generate_structs(t_program const* program) {
  auto name = program->get_name();
  const auto& id = program->get_path();
  if (!cache_->programs_.count(id)) {
    cache_->programs_[id] =
        generators_->program_generator_->generate(program, generators_, cache_);
  }
  render_to_file(cache_->programs_[id], "module_data.h", name + "_data.h");
  render_to_file(cache_->programs_[id], "module_data.cpp", name + "_data.cpp");
  render_to_file(cache_->programs_[id], "module_types.h", name + "_types.h");
  render_to_file(
      cache_->programs_[id], "module_types.tcc", name + "_types.tcc");
  render_to_file(
      cache_->programs_[id], "module_types.cpp", name + "_types.cpp");
  render_to_file(
      cache_->programs_[id],
      "module_types_custom_protocol.h",
      name + "_types_custom_protocol.h");
  if (cache_->parsed_options_.count("frozen2")) {
    render_to_file(
        cache_->programs_[id], "module_layouts.h", name + "_layouts.h");
    render_to_file(
        cache_->programs_[id], "module_layouts.cpp", name + "_layouts.cpp");
  }
}

void t_mstch_cpp2_generator::generate_service(t_service const* service) {
  const auto& id = get_program()->get_path();
  std::string name = service->get_name();
  std::string service_id = id + name;
  if (!cache_->services_.count(service_id)) {
    cache_->services_[service_id] =
        generators_->service_generator_->generate(service, generators_, cache_);
  }
  render_to_file(
      cache_->services_[service_id],
      "ServiceAsyncClient.h",
      name + "AsyncClient.h");
  render_to_file(cache_->services_[service_id], "service.cpp", name + ".cpp");
  render_to_file(cache_->services_[service_id], "service.h", name + ".h");
  render_to_file(cache_->services_[service_id], "service.tcc", name + ".tcc");
  render_to_file(
      cache_->services_[service_id],
      "service_client.cpp",
      name + "_client.cpp");
  render_to_file(
      cache_->services_[service_id],
      "types_custom_protocol.h",
      name + "_custom_protocol.h");

  std::vector<std::array<std::string, 3>> protocols = {
      {{"binary", "BinaryProtocol", "T_BINARY_PROTOCOL"}},
      {{"compact", "CompactProtocol", "T_COMPACT_PROTOCOL"}},
  };
  if (cache_->parsed_options_.count("frozen2")) {
    protocols.push_back({{"frozen", "Frozen2Protocol", "T_FROZEN2_PROTOCOL"}});
  }
  for (const auto& protocol : protocols) {
    render_to_file(
        cache_->services_[service_id],
        "service_processmap_protocol.cpp",
        name + "_processmap_" + protocol.at(0) + ".cpp");
  }
}

std::string t_mstch_cpp2_generator::get_cpp2_namespace(
    t_program const* program) {
  auto ns = program->get_namespace("cpp2");
  if (ns.empty()) {
    ns = program->get_namespace("cpp");
    if (ns.empty()) {
      ns = "cpp2";
    }
  }
  return ns;
}

mstch::array t_mstch_cpp2_generator::get_namespace_array(
    t_program const* program) {
  std::vector<std::string> v;

  auto ns = program->get_namespace("cpp2");
  if (!ns.empty()) {
    v = split_namespace(ns);
  } else {
    ns = program->get_namespace("cpp");
    if (!ns.empty()) {
      v = split_namespace(ns);
    }
    v.push_back("cpp2");
  }
  mstch::array a;
  for (auto it = v.begin(); it != v.end(); ++it) {
    mstch::map m;
    m.emplace("namespace:name", *it);
    a.push_back(m);
  }
  for (auto itr = a.begin(); itr != a.end(); ++itr) {
    boost::get<mstch::map>(*itr).emplace("first?", itr == a.begin());
    boost::get<mstch::map>(*itr).emplace("last?", std::next(itr) == a.end());
  }
  return a;
}

mstch::node t_mstch_cpp2_generator::cpp_includes(t_program const* program) {
  mstch::array a{};
  for (auto include : program->get_cpp_includes()) {
    mstch::map cpp_include;
    if (include.at(0) != '<') {
      include = "\"" + include + "\"";
    }
    cpp_include.emplace("cpp_include", std::string(include));
    a.push_back(cpp_include);
  }
  return a;
}

mstch::node t_mstch_cpp2_generator::include_prefix(
    t_program const* program,
    std::string& include_prefix) {
  auto prefix = program->get_include_prefix();
  if (prefix.empty()) {
    if (include_prefix.empty()) {
      return prefix;
    } else {
      return include_prefix + "/gen-cpp2/";
    }
  }
  if (prefix[0] == '/') {
    return include_prefix + "/gen-cpp2/";
  }
  return prefix + "gen-cpp2/";
}
} // namespace

THRIFT_REGISTER_GENERATOR(mstch_cpp2, "cpp2", "");
