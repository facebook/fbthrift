/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
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

#include <algorithm>
#include <iterator>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem.hpp>

#include <thrift/compiler/ast/t_field.h>
#include <thrift/compiler/ast/t_service.h>
#include <thrift/compiler/detail/mustache/mstch.h>
#include <thrift/compiler/generate/common.h>
#include <thrift/compiler/generate/t_mstch_generator.h>
#include <thrift/compiler/generate/t_mstch_objects.h>
#include <thrift/compiler/lib/cpp2/util.h>
#include <thrift/compiler/lib/py3/util.h>

namespace apache {
namespace thrift {
namespace compiler {

namespace {

bool is_type_iobuf(const std::string& name) {
  return name == "folly::IOBuf" || name == "std::unique_ptr<folly::IOBuf>";
}

const t_const* find_structured_adapter_annotation(const t_named& node) {
  return node.find_structured_annotation_or_null(
      "facebook.com/thrift/annotation/python/Adapter");
}

const std::string get_annotation_property(
    const t_const* annotation, const std::string& key) {
  if (annotation) {
    for (const auto& item : annotation->value()->get_map()) {
      if (item.first->get_string() == key) {
        return item.second->get_string();
      }
    }
  }
  return "";
}

const std::string extract_module_path(const std::string& fully_qualified_name) {
  auto tokens = split_namespace(fully_qualified_name);
  if (tokens.size() <= 1) {
    return "";
  }
  tokens.pop_back();
  return boost::algorithm::join(tokens, ".");
}

class mstch_python_typedef : public mstch_typedef {
 public:
  mstch_python_typedef(
      const t_typedef* typedf,
      const mstch_factories& factories,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos)
      : mstch_typedef(typedf, factories, std::move(cache), pos),
        adapter_annotation_(find_structured_adapter_annotation(*typedf)) {
    register_methods(
        this,
        {
            {"typedef:has_adapter?", &mstch_python_typedef::has_adapter},
            {"typedef:adapter_type_hint",
             &mstch_python_typedef::adapter_type_hint},
        });
  }

  mstch::node has_adapter() { return adapter_annotation_ != nullptr; }

  mstch::node adapter_type_hint() {
    return get_annotation_property(adapter_annotation_, "typeHint");
  }

 private:
  const t_const* adapter_annotation_;
};

class mstch_python_type : public mstch_type {
 public:
  mstch_python_type(
      const t_type* type,
      const mstch_factories& factories,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      const t_program* prog)
      : mstch_type(
            type->get_true_type(), std::move(factories), std::move(cache), pos),
        prog_{prog},
        adapter_annotation_(find_structured_adapter_annotation(*type)) {
    register_methods(
        this,
        {
            {"type:module_path", &mstch_python_type::module_path},
            {"type:program_name", &mstch_python_type::program_name},
            {"type:metadata_path", &mstch_python_type::metadata_path},
            {"type:py3_namespace", &mstch_python_type::py3_namespace},
            {"type:need_module_path?", &mstch_python_type::need_module_path},
            {"type:external_program?", &mstch_python_type::is_external_program},
            {"type:integer?", &mstch_python_type::is_integer},
            {"type:iobuf?", &mstch_python_type::is_iobuf},
            {"type:has_adapter?", &mstch_python_type::has_adapter},
            {"type:adapter_name", &mstch_python_type::adapter_name},
            {"type:adapter_type_hint", &mstch_python_type::adapter_type_hint},
            {"type:with_regular_response?",
             &mstch_python_type::with_regular_response},
        });
  }

  mstch::node module_path() {
    return get_py3_namespace_with_name_and_prefix(
               get_type_program(), get_option("root_module_prefix")) +
        ".thrift_types";
  }

  mstch::node program_name() { return get_type_program()->name(); }

  mstch::node metadata_path() {
    return get_py3_namespace_with_name_and_prefix(
               get_type_program(), get_option("root_module_prefix")) +
        ".thrift_metadata";
  }

  mstch::node py3_namespace() {
    std::ostringstream ss;
    for (const auto& path : get_py3_namespace(get_type_program())) {
      ss << path << ".";
    }
    return ss.str();
  }

  mstch::node need_module_path() {
    if (!has_option("is_types_file")) {
      return true;
    }
    if (const t_program* prog = type_->program()) {
      if (prog != prog_) {
        return true;
      }
    }
    return false;
  }

  mstch::node is_external_program() {
    auto p = type_->program();
    return p && p != prog_;
  }

