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

#include <cctype>
#include <set>
#include <string>
#include <unordered_set>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <thrift/compiler/generate/t_mstch_generator.h>
#include <thrift/compiler/generate/t_mstch_objects.h>

using namespace std;

namespace apache {
namespace thrift {
namespace compiler {

namespace {
std::string mangle(const std::string& name) {
  static const char* keywords[] = {
      "abstract", "alignof", "as",      "async",    "await",  "become",
      "box",      "break",   "const",   "continue", "crate",  "do",
      "else",     "enum",    "extern",  "false",    "final",  "fn",
      "for",      "if",      "impl",    "in",       "let",    "loop",
      "macro",    "match",   "mod",     "move",     "mut",    "offsetof",
      "override", "priv",    "proc",    "pub",      "pure",   "ref",
      "return",   "Self",    "self",    "sizeof",   "static", "struct",
      "super",    "trait",   "true",    "type",     "typeof", "unsafe",
      "unsized",  "use",     "virtual", "where",    "while",  "yield",
  };

  for (auto s : keywords) {
    if (name == s) {
      return name + '_';
    }
  }

  return name;
}

std::string mangle_type(const std::string& name) {
  static const char* primitives[] = {
      "i8",
      "u8",
      "i16",
      "u16",
      "i32",
      "u32",
      "i64",
      "u64",
      "i128",
      "u128",
      "f32",
      "f64",
      "isize",
      "usize",
      "str",
      "bool",
  };

  for (auto s : primitives) {
    if (name == s) {
      return name + '_';
    }
  }

  return mangle(name);
}

// Convert CamelCase to snake_case.
std::string snakecase(const std::string& name) {
  std::ostringstream snake;

  char last = '_';
  for (auto ch : name) {
    if (isupper(ch)) {
      if (last != '_') {
        // Don't insert '_' after an existing one, such as in `Sample_CalcRs`.
        // Also don't put a '_' right at the front.
        snake << '_';
      }
      last = (char)tolower(ch);
    } else {
      last = ch;
    }
    snake << last;
  }

  return snake.str();
}

// Convert snake_case to UpperCamelCase.
std::string camelcase(const std::string& name) {
  std::ostringstream camel;

  size_t i = 0;
  for (; i < name.size() && name[i] == '_'; i++) {
    // Copy same number of leading underscores.
    camel << '_';
  }

  auto underscore = true;
  for (; i < name.size(); i++) {
    if (name[i] == '_') {
      underscore = true;
    } else if (underscore) {
      camel << (char)toupper(name[i]);
      underscore = false;
    } else {
      camel << name[i];
    }
  }

  return camel.str();
}

std::string quote(const std::string& data) {
  std::ostringstream quoted;
  quoted << '"';

  for (auto ch : data) {
    if (ch == '\t') {
      quoted << '\\' << 't';
    } else if (ch == '\r') {
      quoted << '\\' << 'r';
    } else if (ch == '\n') {
      quoted << '\\' << 'n';
    } else if (ch == '\\' || ch == '"') {
      quoted << '\\' << ch;
    } else if (ch < '\x7f') {
      quoted << ch;
    } else {
      throw "Non-ASCII string literal not implemented";
    }
  }

  quoted << '"';
  return quoted.str();
}

bool can_derive_ord(const t_type* type) {
  type = type->get_true_type();
  if (type->is_string() || type->is_binary() || type->is_bool() ||
      type->is_byte() || type->is_i16() || type->is_i32() || type->is_i64() ||
      type->is_enum() || type->is_void()) {
    return true;
  }
  if (type->annotations_.count("rust.ord")) {
    return true;
  }
  if (type->is_list()) {
    auto elem_type = dynamic_cast<const t_list*>(type)->get_elem_type();
    return elem_type && can_derive_ord(elem_type);
  }
  return false;
}

struct rust_codegen_options {
  // Key: package name according to Thrift.
  // Value: rust_crate_name to use in generated code.
  std::map<std::string, std::string> cratemap;

  // Whether to emit derive(Serialize, Deserialize).
  // Enabled by `--gen rust:serde`.
  bool serde = false;

  // True if we are generating a submodule rather than the whole crate.
  bool multifile_mode = false;

  // The current program being generated and its Rust module path.
  const t_program* current_program;
  std::string current_crate;
};

std::string get_import_name(
    const t_program* program,
    const rust_codegen_options& options) {
  if (program == options.current_program) {
    return options.current_crate;
  }

  auto program_name = program->get_name();
  auto crate_name = options.cratemap.find(program_name);
  if (crate_name != options.cratemap.end()) {
    return crate_name->second;
  }
  return program_name;
}

enum class FieldKind { Box, Arc, Inline };

FieldKind field_kind(const std::map<std::string, std::string>& annotations) {
  if (annotations.count("rust.arc") != 0) {
    return FieldKind::Arc;
  }
  if (annotations.count("rust.box") != 0) {
    return FieldKind::Box;
  }
  return FieldKind::Inline;
}

// For example `set<Value> (rust.type = "indexmap::IndexSet")` or `map<string,
// Value> (rust.type = "indexmap::IndexMap")`. Unlike for standard library
// collections, serialization impls for these types are not provided in the
// fbthrift Rust runtime library and instead that logic will need to be emitted
// into the generated crate.
bool has_nonstandard_type_annotation(const t_type* type) {
  auto rust_type = type->annotations_.find("rust.type");
  return rust_type != type->annotations_.end() &&
      rust_type->second.find("::") != string::npos;
}
} // namespace

class t_mstch_rust_generator : public t_mstch_generator {
 public:
  t_mstch_rust_generator(
      t_program* program,
      t_generation_context context,
      const std::map<std::string, std::string>& parsed_options,
      const std::string& /* option_string */)
      : t_mstch_generator(program, std::move(context), "rust", parsed_options) {
    auto cratemap_flag = parsed_options.find("cratemap");
    if (cratemap_flag != parsed_options.end()) {
      load_crate_map(cratemap_flag->second);
    }

    options_.serde = parsed_options.count("serde");

    auto include_prefix_flag = parsed_options.find("include_prefix");
    if (include_prefix_flag != parsed_options.end()) {
      program->set_include_prefix(include_prefix_flag->second);
    }

    if (options_.multifile_mode) {
      options_.current_crate = "crate::" + mangle(program->get_name());
    } else {
      options_.current_crate = "crate";
    }

    options_.current_program = program;
    out_dir_base_ = "gen-rust";
  }

