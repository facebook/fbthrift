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

class t_mstch_cpp2_generator : public t_mstch_generator {
 public:
  t_mstch_cpp2_generator(
      t_program* program,
      const std::map<std::string, std::string>& parsed_options,
      const std::string& /*option_string*/);

  void generate_program() override;

 private:
  void set_mstch_generators();
  void generate_constants(t_program const* program);
  void generate_structs(t_program const* program);
  void generate_service(t_service const* service);
  std::string get_cpp2_namespace(t_program const* program);
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
  mstch::node cpp_enum_type() {
    if (enm_->annotations_.count("cpp.enum_type")) {
      return enm_->annotations_.at("cpp.enum_type");
    } else if (enm_->annotations_.count("cpp2.enum_type")) {
      return enm_->annotations_.at("cpp2.enum_type");
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
            {"type:cpp_type", &mstch_cpp2_type::cpp_type},
            {"type:cpp_typedef_type", &mstch_cpp2_type::cpp_typedef_type},
            {"type:string_or_binary?", &mstch_cpp2_type::is_string_or_binary},
            {"type:cpp_template", &mstch_cpp2_type::cpp_template},
            {"type:cpp_indirection", &mstch_cpp2_type::cpp_indirection},
            {"type:non_empty_struct?", &mstch_cpp2_type::is_non_empty_struct},
            {"type:namespace_cpp2", &mstch_cpp2_type::namespace_cpp2},
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
  mstch::node resolves_to_complex_return() {
    return resolved_type_->is_container() || resolved_type_->is_string() ||
        resolved_type_->is_struct() || resolved_type_->is_xception();
  }
  mstch::node cpp_type() {
    if (type_->annotations_.count("cpp.type")) {
      return type_->annotations_.at("cpp.type");
    } else if (type_->annotations_.count("cpp2.type")) {
      return type_->annotations_.at("cpp2.type");
    }
    return std::string();
  }
  mstch::node cpp_typedef_type() {
    if (type_->is_typedef()) {
      if (resolved_type_->annotations_.count("cpp.type")) {
        return resolved_type_->annotations_.at("cpp.type");
      } else if (resolved_type_->annotations_.count("cpp2.type")) {
        return resolved_type_->annotations_.at("cpp2.type");
      }
    }
    return std::string();
  }
  mstch::node is_string_or_binary() {
    return resolved_type_->is_string() || resolved_type_->is_binary();
  }
  mstch::node cpp_template() {
    if (type_->annotations_.count("cpp.template")) {
      return type_->annotations_.at("cpp.template");
    } else if (type_->annotations_.count("cpp2.template")) {
      return type_->annotations_.at("cpp2.template");
    }
    return std::string();
  }
  mstch::node cpp_indirection() {
    if (resolved_type_->annotations_.count("cpp.indirection")) {
      return resolved_type_->annotations_.at("cpp.indirection");
    } else if (resolved_type_->annotations_.count("cpp2.indirection")) {
      return resolved_type_->annotations_.at("cpp2.indirection");
    }
    return std::string();
  }
  mstch::node is_non_empty_struct() {
    if (resolved_type_->is_struct() || resolved_type_->is_xception()) {
      auto as_struct = dynamic_cast<t_struct const*>(resolved_type_);
      return !as_struct->get_members().empty();
    }
    return false;
  }
  mstch::array get_namespace_array(t_program const* program) {
    std::vector<std::string> v;

    auto ns = program->get_namespace("cpp2");
    if (ns != "") {
      v = split_namespace(ns);
    } else {
      ns = program->get_namespace("cpp");
      if (ns != "") {
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
  mstch::node namespace_cpp2() {
    return get_namespace_array(type_->get_program());
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
            {"struct:cpp_noexcept_move_ctor",
             &mstch_cpp2_struct::cpp_noexcept_move_ctor},
            {"struct:final", &mstch_cpp2_struct::cpp_final},
            {"struct:message", &mstch_cpp2_struct::message},
            {"struct:struct_list", &mstch_cpp2_struct::struct_list},
            {"struct:non_req_fields?", &mstch_cpp2_struct::has_non_req_fields},
            {"struct:isset_fields?", &mstch_cpp2_struct::has_isset_fields},
            {"struct:isset_fields", &mstch_cpp2_struct::isset_fields},
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
    std::vector<t_field const*> filtered_fields;
    for (auto const* field : strct_->get_members()) {
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
      if (f->annotations_.count("cpp.ref") ||
          f->annotations_.count("cpp2.ref") ||
          (f->annotations_.count("cpp.ref_type") &&
           f->annotations_.at("cpp.ref_type") == "unique") ||
          (f->annotations_.count("cpp2.ref_type") &&
           f->annotations_.at("cpp.ref_type") == "unique")) {
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
    if (strct_->annotations_.count("cpp.declare_hash")) {
      return true;
    } else if (strct_->annotations_.count("cpp2.declare_hash")) {
      return true;
    }
    return false;
  }
  mstch::node cpp_declare_equal_to() {
    if (strct_->annotations_.count("cpp.declare_equal_to")) {
      return true;
    } else if (strct_->annotations_.count("cpp2.declare_equal_to")) {
      return true;
    }
    return false;
  }
  mstch::node cpp_noncopyable() {
    if (strct_->annotations_.count("cpp2.noncopyable")) {
      return true;
    }
    return false;
  }
  mstch::node cpp_noexcept_move_ctor() {
    if (strct_->annotations_.count("cpp.noexcept_move_ctor")) {
      return true;
    } else if (strct_->annotations_.count("cpp2.noexcept_move_ctor")) {
      return true;
    }
    return false;
  }
  mstch::node cpp_final() {
    if (strct_->annotations_.count("final")) {
      return true;
    }
    return false;
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
        std::begin(strct_->get_members()),
        std::end(strct_->get_members()),
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
            {"function:eb", &mstch_cpp2_function::eb},
        });
  }
  mstch::node eb() {
    auto const* strct = function_->get_annotations();
    if (strct && strct->annotations_.count("thread") &&
        strct->annotations_.at("thread") == "eb") {
      return true;
    }
    return false;
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
            {"service:programName", &mstch_cpp2_service::program_name},
            {"service:programIncludePrefix",
             &mstch_cpp2_service::include_prefix},
            {"service:thrift_includes", &mstch_cpp2_service::thrift_includes},
            {"service:namespace_cpp2", &mstch_cpp2_service::namespace_cpp2},
            {"service:oneway_functions", &mstch_cpp2_service::oneway_functions},
            {"service:oneways?", &mstch_cpp2_service::has_oneway},
            {"service:cpp_includes", &mstch_cpp2_service::cpp_includes},
            {"service:frozen2?", &mstch_cpp2_service::frozen2},
        });
  }
  mstch::array get_namespace_array(t_program const* program) {
    std::vector<std::string> v;

    auto ns = program->get_namespace("cpp2");
    if (ns != "") {
      v = split_namespace(ns);
    } else {
      ns = program->get_namespace("cpp");
      if (ns != "") {
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
  mstch::node program_name() {
    return service_->get_program()->get_name();
  }
  mstch::node include_prefix() {
    if (!service_->get_program()->get_include_prefix().empty()) {
      return cache_->parsed_options_["include_prefix"] + "/gen-cpp2/";
    }
    return std::string("");
  }
  mstch::node thrift_includes() {
    mstch::array a{};
    for (auto const* program : service_->get_program()->get_includes()) {
      std::string program_id =
          program->get_name() + get_service_namespace(service_->get_program());
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
    return get_namespace_array(service_->get_program());
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
  mstch::node cpp_includes() {
    mstch::array a{};
    for (auto const& include : service_->get_program()->get_cpp_includes()) {
      mstch::map cpp_include;
      cpp_include.emplace(
          "system?", include.at(0) == '<' ? std::to_string(0) : "");
      cpp_include.emplace("path", std::string(include));
      a.push_back(cpp_include);
    }
    return a;
  }
  mstch::node frozen2() {
    return cache_->parsed_options_.count("frozen2") != 0;
  }
};

class mstch_cpp2_const : public mstch_const {
 public:
  mstch_cpp2_const(
      t_const const* cnst,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos,
      int32_t index)
      : mstch_const(cnst, generators, cache, pos, index) {
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
            {"program:normalizedIncludePrefix",
             &mstch_cpp2_program::include_prefix},
            {"program:enums?", &mstch_cpp2_program::has_enums},
            {"program:typedefs?", &mstch_cpp2_program::has_typedefs},
            {"program:thrift_includes", &mstch_cpp2_program::thrift_includes},
            {"program:frozen2?", &mstch_cpp2_program::frozen2},
            {"program:indirection?", &mstch_cpp2_program::has_indirection},
        });
  }
  virtual std::string get_program_namespace() override {
    auto ns = program_->get_namespace("cpp2");
    if (ns.empty()) {
      ns = program_->get_namespace("cpp");
      if (ns.empty()) {
        ns = "cpp2";
      }
    }
    return ns;
  }
  mstch::array get_namespace_array() {
    std::vector<std::string> v;

    auto ns = program_->get_namespace("cpp2");
    if (ns != "") {
      v = split_namespace(ns);
    } else {
      ns = program_->get_namespace("cpp");
      if (ns != "") {
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
  mstch::node cpp_includes() {
    mstch::array a{};
    for (auto const& include : program_->get_cpp_includes()) {
      mstch::map cpp_include;
      cpp_include.emplace(
          "system?", include.at(0) == '<' ? std::to_string(0) : "");
      cpp_include.emplace("path", std::string(include));
      a.push_back(cpp_include);
    }
    return a;
  }
  mstch::node namespace_cpp2() {
    return get_namespace_array();
  }
  mstch::node include_prefix() {
    if (!program_->get_include_prefix().empty()) {
      return cache_->parsed_options_["include_prefix"] + "/gen-cpp2/";
    }
    return std::string("");
  }
  mstch::node has_enums() {
    return !program_->get_enums().empty();
  }
  mstch::node has_typedefs() {
    return !program_->get_typedefs().empty();
  }
  mstch::node thrift_includes() {
    mstch::array a{};
    for (auto const* program : program_->get_includes()) {
      std::string program_id = program->get_name() + get_program_namespace();
      if (!cache_->programs_.count(program_id)) {
        cache_->programs_[program_id] =
            generators_->program_generator_->generate(
                program, generators_, cache_);
      }
      a.push_back(cache_->programs_[program_id]);
    }
    return a;
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
    return true;
  }
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
      int32_t index = 0) const override {
    return std::make_shared<mstch_cpp2_const>(
        cnst, generators, cache, pos, index);
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
  // TODO: use gen-cpp2 when this implementation is ready to replace the
  // old python implementation.
  out_dir_base_ = "gen-mstch_cpp2";
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
  generators_->set_struct_generator(std::make_unique<struct_cpp2_generator>());
  generators_->set_function_generator(
      std::make_unique<function_cpp2_generator>());
  generators_->set_service_generator(
      std::make_unique<service_cpp2_generator>());
  generators_->set_const_generator(std::make_unique<const_cpp2_generator>());
  generators_->set_program_generator(
      std::make_unique<program_cpp2_generator>());
}

void t_mstch_cpp2_generator::generate_constants(t_program const* program) {
  std::string name = program->get_name();
  std::string id = name + get_cpp2_namespace(program);
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
  std::string id = name + get_cpp2_namespace(program);
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
  std::string id =
      get_program()->get_name() + get_cpp2_namespace(service->get_program());
  std::string name = service->get_name();
  std::string service_id = id + name;
  if (!cache_->services_.count(service_id)) {
    cache_->services_[service_id] =
        generators_->service_generator_->generate(service, generators_, cache_);
  }
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
}

THRIFT_REGISTER_GENERATOR(mstch_cpp2, "cpp2", "");