  mstch::node is_integer() { return type_->is_any_int() || type_->is_byte(); }

  // Supporting legacy py3 cpp.type iobuf declaration here
  mstch::node is_iobuf() {
    return type_->has_annotation("py3.iobuf") ||
        is_type_iobuf(type_->get_annotation("cpp2.type")) ||
        is_type_iobuf(type_->get_annotation("cpp.type"));
  }

  mstch::node has_adapter() { return adapter_annotation_ != nullptr; }

  mstch::node adapter_name() {
    return get_annotation_property(adapter_annotation_, "name");
  }

  mstch::node adapter_type_hint() {
    return get_annotation_property(adapter_annotation_, "typeHint");
  }

  mstch::node with_regular_response() {
    if (!resolved_type_->is_streamresponse()) {
      return !resolved_type_->is_void();
    }
    auto stream = dynamic_cast<const t_stream_response*>(resolved_type_);
    return stream && !stream->first_response_type().empty();
  }

 protected:
  const t_program* get_type_program() const {
    if (const t_program* p = type_->program()) {
      return p;
    }
    return prog_;
  }

  const t_program* prog_;
  const t_const* adapter_annotation_;
};

class mstch_python_const_value : public mstch_const_value {
 public:
  mstch_python_const_value(
      const t_const_value* const_value,
      const mstch_factories& factories,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      int32_t index,
      const t_const* current_const,
      const t_type* expected_type)
      : mstch_const_value(
            const_value,
            std::move(factories),
            std::move(cache),
            pos,
            index,
            current_const,
            expected_type) {
    register_methods(
        this,
        {
            {"value:py3_string_value", &mstch_python_const_value::string_value},
            {"value:py3_enum_value_name",
             &mstch_python_const_value::py3_enum_value_name},
            {"value:py3_string?", &mstch_python_const_value::is_string},
            {"value:py3_binary?", &mstch_python_const_value::is_binary},
            {"value:const_enum_type",
             &mstch_python_const_value::const_enum_type},
            {"value:value_for_bool?",
             &mstch_python_const_value::value_for_bool},
            {"value:value_for_floating_point?",
             &mstch_python_const_value::value_for_floating_point},
            {"value:list_elem_type", &mstch_python_const_value::list_elem_type},
            {"value:value_for_set?", &mstch_python_const_value::value_for_set},
            {"value:map_key_type", &mstch_python_const_value::map_key_type},
            {"value:map_val_type", &mstch_python_const_value::map_val_type},
        });
  }
  mstch::node is_string() {
    auto& ttype = const_value_->ttype();
    return type_ == cv::CV_STRING && ttype &&
        ttype->get_true_type()->is_string();
  }

  mstch::node is_binary() {
    auto& ttype = const_value_->ttype();
    return type_ == cv::CV_STRING && ttype &&
        ttype->get_true_type()->is_binary();
  }

  mstch::node const_enum_type() {
    if (!const_value_->ttype() || type_ != cv::CV_INTEGER ||
        !const_value_->is_enum()) {
      return {};
    }
    const auto* type = const_value_->ttype()->get_true_type();
    if (type->is_enum()) {
      return factories_.type_factory->make_mstch_object(
          type, factories_, cache_);
    }
    return {};
  }

  mstch::node value_for_bool() {
    if (auto ttype = const_value_->ttype()) {
      return ttype->get_true_type()->is_bool();
    }
    return false;
  }

  mstch::node value_for_floating_point() {
    if (auto ttype = const_value_->ttype()) {
      return ttype->get_true_type()->is_floating_point();
    }
    return false;
  }

  mstch::node py3_enum_value_name() {
    if (const_value_->is_enum() && const_value_->get_enum_value() != nullptr) {
      return py3::get_py3_name(*const_value_->get_enum_value());
    }
    return mstch_const_value::enum_value_name();
  }

  mstch::node string_value() {
    if (type_ != cv::CV_STRING) {
      return mstch::node();
    }
    std::string string_val = const_value_->get_string();
    if (string_val.find('\n') == std::string::npos) {
      if (string_val.find('"') == std::string::npos) {
        return "\"" + string_val + "\"";
      }
      if (string_val.find('\'') == std::string::npos) {
        return "'" + string_val + "'";
      }
    }
    const auto& front = string_val.front();
    const auto& back = string_val.back();

    if (front != '"' && back != '"') {
      return "\"\"\"" + string_val + "\"\"\"";
    }
    if (front != '\'' && back != '\'') {
      return "'''" + string_val + "'''";
    }
    if (front == '"') { // and back = '\''
      string_val.pop_back(); // remove the last '\''
      return "'''" + string_val + "'''\"'\"";
    }
    // the only possible case left: back = '"' and front = '\''
    string_val.pop_back(); // remove the last '"'
    return "\"\"\"" + string_val + "\"\"\"'\"'";
  }