  void generate_program() override;
  void fill_validator_list(validator_list&) const override;

 private:
  void set_mstch_generators();
  void load_crate_map(const std::string& path);
  rust_codegen_options options_;
};

class mstch_rust_program : public mstch_program {
 public:
  mstch_rust_program(
      t_program const* program,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos,
      const rust_codegen_options& options)
      : mstch_program(program, generators, cache, pos), options_(options) {
    register_methods(
        this,
        {
            {"program:types?", &mstch_rust_program::rust_has_types},
            {"program:structsOrEnums?",
             &mstch_rust_program::rust_structs_or_enums},
            {"program:serde?", &mstch_rust_program::rust_serde},
            {"program:multifile?", &mstch_rust_program::rust_multifile},
            {"program:crate", &mstch_rust_program::rust_crate},
            {"program:package", &mstch_rust_program::rust_package},
            {"program:includes", &mstch_rust_program::rust_includes},
            {"program:anyServiceWithoutParent?",
             &mstch_rust_program::rust_any_service_without_parent},
            {"program:nonstandardCollections?",
             &mstch_rust_program::rust_has_nonstandard_collections},
            {"program:nonstandardCollections",
             &mstch_rust_program::rust_nonstandard_collections},
        });
  }
  mstch::node rust_has_types() {
    return !program_->get_structs().empty() || !program_->get_enums().empty() ||
        !program_->get_typedefs().empty() || !program_->get_xceptions().empty();
  }
  mstch::node rust_structs_or_enums() {
    return !program_->get_structs().empty() || !program_->get_enums().empty() ||
        !program_->get_xceptions().empty();
  }
  mstch::node rust_serde() {
    return options_.serde;
  }
  mstch::node rust_multifile() {
    return options_.multifile_mode;
  }
  mstch::node rust_crate() {
    if (options_.multifile_mode) {
      return "crate::" + mangle(program_->get_name());
    }
    return std::string("crate");
  }
  mstch::node rust_package() {
    return get_import_name(program_, options_);
  }
  mstch::node rust_includes() {
    mstch::array includes;
    for (auto* program : program_->get_included_programs()) {
      includes.push_back(generators_->program_generator_->generate(
          program, generators_, cache_, pos_));
    }
    return includes;
  }
  mstch::node rust_any_service_without_parent() {
    for (const t_service* service : program_->get_services()) {
      if (service->get_extends() == nullptr) {
        return true;
      }
    }
    return false;
  }
  template <typename F>
  void foreach_type(F&& f) const {
    for (auto strct : program_->get_structs()) {
      for (auto field : strct->get_members()) {
        f(field->get_type());
      }
    }
    for (auto service : program_->get_services()) {
      for (auto function : service->get_functions()) {
        for (auto arg : function->get_arglist()->get_members()) {
          f(arg->get_type());
        }
        f(function->get_returntype());
      }
    }
  }
  mstch::node rust_has_nonstandard_collections() {
    bool has_nonstandard_collection = false;
    foreach_type([&](t_type* type) {
      if (has_nonstandard_type_annotation(type)) {
        has_nonstandard_collection = true;
      }
    });
    return has_nonstandard_collection;
  }
  mstch::node rust_nonstandard_collections() {
    // Sort/deduplicate by value of `rust.type` annotation.
    struct rust_type_less {
      bool operator()(const t_type* lhs, const t_type* rhs) const {
        auto& lhs_annotation = lhs->annotations_.at("rust.type");
        auto& rhs_annotation = rhs->annotations_.at("rust.type");
        if (lhs_annotation != rhs_annotation) {
          return lhs_annotation < rhs_annotation;
        }
        return lhs->get_full_name() < rhs->get_full_name();
      }
    };
    std::set<const t_type*, rust_type_less> nonstandard_collections;
    foreach_type([&](t_type* type) {
      if (has_nonstandard_type_annotation(type)) {
        nonstandard_collections.insert(type);
      }
    });
    std::vector<const t_type*> elements(
        nonstandard_collections.begin(), nonstandard_collections.end());
    return generate_elements(
        elements, generators_->type_generator_.get(), generators_, cache_);
  }

 private:
  const rust_codegen_options& options_;
};

class mstch_rust_struct : public mstch_struct {
 public:
  mstch_rust_struct(
      const t_struct* strct,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos,
      const rust_codegen_options& options)
      : mstch_struct(strct, generators, cache, pos), options_(options) {
    register_methods(
        this,
        {
            {"struct:rust_name", &mstch_rust_struct::rust_name},
            {"struct:package", &mstch_rust_struct::rust_package},
            {"struct:ord?", &mstch_rust_struct::rust_is_ord},
            {"struct:copy?", &mstch_rust_struct::rust_is_copy},
        });
  }
  mstch::node rust_name() {
    return mangle_type(strct_->get_name());
  }
  mstch::node rust_package() {
    return get_import_name(strct_->get_program(), options_);
  }
  mstch::node rust_is_ord() {
    if (strct_->annotations_.count("rust.ord")) {
      return true;
    }
    for (const auto& member : strct_->get_members()) {
      if (!can_derive_ord(member->get_type())) {
        return false;
      }
    }
    return true;
  }
  mstch::node rust_is_copy() {
    return strct_->annotations_.count("rust.copy") != 0;
  }