  mstch::node list_elem_type() {
    if (auto ttype = const_value_->ttype()) {
      const auto* type = ttype->get_true_type();
      const t_type* elem_type = nullptr;
      if (type->is_list()) {
        elem_type = dynamic_cast<const t_list*>(type)->get_elem_type();
      } else if (type->is_set()) {
        elem_type = dynamic_cast<const t_set*>(type)->get_elem_type();
      } else {
        return {};
      }
      return factories_.type_factory->make_mstch_object(
          elem_type, factories_, cache_, pos_);
    }
    return {};
  }

  mstch::node value_for_set() {
    if (auto ttype = const_value_->ttype()) {
      return ttype->get_true_type()->is_set();
    }
    return false;
  }

  mstch::node map_key_type() {
    if (auto ttype = const_value_->ttype()) {
      const auto* type = ttype->get_true_type();
      if (type->is_map()) {
        return factories_.type_factory->make_mstch_object(
            dynamic_cast<const t_map*>(type)->get_key_type(),
            factories_,
            cache_,
            pos_);
      }
    }
    return {};
  }

  mstch::node map_val_type() {
    if (auto ttype = const_value_->ttype()) {
      const auto* type = ttype->get_true_type();
      if (type->is_map()) {
        return factories_.type_factory->make_mstch_object(
            dynamic_cast<const t_map*>(type)->get_val_type(),
            factories_,
            cache_,
            pos_);
      }
    }
    return {};
  }
};

class python_type_factory : public mstch_type_factory {
 public:
  explicit python_type_factory(const t_program* prog) : prog_{prog} {}

  std::shared_ptr<mstch_base> make_mstch_object(
      const t_type* type,
      const mstch_factories& factories,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      int32_t /*index*/) const override {
    return std::make_shared<mstch_python_type>(
        type, factories, cache, pos, prog_);
  }

 protected:
  const t_program* prog_;
};

class mstch_python_program : public mstch_program {
 public:
  mstch_python_program(
      const t_program* program,
      const mstch_factories& factories,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos)
      : mstch_program{program, std::move(factories), std::move(cache), pos} {
    register_methods(
        this,
        {
            {"program:module_path", &mstch_python_program::module_path},
            {"program:py_deprecated_module_path",
             &mstch_python_program::py_deprecated_module_path},
            {"program:is_types_file?", &mstch_python_program::is_types_file},
            {"program:include_namespaces",
             &mstch_python_program::include_namespaces},
            {"program:base_library_package",
             &mstch_python_program::base_library_package},
            {"program:root_module_prefix",
             &mstch_python_program::root_module_prefix},
            {"program:adapter_modules", &mstch_python_program::adapter_modules},
            {"program:adapter_type_hint_modules",
             &mstch_python_program::adapter_type_hint_modules},
        });
    register_has_option("program:import_static?", "import_static");
    gather_included_program_namespaces();
    visit_types_for_services_and_interactions();
    visit_types_for_objects();
    visit_types_for_constants();
    visit_types_for_typedefs();
    visit_types_for_mixin_fields();
    visit_types_for_adapters();
  }

  mstch::node is_types_file() { return has_option("is_types_file"); }

  mstch::node include_namespaces() {
    std::vector<const Namespace*> namespaces;
    for (const auto& it : include_namespaces_) {
      namespaces.push_back(&it.second);
    }
    std::sort(
        namespaces.begin(), namespaces.end(), [](const auto* m, const auto* n) {
          return m->ns < n->ns;
        });
    mstch::array a;
    for (const auto& it : namespaces) {
      a.push_back(mstch::map{
          {"included_module_path", it->ns},
          {"has_services?", it->has_services},
          {"has_types?", it->has_types}});
    }
    return a;
  }

  mstch::node module_path() {
    return get_py3_namespace_with_name_and_prefix(
        program_, get_option("root_module_prefix"));
  }

  mstch::node py_deprecated_module_path() {
    std::string module_path = program_->get_namespace("py");
    if (module_path.empty()) {
      return program_->name();
    }
    return module_path;
  }