 private:
  const rust_codegen_options& options_;
};

class mstch_rust_service : public mstch_service {
 public:
  mstch_rust_service(
      const t_service* service,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos,
      const rust_codegen_options& options)
      : mstch_service(service, generators, cache, pos), options_(options) {
    for (auto function : service->get_functions()) {
      function_upcamel_names_.insert(camelcase(function->get_name()));
    }
    register_methods(
        this,
        {
            {"service:rustFunctions", &mstch_rust_service::rust_functions},
            {"service:package", &mstch_rust_service::rust_package},
            {"service:snake", &mstch_rust_service::rust_snake},
            {"service:requestContext?",
             &mstch_rust_service::rust_request_context},
            {"service:extendedServices",
             &mstch_rust_service::rust_extended_services},
        });
  }
  mstch::node rust_functions();
  mstch::node rust_package() {
    return get_import_name(service_->get_program(), options_);
  }
  mstch::node rust_snake() {
    auto module_name = service_->annotations_.find("rust.mod");
    if (module_name != service_->annotations_.end()) {
      return module_name->second;
    }
    return mangle_type(snakecase(service_->get_name()));
  }
  mstch::node rust_request_context() {
    return service_->annotations_.count("rust.request_context") > 0;
  }
  mstch::node rust_extended_services() {
    mstch::array extended_services;
    const t_service* service = service_;
    std::string type_prefix =
        get_import_name(service_->get_program(), options_);
    std::string as_ref_impl = "&self.parent";
    while (true) {
      const t_service* parent_service = service->get_extends();
      if (parent_service == nullptr) {
        break;
      }
      if (parent_service->get_program() != service->get_program()) {
        type_prefix +=
            "::dependencies::" + parent_service->get_program()->get_name();
      }
      mstch::map node;
      node["extendedService:packagePrefix"] = type_prefix;
      node["extendedService:asRefImpl"] = as_ref_impl;
      node["extendedService:service"] =
          generate_cached_extended_service(parent_service);
      extended_services.push_back(node);
      as_ref_impl = "self.parent.as_ref()";
      service = parent_service;
    }
    return extended_services;
  }

 private:
  std::unordered_multiset<std::string> function_upcamel_names_;
  const rust_codegen_options& options_;
};

class mstch_rust_function : public mstch_function {
 public:
  mstch_rust_function(
      const t_function* function,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos,
      int32_t index,
      const std::unordered_multiset<std::string>& function_upcamel_names)
      : mstch_function(function, generators, cache, pos),
        index_(index),
        function_upcamel_names_(function_upcamel_names) {
    register_methods(
        this,
        {
            {"function:upcamel", &mstch_rust_function::rust_upcamel},
            {"function:index", &mstch_rust_function::rust_index},
            {"function:void?", &mstch_rust_function::rust_void},
            {"function:uniqueExceptions",
             &mstch_rust_function::rust_unique_exceptions},
        });
  }
  mstch::node rust_upcamel() {
    auto upcamel_name = camelcase(function_->get_name());
    if (function_upcamel_names_.count(upcamel_name) > 1) {
      // If a service contains a pair of methods that collide converted to
      // CamelCase, like a service containing both create_shard and createShard,
      // then we name the exception types without any case conversion; instead
      // of a CreateShardExn they'll get create_shardExn and createShardExn.
      return function_->get_name();
    }
    return upcamel_name;
  }
  mstch::node rust_index() {
    return index_;
  }
  mstch::node rust_void() {
    return function_->get_returntype()->is_void();
  }
  mstch::node rust_unique_exceptions() {
    // When generating From<> impls for an error type, we must not generate one
    // where more than one variant contains the same type of exception. Find
    // only those exceptions that map uniquely to a variant.

    std::map<t_type*, unsigned> type_count;
    for (const auto& field : function_->get_xceptions()->get_members()) {
      type_count[field->get_type()] += 1;
    }

    std::vector<t_field*> unique_exceptions;
    const auto& fields = function_->get_xceptions()->get_members();
    std::copy_if(
        fields.cbegin(),
        fields.cend(),
        std::back_inserter(unique_exceptions),
        [&type_count](const auto& field) {
          return type_count.at(field->get_type()) == 1;
        });

    return generate_elements(
        unique_exceptions,
        generators_->field_generator_.get(),
        generators_,
        cache_);
  }

 private:
  int32_t index_;
  const std::unordered_multiset<std::string>& function_upcamel_names_;
};

class mstch_rust_enum_value : public mstch_enum_value {
 public:
  mstch_rust_enum_value(
      const t_enum_value* enm_value,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos)
      : mstch_enum_value(enm_value, generators, cache, pos) {
    register_methods(
        this,
        {
            {"enumValue:rust_name", &mstch_rust_enum_value::rust_name},
        });
  }
  mstch::node rust_name() {
    return mangle(enm_value_->get_name());
  }
};

class mstch_rust_enum : public mstch_enum {
 public:
  mstch_rust_enum(
      const t_enum* enm,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos,
      const rust_codegen_options& options)
      : mstch_enum(enm, generators, cache, pos), options_(options) {
    register_methods(
        this,
        {
            {"enum:rust_name", &mstch_rust_enum::rust_name},
            {"enum:package", &mstch_rust_enum::rust_package},
            {"enum:values?", &mstch_rust_enum::rust_has_values},
        });
  }
  mstch::node rust_name() {
    return mangle_type(enm_->get_name());
  }
  mstch::node rust_package() {
    return get_import_name(enm_->get_program(), options_);
  }
  mstch::node rust_has_values() {
    return !enm_->get_enum_values().empty();
  }