  mstch::node base_library_package() {
    auto option = get_option("base_library_package");
    return option.empty() ? "thrift.python" : option;
  }

  mstch::node root_module_prefix() {
    auto prefix = get_option("root_module_prefix");
    return prefix.empty() ? "" : prefix + ".";
  }

  mstch::node adapter_modules() { return module_path_array(adapter_modules_); }

  mstch::node adapter_type_hint_modules() {
    return module_path_array(adapter_type_hint_modules_);
  }

 protected:
  struct Namespace {
    std::string ns;
    bool has_services;
    bool has_types;
  };

  void gather_included_program_namespaces() {
    for (const t_program* included_program :
         program_->get_included_programs()) {
      bool has_types =
          !(included_program->objects().empty() &&
            included_program->enums().empty() &&
            included_program->typedefs().empty() &&
            included_program->consts().empty());
      include_namespaces_[included_program->path()] = Namespace{
          get_py3_namespace_with_name_and_prefix(
              included_program, get_option("root_module_prefix")),
          !included_program->services().empty(),
          has_types,
      };
    }
  }

  void add_typedef_namespace(const t_type* type) {
    auto prog = type->program();
    if (prog && prog != program_) {
      const auto& path = prog->path();
      if (include_namespaces_.find(path) != include_namespaces_.end()) {
        return;
      }
      auto ns = Namespace();
      ns.ns = get_py3_namespace_with_name_and_prefix(
          prog, get_option("root_module_prefix"));
      ns.has_services = false;
      ns.has_types = true;
      include_namespaces_[path] = std::move(ns);
    }
  }

  void visit_type_single_service(const t_service* service) {
    for (const auto& function : service->functions()) {
      for (const auto& field : function.get_paramlist()->fields()) {
        visit_type(field.get_type());
      }
      for (const auto& field : function.get_stream_xceptions()->fields()) {
        visit_type(field.get_type());
      }
      visit_type(function.get_returntype());
    }
  }

  void visit_types_for_services_and_interactions() {
    for (const auto* service : program_->services()) {
      visit_type_single_service(service);
    }
    for (const auto* interaction : program_->interactions()) {
      visit_type_single_service(interaction);
    }
  }

  void visit_types_for_objects() {
    for (const auto& object : program_->objects()) {
      for (auto&& field : object->fields()) {
        visit_type(field.get_type());
      }
    }
  }

  void visit_types_for_constants() {
    for (const auto& constant : program_->consts()) {
      visit_type(constant->get_type());
    }
  }

  void visit_types_for_typedefs() {
    for (const auto typedef_def : program_->typedefs()) {
      visit_type(typedef_def->get_type());
    }
  }

  void visit_types_for_mixin_fields() {
    for (const auto& strct : program_->structs()) {
      for (const auto& m : cpp2::get_mixins_and_members(*strct)) {
        visit_type(m.member->get_type());
      }
    }
  }

  void visit_types_for_adapters() {
    for (const auto& strct : program_->structs()) {
      if (auto annotation = find_structured_adapter_annotation(*strct)) {
        extract_module_and_insert_to(
            get_annotation_property(annotation, "name"), adapter_modules_);
        extract_module_and_insert_to(
            get_annotation_property(annotation, "typeHint"), adapter_modules_);
        extract_module_and_insert_to(
            get_annotation_property(annotation, "typeHint"),
            adapter_type_hint_modules_);
      }
      for (const auto& field : strct->fields()) {
        if (auto annotation = find_structured_adapter_annotation(field)) {
          extract_module_and_insert_to(
              get_annotation_property(annotation, "name"), adapter_modules_);
          extract_module_and_insert_to(
              get_annotation_property(annotation, "typeHint"),
              adapter_type_hint_modules_);
        }
      }
    }
    for (const auto typedef_def : program_->typedefs()) {
      if (auto annotation = find_structured_adapter_annotation(*typedef_def)) {
        extract_module_and_insert_to(
            get_annotation_property(annotation, "name"), adapter_modules_);
        extract_module_and_insert_to(
            get_annotation_property(annotation, "typeHint"), adapter_modules_);
        extract_module_and_insert_to(
            get_annotation_property(annotation, "typeHint"),
            adapter_type_hint_modules_);
      }
    }
  }

  void extract_module_and_insert_to(
      const std::string& name, std::unordered_set<std::string>& modules) {
    auto module_path = extract_module_path(name);
    if (module_path != "") {
      modules.insert(module_path);
    }
  }