 private:
  const rust_codegen_options& options_;
};

class mstch_rust_type : public mstch_type {
 public:
  mstch_rust_type(
      const t_type* type,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos,
      const rust_codegen_options& options)
      : mstch_type(type, generators, cache, pos), options_(options) {
    register_methods(
        this,
        {
            {"type:rust_name", &mstch_rust_type::rust_name},
            {"type:package", &mstch_rust_type::rust_package},
            {"type:rust", &mstch_rust_type::rust_type},
            {"type:nonstandardCollection?",
             &mstch_rust_type::rust_nonstandard_collection},
        });
  }
  mstch::node rust_name() {
    return mangle_type(type_->get_name());
  }
  mstch::node rust_package() {
    return get_import_name(type_->get_program(), options_);
  }
  mstch::node rust_type() {
    auto rust_type = type_->annotations_.find("rust.type");
    if (rust_type != type_->annotations_.end()) {
      if (rust_type->second.find("::") == std::string::npos) {
        return "std::collections::" + rust_type->second;
      } else {
        return rust_type->second;
      }
    }
    return nullptr;
  }
  mstch::node rust_nonstandard_collection() {
    return has_nonstandard_type_annotation(type_);
  }

 private:
  const rust_codegen_options& options_;
};

class mstch_rust_value : public mstch_base {
 public:
  using value_type = t_const_value::t_const_value_type;
  mstch_rust_value(
      const t_const_value* const_value,
      const t_type* type,
      unsigned depth,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      const rust_codegen_options& options)
      : mstch_base(generators, cache, pos),
        const_value_(const_value),
        type_(type),
        depth_(depth),
        options_(options) {
    // Step through any non-newtype typedefs.
    while (type_->is_typedef() &&
           type_->annotations_.count("rust.newtype") == 0) {
      auto typedef_type = dynamic_cast<const t_typedef*>(type_);
      if (!typedef_type) {
        break;
      }
      type_ = typedef_type->get_type();
    }

    register_methods(
        this,
        {
            {"value:type", &mstch_rust_value::type},
            {"value:newtype?", &mstch_rust_value::is_newtype},
            {"value:inner", &mstch_rust_value::inner},
            {"value:bool?", &mstch_rust_value::is_bool},
            {"value:boolValue", &mstch_rust_value::bool_value},
            {"value:integer?", &mstch_rust_value::is_integer},
            {"value:integerValue", &mstch_rust_value::integer_value},
            {"value:floatingPoint?", &mstch_rust_value::is_floating_point},
            {"value:floatingPointValue",
             &mstch_rust_value::floating_point_value},
            {"value:string?", &mstch_rust_value::is_string},
            {"value:binary?", &mstch_rust_value::is_binary},
            {"value:quoted", &mstch_rust_value::string_quoted},
            {"value:list?", &mstch_rust_value::is_list},
            {"value:listElements", &mstch_rust_value::list_elements},
            {"value:set?", &mstch_rust_value::is_set},
            {"value:setMembers", &mstch_rust_value::set_members},
            {"value:map?", &mstch_rust_value::is_map},
            {"value:mapEntries", &mstch_rust_value::map_entries},
            {"value:struct?", &mstch_rust_value::is_struct},
            {"value:structFields", &mstch_rust_value::struct_fields},
            {"value:union?", &mstch_rust_value::is_union},
            {"value:unionVariant", &mstch_rust_value::union_variant},
            {"value:unionValue", &mstch_rust_value::union_value},
            {"value:enum?", &mstch_rust_value::is_enum},
            {"value:enumPackage", &mstch_rust_value::enum_package},
            {"value:enumName", &mstch_rust_value::enum_name},
            {"value:enumVariant", &mstch_rust_value::enum_variant},
            {"value:empty?", &mstch_rust_value::is_empty},
            {"value:indent", &mstch_rust_value::indent},
        });
  }
  mstch::node type() {
    return std::make_shared<mstch_rust_type>(
        type_, generators_, cache_, pos_, options_);
  }
  mstch::node is_newtype() {
    return type_->is_typedef() &&
        type_->annotations_.count("rust.newtype") != 0;
  }
  mstch::node inner() {
    auto typedef_type = dynamic_cast<const t_typedef*>(type_);
    if (typedef_type) {
      auto inner_type = typedef_type->get_type();
      return std::make_shared<mstch_rust_value>(
          const_value_,
          inner_type,
          depth_,
          generators_,
          cache_,
          pos_,
          options_);
    }
    return mstch::node();
  }
  mstch::node is_bool() {
    return type_->is_bool();
  }
  mstch::node bool_value() {
    if (const_value_->get_type() == value_type::CV_INTEGER) {
      return const_value_->get_integer() != 0;
    }
    return const_value_->get_bool();
  }
  mstch::node is_integer() {
    return type_->is_byte() || type_->is_i16() || type_->is_i32() ||
        type_->is_i64();
  }
  mstch::node integer_value() {
    return std::to_string(const_value_->get_integer());
  }
  mstch::node is_floating_point() {
    return type_->is_float() || type_->is_double();
  }
  mstch::node floating_point_value() {
    std::ostringstream oss;
    oss << std::setprecision(std::numeric_limits<double>::digits10);
    oss << const_value_->get_double();
    auto digits = oss.str();
    if (digits.find('.') == std::string::npos &&
        digits.find('e') == std::string::npos &&
        digits.find('E') == std::string::npos) {
      digits += ".0";
    }
    return digits;
  }
  mstch::node is_string() {
    return type_->is_string();
  }
  mstch::node is_binary() {
    return type_->is_binary();
  }
  mstch::node string_quoted() {
    return quote(const_value_->get_string());
  }
  mstch::node is_list() {
    return type_->is_list() &&
        (const_value_->get_type() == value_type::CV_LIST ||
         (const_value_->get_type() == value_type::CV_MAP &&
          const_value_->get_map().empty()));
  }
  mstch::node list_elements() {
    const t_type* elem_type;
    if (type_->is_set()) {
      auto set_type = dynamic_cast<const t_set*>(type_);
      if (!set_type) {
        return mstch::node();
      }
      elem_type = set_type->get_elem_type();
    } else {
      auto list_type = dynamic_cast<const t_list*>(type_);
      if (!list_type) {
        return mstch::node();
      }
      elem_type = list_type->get_elem_type();
    }

    mstch::array elements;
    for (auto elem : const_value_->get_list()) {
      elements.push_back(std::make_shared<mstch_rust_value>(
          elem, elem_type, depth_ + 1, generators_, cache_, pos_, options_));
    }
    return elements;
  }
  mstch::node is_set() {
    return type_->is_set() &&
        (const_value_->get_type() == value_type::CV_LIST ||
         (const_value_->get_type() == value_type::CV_MAP &&
          const_value_->get_map().empty()));
  }
  mstch::node set_members() {
    return list_elements();
  }
  mstch::node is_map() {
    return type_->is_map() && const_value_->get_type() == value_type::CV_MAP;
  }
  mstch::node map_entries();
  mstch::node is_struct() {
    return (type_->is_struct() || type_->is_xception()) && !type_->is_union() &&
        const_value_->get_type() == value_type::CV_MAP;
  }
  mstch::node struct_fields();
  mstch::node is_union() {
    if (!type_->is_union() || const_value_->get_type() != value_type::CV_MAP) {
      return false;
    }
    if (const_value_->get_map().empty()) {
      // value will be the union's Default
      return true;
    }
    return const_value_->get_map().size() == 1 &&
        const_value_->get_map().at(0).first->get_type() ==
        value_type::CV_STRING;
  }
  mstch::node union_variant() {
    if (const_value_->get_map().empty()) {
      return mstch::node();
    }
    return const_value_->get_map().at(0).first->get_string();
  }
  mstch::node union_value() {
    auto struct_type = dynamic_cast<const t_struct*>(type_);
    if (!struct_type) {
      return mstch::node();
    }

    auto entry = const_value_->get_map().at(0);
    auto variant = entry.first->get_string();
    auto content = entry.second;

    for (const auto& member : struct_type->get_members()) {
      if (member->get_name() == variant) {
        return std::make_shared<mstch_rust_value>(
            content,
            member->get_type(),
            depth_ + 1,
            generators_,
            cache_,
            pos_,
            options_);
      }
    }
    return mstch::node();
  }
  mstch::node is_enum() {
    return type_->is_enum();
  }
  mstch::node enum_package() {
    if (const_value_->is_enum()) {
      return get_import_name(const_value_->get_enum()->get_program(), options_);
    }
    return mstch::node();
  }
  mstch::node enum_name() {
    if (const_value_->is_enum()) {
      return mangle_type(const_value_->get_enum()->get_name());
    }
    return mstch::node();
  }
  mstch::node enum_variant() {
    if (const_value_->is_enum()) {
      auto enum_value = const_value_->get_enum_value();
      if (enum_value) {
        return mangle(enum_value->get_name());
      }
    }
    return mstch::node();
  }
  mstch::node is_empty() {
    auto type = const_value_->get_type();
    if (type == value_type::CV_LIST) {
      return const_value_->get_list().empty();
    }
    if (type == value_type::CV_MAP) {
      return const_value_->get_map().empty();
    }
    if (type == value_type::CV_STRING) {
      return const_value_->get_string().empty();
    }
    return false;
  }
  mstch::node indent() {
    return std::string(4 * depth_, ' ');
  }

 private:
  const t_const_value* const_value_;
  const t_type* type_;
  unsigned depth_;
  const rust_codegen_options& options_;
};

class mstch_rust_map_entry : public mstch_base {
 public:
  mstch_rust_map_entry(
      const t_const_value* key,
      const t_type* key_type,
      const t_const_value* value,
      const t_type* value_type,
      unsigned depth,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      const rust_codegen_options& options)
      : mstch_base(generators, cache, pos),
        key_(key),
        key_type_(key_type),
        value_(value),
        value_type_(value_type),
        depth_(depth),
        options_(options) {
    register_methods(
        this,
        {
            {"entry:key", &mstch_rust_map_entry::key},
            {"entry:value", &mstch_rust_map_entry::value},
        });
  }
  mstch::node key() {
    return std::make_shared<mstch_rust_value>(
        key_, key_type_, depth_, generators_, cache_, pos_, options_);
  }
  mstch::node value() {
    return std::make_shared<mstch_rust_value>(
        value_, value_type_, depth_, generators_, cache_, pos_, options_);
  }

 private:
  const t_const_value* key_;
  const t_type* key_type_;
  const t_const_value* value_;
  const t_type* value_type_;
  unsigned depth_;
  const rust_codegen_options& options_;
};

class mstch_rust_struct_field : public mstch_base {
 public:
  mstch_rust_struct_field(
      const t_field* field,
      const t_const_value* value,
      unsigned depth,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      const rust_codegen_options& options)
      : mstch_base(generators, cache, pos),
        field_(field),
        value_(value),
        depth_(depth),
        options_(options) {
    register_methods(
        this,
        {
            {"field:rust_name", &mstch_rust_struct_field::rust_name},
            {"field:optional?", &mstch_rust_struct_field::is_optional},
            {"field:value", &mstch_rust_struct_field::value},
            {"field:type", &mstch_rust_struct_field::type},
            {"field:box?", &mstch_rust_struct_field::is_boxed},
            {"field:arc?", &mstch_rust_struct_field::is_arc},
        });
  }
  mstch::node rust_name() {
    return mangle(field_->get_name());
  }
  mstch::node is_optional() {
    return field_->get_req() == t_field::e_req::T_OPTIONAL;
  }
  mstch::node value() {
    if (value_) {
      auto type = field_->get_type();
      return std::make_shared<mstch_rust_value>(
          value_, type, depth_, generators_, cache_, pos_, options_);
    }
    return mstch::node();
  }
  mstch::node type() {
    auto type = field_->get_type();
    return std::make_shared<mstch_rust_type>(
        type, generators_, cache_, pos_, options_);
  }
  mstch::node is_boxed() {
    return field_kind(field_->annotations_) == FieldKind::Box;
  }
  mstch::node is_arc() {
    return field_kind(field_->annotations_) == FieldKind::Arc;
  }