  enum TypeDef { NoTypedef, HasTypedef };

  void visit_type(const t_type* orig_type) {
    return visit_type_with_typedef(orig_type, TypeDef::NoTypedef);
  }

  void visit_type_with_typedef(const t_type* orig_type, TypeDef is_typedef) {
    auto true_type = orig_type->get_true_type();
    if (!seen_types_.insert(true_type).second) {
      return;
    }
    is_typedef = is_typedef == TypeDef::HasTypedef || orig_type->is_typedef()
        ? TypeDef::HasTypedef
        : TypeDef::NoTypedef;
    if (is_typedef == TypeDef::HasTypedef) {
      add_typedef_namespace(true_type);
    }
    if (true_type->is_list()) {
      visit_type_with_typedef(
          dynamic_cast<const t_list&>(*true_type).get_elem_type(), is_typedef);
    } else if (true_type->is_set()) {
      visit_type_with_typedef(
          dynamic_cast<const t_set&>(*true_type).get_elem_type(), is_typedef);
    } else if (true_type->is_map()) {
      visit_type_with_typedef(
          dynamic_cast<const t_map&>(*true_type).get_key_type(), is_typedef);
      visit_type_with_typedef(
          dynamic_cast<const t_map&>(*true_type).get_val_type(), is_typedef);
    } else if (true_type->is_streamresponse()) {
      const t_type* resp_type =
          dynamic_cast<const t_stream_response&>(*true_type)
              .get_first_response_type();
      const t_type* elem_type =
          dynamic_cast<const t_stream_response&>(*true_type).get_elem_type();
      if (resp_type) {
        visit_type_with_typedef(resp_type, is_typedef);
      }
      visit_type_with_typedef(elem_type, is_typedef);
    }
  }

  mstch::node module_path_array(
      const std::unordered_set<std::string>& modules) {
    mstch::array a;
    for (const auto& m : modules) {
      a.push_back(mstch::map{{"module_path", m}});
    }
    return a;
  }

  std::unordered_map<std::string, Namespace> include_namespaces_;
  std::unordered_set<const t_type*> seen_types_;
  std::unordered_set<std::string> adapter_modules_;
  std::unordered_set<std::string> adapter_type_hint_modules_;
};

class mstch_python_field : public mstch_field {
 public:
  mstch_python_field(
      const t_field* field,
      const mstch_factories& factories,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      int32_t index,
      const field_generator_context* field_context)
      : mstch_field(
            field,
            std::move(factories),
            std::move(cache),
            pos,
            index,
            field_context),
        py_name_{py3::get_py3_name(*field)},
        adapter_annotation_(find_structured_adapter_annotation(*field)) {
    register_methods(
        this,
        {
            {"field:py_name", &mstch_python_field::py_name},
            {"field:tablebased_qualifier",
             &mstch_python_field::tablebased_qualifier},
            {"field:user_default_value",
             &mstch_python_field::user_default_value},
            {"field:has_adapter?", &mstch_python_field::has_adapter},
            {"field:adapter_name", &mstch_python_field::adapter_name},
            {"field:adapter_type_hint", &mstch_python_field::adapter_type_hint},
        });
  }

  mstch::node py_name() { return py_name_; }
  mstch::node tablebased_qualifier() {
    const std::string enum_type = "FieldQualifier.";
    switch (field_->qualifier()) {
      case t_field_qualifier::none:
      case t_field_qualifier::required:
        return enum_type + "Unqualified";
      case t_field_qualifier::optional:
        return enum_type + "Optional";
      case t_field_qualifier::terse:
        return enum_type + "Terse";
      default:
        throw std::runtime_error("unknown qualifier");
    }
  }
  mstch::node user_default_value() {
    const t_const_value* value = field_->get_value();
    if (!value) {
      return mstch::node();
    }
    if (value->is_empty()) {
      auto true_type = field_->get_type()->get_true_type();
      if ((true_type->is_list() || true_type->is_set()) &&
          value->get_type() != t_const_value::CV_LIST) {
        const_cast<t_const_value*>(value)->set_list();
      }
      if (true_type->is_map() && value->get_type() != t_const_value::CV_MAP) {
        const_cast<t_const_value*>(value)->set_map();
      }
    }
    return factories_.const_value_factory->make_mstch_object(
        value, factories_, cache_, pos_, 0, nullptr, nullptr);
  }
  mstch::node has_adapter() { return adapter_annotation_ != nullptr; }
  mstch::node adapter_name() {
    return get_annotation_property(adapter_annotation_, "name");
  }
  mstch::node adapter_type_hint() {
    return get_annotation_property(adapter_annotation_, "typeHint");
  }