 private:
  const t_field* field_;
  const t_const_value* value_;
  unsigned depth_;
  const rust_codegen_options& options_;
};

mstch::node mstch_rust_value::map_entries() {
  auto map_type = dynamic_cast<const t_map*>(type_);
  if (!map_type) {
    return mstch::node();
  }
  auto key_type = map_type->get_key_type();
  auto value_type = map_type->get_val_type();

  mstch::array entries;
  for (auto entry : const_value_->get_map()) {
    entries.push_back(std::make_shared<mstch_rust_map_entry>(
        entry.first,
        key_type,
        entry.second,
        value_type,
        depth_ + 1,
        generators_,
        cache_,
        pos_,
        options_));
  }
  return entries;
}

mstch::node mstch_rust_value::struct_fields() {
  auto struct_type = dynamic_cast<const t_struct*>(type_);
  if (!struct_type) {
    return mstch::node();
  }

  std::map<std::string, const t_const_value*> map_entries;
  for (auto entry : const_value_->get_map()) {
    auto key = entry.first;
    if (key->get_type() == value_type::CV_STRING) {
      map_entries[key->get_string()] = entry.second;
    }
  }

  mstch::array fields;
  for (const auto& member : struct_type->get_members()) {
    auto value = map_entries[member->get_name()];
    if (!value) {
      value = member->get_value();
    }
    fields.push_back(std::make_shared<mstch_rust_struct_field>(
        member, value, depth_ + 1, generators_, cache_, pos_, options_));
  }
  return fields;
}

class mstch_rust_const : public mstch_const {
 public:
  mstch_rust_const(
      const t_const* cnst,
      const t_const* current_const,
      const t_type* expected_type,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos,
      int32_t index,
      const std::string& field_name,
      const rust_codegen_options& options)
      : mstch_const(
            cnst,
            current_const,
            expected_type,
            generators,
            cache,
            pos,
            index,
            field_name),
        options_(options) {
    register_methods(
        this,
        {
            {"constant:package", &mstch_rust_const::rust_package},
            {"constant:lazy?", &mstch_rust_const::rust_lazy},
            {"constant:rust", &mstch_rust_const::rust_typed_value},
        });
  }
  mstch::node rust_package() {
    return get_import_name(cnst_->get_program(), options_);
  }
  mstch::node rust_lazy() {
    auto type = cnst_->get_type()->get_true_type();
    return type->is_list() || type->is_map() || type->is_set() ||
        type->is_struct();
  }
  mstch::node rust_typed_value() {
    unsigned depth = 0;
    return std::make_shared<mstch_rust_value>(
        cnst_->get_value(),
        cnst_->get_type(),
        depth,
        generators_,
        cache_,
        pos_,
        options_);
  }

 private:
  const rust_codegen_options& options_;
};

class mstch_rust_field : public mstch_field {
 public:
  mstch_rust_field(
      const t_field* field,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos,
      int32_t index,
      const rust_codegen_options& options)
      : mstch_field(field, generators, cache, pos, index), options_(options) {
    register_methods(
        this,
        {
            {"field:rust_name", &mstch_rust_field::rust_name},
            {"field:primitive?", &mstch_rust_field::rust_primitive},
            {"field:rename?", &mstch_rust_field::rust_rename},
            {"field:default", &mstch_rust_field::rust_default},
            {"field:box?", &mstch_rust_field::rust_is_boxed},
            {"field:arc?", &mstch_rust_field::rust_is_arc},
        });
  }
  mstch::node rust_name() {
    return mangle(field_->get_name());
  }
  mstch::node rust_primitive() {
    auto type = field_->get_type();
    return type->is_bool() || type->is_any_int() || type->is_floating_point();
  }
  mstch::node rust_rename() {
    return field_->get_name() != mangle(field_->get_name());
  }
  mstch::node rust_default() {
    auto value = field_->get_value();
    if (value) {
      unsigned depth = 2; // impl Default + fn default
      auto type = field_->get_type();
      return std::make_shared<mstch_rust_value>(
          value, type, depth, generators_, cache_, pos_, options_);
    }
    return mstch::node();
  }
  mstch::node rust_is_boxed() {
    return field_kind(field_->annotations_) == FieldKind::Box;
  }
  mstch::node rust_is_arc() {
    return field_kind(field_->annotations_) == FieldKind::Arc;
  }