 private:
  const std::string py_name_;
  const t_const* adapter_annotation_;
};

class mstch_python_struct : public mstch_struct {
 public:
  mstch_python_struct(
      const t_struct* strct,
      const mstch_factories& factories,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos)
      : mstch_struct(strct, std::move(factories), std::move(cache), pos),
        adapter_annotation_(find_structured_adapter_annotation(*strct)) {
    register_methods(
        this,
        {
            {"struct:fields_and_mixin_fields",
             &mstch_python_struct::fields_and_mixin_fields},
            {"struct:exception_message?",
             &mstch_python_struct::has_exception_message},
            {"struct:exception_message",
             &mstch_python_struct::exception_message},
            {"struct:has_adapter?", &mstch_python_struct::has_adapter},
            {"struct:adapter_name", &mstch_python_struct::adapter_name},
            {"struct:adapter_type_hint",
             &mstch_python_struct::adapter_type_hint},
        });
  }

  mstch::node fields_and_mixin_fields() {
    std::vector<const t_field*> fields = struct_->fields().copy();
    for (auto m : cpp2::get_mixins_and_members(*struct_)) {
      fields.push_back(m.member);
    }
    std::sort(fields.begin(), fields.end(), [](const auto* m, const auto* n) {
      return m->id() < n->id();
    });
    return make_mstch_fields(fields);
  }

  mstch::node has_exception_message() {
    return struct_->has_annotation("message");
  }
  mstch::node exception_message() { return struct_->get_annotation("message"); }

  mstch::node has_adapter() { return adapter_annotation_ != nullptr; }

  mstch::node adapter_name() {
    return get_annotation_property(adapter_annotation_, "name");
  }

  mstch::node adapter_type_hint() {
    return get_annotation_property(adapter_annotation_, "typeHint");
  }

 private:
  const t_const* adapter_annotation_;
};

class mstch_python_enum : public mstch_enum {
 public:
  mstch_python_enum(
      const t_enum* enm,
      const mstch_factories& factories,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos)
      : mstch_enum(enm, std::move(factories), std::move(cache), pos) {
    register_methods(
        this,
        {
            {"enum:flags?", &mstch_python_enum::has_flags},
        });
  }

  mstch::node has_flags() { return enm_->has_annotation("py3.flags"); }
};

class mstch_python_enum_value : public mstch_enum_value {
 public:
  mstch_python_enum_value(
      const t_enum_value* enum_value,
      const mstch_factories& factories,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos)
      : mstch_enum_value(
            enum_value, std::move(factories), std::move(cache), pos) {
    register_methods(
        this,
        {
            {"enum_value:py_name", &mstch_python_enum_value::py_name},
        });
  }

  mstch::node py_name() { return py3::get_py3_name(*enum_value_); }
};

class python_program_factory : public mstch_program_factory {
 public:
  std::shared_ptr<mstch_base> make_mstch_object(
      const t_program* program,
      const mstch_factories& factories,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      int32_t /*index*/) const override {
    const std::string& id = program->path();
    auto it = cache->programs_.find(id);
    if (it != cache->programs_.end()) {
      return it->second;
    }
    auto r = cache->programs_.emplace(
        id,
        std::make_shared<mstch_python_program>(program, factories, cache, pos));
    return r.first->second;
  }
};

class mstch_python_function : public mstch_function {
 public:
  mstch_python_function(
      const t_function* function,
      const mstch_factories& factories,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos)
      : mstch_function(function, factories, cache, pos) {
    register_methods(
        this,
        {
            {"function:args?", &mstch_python_function::has_args},
            {"function:regular_response_type",
             &mstch_python_function::regular_response_type},
            {"function:return_stream_elem_type",
             &mstch_python_function::return_stream_elem_type},
            {"function:async_only?", &mstch_python_function::async_only},
        });
  }

  mstch::node has_args() {
    return !function_->get_paramlist()->get_members().empty();
  }

  mstch::node regular_response_type() {
    if (function_->is_oneway()) {
      return {};
    }
    const t_type* rettype = function_->return_type()->get_true_type();
    if (rettype->is_streamresponse()) {
      auto stream = dynamic_cast<const t_stream_response*>(rettype);
      rettype = stream->has_first_response() ? stream->get_first_response_type()
                                             : &t_base_type::t_void();
    }
    return factories_.type_factory->make_mstch_object(
        rettype, factories_, cache_, pos_);
  }