 private:
  const rust_codegen_options& options_;
};

class mstch_rust_typedef : public mstch_typedef {
 public:
  mstch_rust_typedef(
      const t_typedef* typedf,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos)
      : mstch_typedef(typedf, generators, cache, pos) {
    register_methods(
        this,
        {
            {"typedef:rust_name", &mstch_rust_typedef::rust_name},
            {"typedef:newtype?", &mstch_rust_typedef::rust_newtype},
            {"typedef:ord?", &mstch_rust_typedef::rust_ord},
            {"typedef:copy?", &mstch_rust_typedef::rust_copy},
        });
  }
  mstch::node rust_name() {
    return mangle_type(typedf_->get_symbolic());
  }
  mstch::node rust_newtype() {
    return typedf_->annotations_.count("rust.newtype") != 0;
  }
  mstch::node rust_ord() {
    return typedf_->annotations_.count("rust.ord") != 0 ||
        can_derive_ord(typedf_->get_type());
  }
  mstch::node rust_copy() {
    auto inner = typedf_->get_true_type();
    if (inner->is_bool() || inner->is_byte() || inner->is_i16() ||
        inner->is_i32() || inner->is_i64() || inner->is_enum() ||
        inner->is_void()) {
      return true;
    }
    if (typedf_->annotations_.count("rust.copy")) {
      return true;
    }
    return false;
  }
};

class mstch_rust_annotation : public mstch_annotation {
 public:
  mstch_rust_annotation(
      const t_annotation& annotation,
      std::shared_ptr<const mstch_generators> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      int32_t index)
      : mstch_annotation(
            annotation.key,
            annotation.val,
            generators,
            cache,
            pos,
            index) {
    register_methods(
        this,
        {
            {"annotation:value?", &mstch_rust_annotation::rust_has_value},
            {"annotation:rust_name", &mstch_rust_annotation::rust_name},
            {"annotation:rust_value", &mstch_rust_annotation::rust_value},
        });
  }
  mstch::node rust_has_value() {
    return !val_.empty();
  }
  mstch::node rust_name() {
    return boost::algorithm::replace_all_copy(key_, ".", "_");
  }
  mstch::node rust_value() {
    return quote(val_);
  }
};

class program_rust_generator : public program_generator {
 public:
  explicit program_rust_generator(const rust_codegen_options& options)
      : options_(options) {}
  ~program_rust_generator() override = default;
  std::shared_ptr<mstch_base> generate(
      t_program const* program,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      int32_t /*index*/) const override {
    return std::make_shared<mstch_rust_program>(
        program, generators, cache, pos, options_);
  }

 private:
  const rust_codegen_options& options_;
};

class struct_rust_generator : public struct_generator {
 public:
  explicit struct_rust_generator(const rust_codegen_options& options)
      : options_(options) {}
  ~struct_rust_generator() override = default;
  std::shared_ptr<mstch_base> generate(
      const t_struct* strct,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      int32_t /*index*/) const override {
    return std::make_shared<mstch_rust_struct>(
        strct, generators, cache, pos, options_);
  }

 private:
  const rust_codegen_options& options_;
};

class service_rust_generator : public service_generator {
 public:
  explicit service_rust_generator(const rust_codegen_options& options)
      : options_(options) {}
  ~service_rust_generator() override = default;
  std::shared_ptr<mstch_base> generate(
      const t_service* service,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      int32_t /*index*/) const override {
    return std::make_shared<mstch_rust_service>(
        service, generators, cache, pos, options_);
  }

 private:
  const rust_codegen_options& options_;
};

class function_rust_generator {
 public:
  std::shared_ptr<mstch_base> generate(
      const t_function* function,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      int32_t index,
      const std::unordered_multiset<std::string>& function_upcamel_names)
      const {
    return std::make_shared<mstch_rust_function>(
        function, generators, cache, pos, index, function_upcamel_names);
  }
};

mstch::node mstch_rust_service::rust_functions() {
  function_rust_generator function_generator;
  return generate_elements(
      service_->get_functions(),
      &function_generator,
      generators_,
      cache_,
      function_upcamel_names_);
}

class field_rust_generator : public field_generator {
 public:
  explicit field_rust_generator(const rust_codegen_options& options)
      : options_(options) {}
  ~field_rust_generator() override = default;
  std::shared_ptr<mstch_base> generate(
      const t_field* field,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      int32_t index) const override {
    return std::make_shared<mstch_rust_field>(
        field, generators, cache, pos, index, options_);
  }

 private:
  const rust_codegen_options& options_;
};

class enum_value_rust_generator : public enum_value_generator {
 public:
  enum_value_rust_generator() = default;
  ~enum_value_rust_generator() override = default;
  std::shared_ptr<mstch_base> generate(
      const t_enum_value* enm_value,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      int32_t /*index*/) const override {
    return std::make_shared<mstch_rust_enum_value>(
        enm_value, generators, cache, pos);
  }
};

class enum_rust_generator : public enum_generator {
 public:
  explicit enum_rust_generator(const rust_codegen_options& options)
      : options_(options) {}
  ~enum_rust_generator() override = default;
  std::shared_ptr<mstch_base> generate(
      const t_enum* enm,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      int32_t /*index*/) const override {
    return std::make_shared<mstch_rust_enum>(
        enm, generators, cache, pos, options_);
  }

 private:
  const rust_codegen_options& options_;
};

class type_rust_generator : public type_generator {
 public:
  explicit type_rust_generator(const rust_codegen_options& options)
      : options_(options) {}
  ~type_rust_generator() override = default;
  std::shared_ptr<mstch_base> generate(
      const t_type* type,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      int32_t /*index*/) const override {
    return std::make_shared<mstch_rust_type>(
        type, generators, cache, pos, options_);
  }

 private:
  const rust_codegen_options& options_;
};

class const_rust_generator : public const_generator {
 public:
  explicit const_rust_generator(const rust_codegen_options& options)
      : options_(options) {}
  ~const_rust_generator() override = default;
  std::shared_ptr<mstch_base> generate(
      const t_const* cnst,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      int32_t index,
      const t_const* current_const,
      const t_type* expected_type,
      const std::string& field_name) const override {
    return std::make_shared<mstch_rust_const>(
        cnst,
        current_const,
        expected_type,
        generators,
        cache,
        pos,
        index,
        field_name,
        options_);
  }

 private:
  const rust_codegen_options& options_;
};

class typedef_rust_generator : public typedef_generator {
 public:
  typedef_rust_generator() = default;
  ~typedef_rust_generator() override = default;
  std::shared_ptr<mstch_base> generate(
      const t_typedef* typedf,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      int32_t /*index*/) const override {
    return std::make_shared<mstch_rust_typedef>(typedf, generators, cache, pos);
  }
};

class annotation_rust_generator : public annotation_generator {
 public:
  std::shared_ptr<mstch_base> generate(
      const t_annotation& annotation,
      std::shared_ptr<const mstch_generators> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      int32_t index) const override {
    return std::make_shared<mstch_rust_annotation>(
        annotation, generators, cache, pos, index);
  }
};

void t_mstch_rust_generator::generate_program() {
  set_mstch_generators();

  const auto* program = get_program();
  const auto& id = program->get_path();
  if (!cache_->programs_.count(id)) {
    cache_->programs_[id] =
        generators_->program_generator_->generate(program, generators_, cache_);
  }

  render_to_file(cache_->programs_[id], "lib.rs", "lib.rs");
}

void t_mstch_rust_generator::set_mstch_generators() {
  generators_->set_program_generator(
      std::make_unique<program_rust_generator>(options_));
  generators_->set_struct_generator(
      std::make_unique<struct_rust_generator>(options_));
  generators_->set_service_generator(
      std::make_unique<service_rust_generator>(options_));
  generators_->set_field_generator(
      std::make_unique<field_rust_generator>(options_));
  generators_->set_enum_value_generator(
      std::make_unique<enum_value_rust_generator>());
  generators_->set_enum_generator(
      std::make_unique<enum_rust_generator>(options_));
  generators_->set_type_generator(
      std::make_unique<type_rust_generator>(options_));
  generators_->set_const_generator(
      std::make_unique<const_rust_generator>(options_));
  generators_->set_typedef_generator(
      std::make_unique<typedef_rust_generator>());
  generators_->set_annotation_generator(
      std::make_unique<annotation_rust_generator>());
}

void t_mstch_rust_generator::load_crate_map(const std::string& path) {
  // Each line of the file is:
  // thrift_name crate_name
  //
  // As an example of each value, we might have:
  //   - thrift_name: demo
  //     (this is the name by which the dependency is referred to in thrift)
  //   - crate_name: demo_api
  //     (the Rust code will refer to demo_api::types::WhateverType)
  auto in = std::ifstream(path);

  // Map from crate_name to list of thrift_names. Most Thrift crates consist of
  // a single *.thrift file but some may have multiple.
  std::map<std::string, std::vector<std::string>> sources;

  std::string line;
  while (std::getline(in, line)) {
    std::istringstream iss(line);
    std::string thrift_name, crate_name;
    iss >> thrift_name >> crate_name;
    sources[crate_name].push_back(thrift_name);
  }

  for (const auto& source : sources) {
    std::string crate_name;
    auto thrift_names = source.second;
    auto multifile = thrift_names.size() > 1;

    // Look out for our own crate in the cratemap. It will require paths that
    // begin with `crate::module` rather than `::depenency::module`.
    if (source.first == "crate") {
      crate_name = "crate";
      options_.multifile_mode = multifile;
    } else {
      crate_name = "::" + mangle(source.first);
    }

    if (multifile) {
      for (const auto& thrift_name : thrift_names) {
        options_.cratemap[thrift_name] =
            crate_name + "::" + mangle(thrift_name);
      }
    } else if (crate_name != "crate") {
      options_.cratemap[thrift_names[0]] = crate_name;
    }
  }
}

namespace {
class annotation_validator : public validator {
 public:
  using validator::visit;
  bool visit(t_struct* s) override;
};

bool annotation_validator::visit(t_struct* s) {
  for (auto* member : s->get_members()) {
    bool box = member->annotations_.count("rust.box") != 0;
    bool arc = member->annotations_.count("rust.arc") != 0;
    if (box && arc) {
      add_error(
          member->get_lineno(),
          "Field `" + member->get_name() +
              "` cannot be both Box'ed and Arc'ed");
    }
  }
  return true;
}
} // namespace

void t_mstch_rust_generator::fill_validator_list(validator_list& l) const {
  l.add<annotation_validator>();
}

THRIFT_REGISTER_GENERATOR(mstch_rust, "Rust", "");

} // namespace compiler
} // namespace thrift
} // namespace apache