  mstch::node return_stream_elem_type() {
    if (function_->is_oneway()) {
      return {};
    }
    const t_type* rettype = function_->return_type()->get_true_type();
    if (!rettype->is_streamresponse()) {
      return {};
    }
    return factories_.type_factory->make_mstch_object(
        dynamic_cast<const t_stream_response*>(rettype)->get_elem_type(),
        factories_,
        cache_,
        pos_);
  }

  mstch::node async_only() {
    return function_->returns_stream() || function_->returns_sink();
  }

 protected:
  const std::string cppName_;
};

class mstch_python_service : public mstch_service {
 public:
  mstch_python_service(
      const t_service* service,
      const mstch_factories& factories,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos,
      const t_program* prog)
      : mstch_service(service, factories, cache, pos), prog_{prog} {
    register_methods(
        this,
        {
            {"service:module_path", &mstch_python_service::module_path},
            {"service:program_name", &mstch_python_service::program_name},
            {"service:parent_service_name",
             &mstch_python_service::parent_service_name},
            {"service:supported_functions",
             &mstch_python_service::supported_functions},
            {"service:supported_service_functions",
             &mstch_python_service::supported_service_functions},
            {"service:external_program?",
             &mstch_python_service::is_external_program},
        });
  }

  mstch::node module_path() {
    return get_py3_namespace_with_name_and_prefix(
        service_->program(), get_option("root_module_prefix"));
  }

  mstch::node program_name() { return service_->program()->name(); }

  mstch::node parent_service_name() {
    return cache_->options_.at("parent_service_name");
  }

  std::vector<t_function*> get_supported_functions(
      std::function<bool(const t_function*)> func_filter) {
    std::vector<t_function*> funcs;
    for (auto func : service_->get_functions()) {
      if (func_filter(func)) {
        funcs.push_back(func);
      }
    }
    return funcs;
  }

  mstch::node supported_functions() {
    return make_mstch_functions(
        get_supported_functions([](const t_function* func) -> bool {
          return !func->returns_sink() && !func->get_returntype()->is_service();
        }));
  }

  mstch::node supported_service_functions() {
    return make_mstch_functions(
        get_supported_functions([](const t_function* func) -> bool {
          return !func->returns_stream() && !func->returns_sink() &&
              !func->get_returntype()->is_service();
        }));
  }

  mstch::node is_external_program() { return prog_ != service_->program(); }

 protected:
  const t_program* prog_;
};

class python_service_factory : public mstch_service_factory {
 public:
  explicit python_service_factory(const t_program* prog) : prog_{prog} {}

  std::shared_ptr<mstch_base> make_mstch_object(
      const t_service* service,
      const mstch_factories& factories,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      int32_t /*index*/) const override {
    return std::make_shared<mstch_python_service>(
        service, factories, cache, pos, prog_);
  }

 protected:
  const t_program* prog_;
};

// Generator-specific validator that enforces that a reserved key is not used as
// a namespace component.
class no_reserved_key_in_namespace_validator : virtual public validator {
 public:
  using validator::visit;

  bool visit(t_program* prog) override {
    validate(prog);
    return true;
  }

 private:
  void validate(t_program* prog) {
    auto namespace_tokens = get_py3_namespace(prog);
    if (namespace_tokens.empty()) {
      return;
    }
    for (const auto& field_name : namespace_tokens) {
      if (get_python_reserved_names().find(field_name) !=
          get_python_reserved_names().end()) {
        report_error(
            *prog,
            "Namespace '{}' contains reserved keyword '{}'",
            fmt::join(namespace_tokens, "."),
            field_name);
      }
    }

    std::vector<std::string> fields;
    boost::split(fields, prog->path(), boost::is_any_of("\\/."));
    for (const auto& field : fields) {
      if (field == "include") {
        report_error(
            *prog,
            "Path '{}' contains reserved keyword 'include'",
            prog->path());
      }
    }
  }
};

// Generator-specific validator that enforces "name" and "value" are not used as
// enum member or union field names (thrift-py3).
class enum_member_union_field_names_validator : virtual public validator {
 public:
  using validator::visit;

  bool visit(t_enum* enm) override {
    for (const t_enum_value* ev : enm->get_enum_values()) {
      validate(ev, ev->get_name());
    }
    return true;
  }

  bool visit(t_struct* s) override {
    if (!s->is_union()) {
      return false;
    }
    for (const t_field& f : s->fields()) {
      validate(&f, f.name());
    }
    return true;
  }

 private:
  void validate(const t_named* node, const std::string& name) {
    const auto& pyname = node->get_annotation("py3.name", &name);
    if (pyname == "name" || pyname == "value") {
      report_error(
          *node,
          "'{}' should not be used as an enum/union field name in thrift-py3. "
          "Use a different name or annotate the field with "
          "`(py3.name=\"<new_py_name>\")`",
          pyname);
    }
  }
};

class t_mstch_python_generator : public t_mstch_generator {
 public:
  t_mstch_python_generator(
      t_program* program,
      t_generation_context context,
      const std::map<std::string, std::string>& options)
      : t_mstch_generator(program, std::move(context), "python", options),
        generate_root_path_{package_to_path()} {
    out_dir_base_ = "gen-python";
    auto include_prefix = get_option("include_prefix");
    if (!include_prefix.empty()) {
      program->set_include_prefix(std::move(include_prefix));
    }
  }

  void generate_program() override {
    set_mstch_factories();
    generate_types();
    generate_metadata();
    generate_clients();
    generate_services();
  }

  void fill_validator_list(validator_list& vl) const override {
    vl.add<no_reserved_key_in_namespace_validator>();
    vl.add<enum_member_union_field_names_validator>();
  }

  enum TypesFile { IsTypesFile, NotTypesFile };

 protected:
  bool should_resolve_typedefs() const override { return true; }
  void set_mstch_factories();
  void generate_file(
      const std::string& file,
      TypesFile is_types_file,
      const boost::filesystem::path& base);
  void set_types_file(bool val);
  void generate_types();
  void generate_metadata();
  void generate_clients();
  void generate_services();
  boost::filesystem::path package_to_path();

  const boost::filesystem::path generate_root_path_;
};

} // namespace

void t_mstch_python_generator::set_mstch_factories() {
  factories_.program_factory = std::make_unique<python_program_factory>();
  factories_.service_factory =
      std::make_unique<python_service_factory>(program_);
  factories_.set_function_factory<mstch_python_function>();
  factories_.type_factory = std::make_unique<python_type_factory>(program_);
  factories_.set_typedef_factory<mstch_python_typedef>();
  factories_.set_struct_factory<mstch_python_struct>();
  factories_.set_field_factory<mstch_python_field>();
  factories_.set_enum_factory<mstch_python_enum>();
  factories_.set_enum_value_factory<mstch_python_enum_value>();
  factories_.set_const_value_factory<mstch_python_const_value>();
}

boost::filesystem::path t_mstch_python_generator::package_to_path() {
  auto package = get_py3_namespace(get_program());
  return boost::algorithm::join(package, "/");
}

void t_mstch_python_generator::generate_file(
    const std::string& file,
    TypesFile is_types_file,
    const boost::filesystem::path& base = {}) {
  auto program = get_program();
  const auto& name = program->name();
  if (is_types_file == IsTypesFile) {
    cache_->options_["is_types_file"] = "";
  } else {
    cache_->options_.erase("is_types_file");
  }
  auto node_ptr = factories_.program_factory->make_mstch_object(
      program, factories_, cache_);
  render_to_file(node_ptr, file, base / name / file);
}

void t_mstch_python_generator::generate_types() {
  generate_file("thrift_types.py", IsTypesFile, generate_root_path_);
  generate_file("thrift_types.pyi", IsTypesFile, generate_root_path_);
}

void t_mstch_python_generator::generate_metadata() {
  generate_file("thrift_metadata.py", IsTypesFile, generate_root_path_);
}

void t_mstch_python_generator::generate_clients() {
  if (get_program()->services().empty()) {
    // There is no need to generate empty / broken code for non existent
    // services.
    return;
  }

  generate_file("thrift_clients.py", NotTypesFile, generate_root_path_);
}

void t_mstch_python_generator::generate_services() {
  if (get_program()->services().empty()) {
    // There is no need to generate empty / broken code for non existent
    // services.
    return;
  }
  generate_file("thrift_services.py", NotTypesFile, generate_root_path_);
}

THRIFT_REGISTER_GENERATOR(
    mstch_python,
    "Python",
    "    include_prefix:  Use full include paths in generated files.\n");

} // namespace compiler
} // namespace thrift
} // namespace apache
