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

#include <cassert>
#include <cctype>
#include <set>
#include <string>
#include <unordered_set>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <thrift/compiler/ast/t_struct.h>
#include <thrift/compiler/generate/t_mstch_generator.h>
#include <thrift/compiler/generate/t_mstch_objects.h>

using namespace std;

namespace apache {
namespace thrift {
namespace compiler {

namespace {
std::string mangle(const std::string& name) {
  static const char* raw_identifiable_keywords[] = {
      "abstract", "alignof", "as",      "async",    "await",    "become",
      "box",      "break",   "const",   "continue", "do",       "else",
      "enum",     "extern",  "false",   "final",    "fn",       "for",
      "if",       "impl",    "in",      "let",      "loop",     "macro",
      "match",    "mod",     "move",    "mut",      "offsetof", "override",
      "priv",     "proc",    "pub",     "pure",     "ref",      "return",
      "sizeof",   "static",  "struct",  "trait",    "true",     "type",
      "typeof",   "unsafe",  "unsized", "use",      "virtual",  "where",
      "while",    "yield",
  };

  static const char* keywords_that_participate_in_name_resolution[] = {
      "crate",
      "super",
      "self",
      "Self",
  };

  constexpr const char* keyword_error_message = R"ERROR(
    Found a rust keyword that participates in name resolution.
    Please use the `rust.name` annotation to create an alias for)ERROR";

  for (auto& s : keywords_that_participate_in_name_resolution) {
    if (name == s) {
      std::ostringstream error_message;
      error_message << keyword_error_message << " " << name;
      throw std::runtime_error(error_message.str());
    }
  }

  for (auto& s : raw_identifiable_keywords) {
    if (name == s) {
      return "r#" + name;
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
      throw std::runtime_error("Non-ASCII string literal not implemented");
    }
  }

  quoted << '"';
  return quoted.str();
}

std::string quoted_rust_doc(const t_node* node) {
  const std::string doc = node->get_doc();

  // strip leading/trailing whitespace
  static const std::string whitespace = "\n\r\t ";
  const auto first = doc.find_first_not_of(whitespace);
  if (first == std::string::npos) {
    // empty string
    return "\"\"";
  }

  const auto last = doc.find_last_not_of(whitespace);
  return quote(doc.substr(first, last - first + 1));
}

bool can_derive_ord(const t_type* type) {
  type = type->get_true_type();
  if (type->is_string() || type->is_binary() || type->is_bool() ||
      type->is_byte() || type->is_i16() || type->is_i32() || type->is_i64() ||
      type->is_enum() || type->is_void()) {
    return true;
  }
  if (type->has_annotation("rust.ord")) {
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

  // Whether fields w/optional values of None should
  // be skipped during serialization. Enabled w/ `--gen
  // rust:skip_none_serialization` Note: `rust:serde` must also be set for this
  // to affect codegen.
  bool skip_none_serialization = false;

  // Whether to skip server stubs. Server stubs are built by default, but can
  // be turned off via `--gen rust:noserver`.
  bool noserver = false;

  // True if we are generating a submodule rather than the whole crate.
  bool multifile_mode = false;

  // List of extra sources to include at top-level of the crate.
  std::vector<std::string> include_srcs;

  // The current program being generated and its Rust module path.
  const t_program* current_program;
  std::string current_crate;
};

std::string get_import_name(
    const t_program* program, const rust_codegen_options& options) {
  if (program == options.current_program) {
    return options.current_crate;
  }

  auto program_name = program->name();
  auto crate_name = options.cratemap.find(program_name);
  if (crate_name != options.cratemap.end()) {
    return crate_name->second;
  }
  return program_name;
}

enum class FieldKind { Box, Arc, Inline };

FieldKind field_kind(const t_named& node) {
  if (node.has_annotation("rust.arc")) {
    return FieldKind::Arc;
  }
  if (node.has_annotation("rust.box")) {
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
  return type->get_annotation("rust.type").find("::") != string::npos;
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
    options_.noserver = parsed_options.count("noserver");
    options_.skip_none_serialization =
        parsed_options.count("skip_none_serialization");
    if (options_.skip_none_serialization) {
      assert(options_.serde);
    }

    auto include_prefix_flag = parsed_options.find("include_prefix");
    if (include_prefix_flag != parsed_options.end()) {
      program->set_include_prefix(include_prefix_flag->second);
    }

    auto include_srcs = parsed_options.find("include_srcs");
    if (include_srcs != parsed_options.end()) {
      auto paths = include_srcs->second;

      string::size_type pos = 0;
      while (pos != string::npos && pos < paths.size()) {
        string::size_type next_pos = paths.find(':', pos);
        auto path = paths.substr(pos, next_pos - pos);
        options_.include_srcs.push_back(path);
        pos = ((next_pos == string::npos) ? next_pos : next_pos + 1);
      }
    }

    if (options_.multifile_mode) {
      options_.current_crate = "crate::" + mangle(program->name());
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
            {"program:nonexhaustiveStructs?",
             &mstch_rust_program::rust_nonexhaustive_structs},
            {"program:serde?", &mstch_rust_program::rust_serde},
            {"program:skip_none_serialization?",
             &mstch_rust_program::rust_skip_none_serialization},
            {"program:server?", &mstch_rust_program::rust_server},
            {"program:multifile?", &mstch_rust_program::rust_multifile},
            {"program:crate", &mstch_rust_program::rust_crate},
            {"program:package", &mstch_rust_program::rust_package},
            {"program:includes", &mstch_rust_program::rust_includes},
            {"program:anyServiceWithoutParent?",
             &mstch_rust_program::rust_any_service_without_parent},
            {"program:nonstandardTypes?",
             &mstch_rust_program::rust_has_nonstandard_types},
            {"program:nonstandardTypes",
             &mstch_rust_program::rust_nonstandard_types},
            {"program:docs?", &mstch_rust_program::rust_has_docs},
            {"program:docs", &mstch_rust_program::rust_docs},
            {"program:include_srcs", &mstch_rust_program::rust_include_srcs},
        });
  }
  mstch::node rust_has_types() {
    return !program_->structs().empty() || !program_->enums().empty() ||
        !program_->typedefs().empty() || !program_->xceptions().empty();
  }
  mstch::node rust_structs_or_enums() {
    return !program_->structs().empty() || !program_->enums().empty() ||
        !program_->xceptions().empty();
  }
  mstch::node rust_nonexhaustive_structs() {
    for (auto& strct : program_->structs()) {
      // The is_union is because `union` are also in this collection.
      if (!strct->is_union() && !strct->has_annotation("rust.exhaustive")) {
        return true;
      }
    }
    for (auto& strct : program_->xceptions()) {
      if (!strct->has_annotation("rust.exhaustive")) {
        return true;
      }
    }
    return false;
  }
  mstch::node rust_serde() { return options_.serde; }
  mstch::node rust_skip_none_serialization() {
    return options_.skip_none_serialization;
  }
  mstch::node rust_server() { return !options_.noserver; }
  mstch::node rust_multifile() { return options_.multifile_mode; }
  mstch::node rust_crate() {
    if (options_.multifile_mode) {
      return "crate::" + mangle(program_->name());
    }
    return std::string("crate");
  }
  mstch::node rust_package() { return get_import_name(program_, options_); }
  mstch::node rust_includes() {
    mstch::array includes;
    for (auto* program : program_->get_included_programs()) {
      includes.push_back(generators_->program_generator_->generate(
          program, generators_, cache_, pos_));
    }
    return includes;
  }
  mstch::node rust_any_service_without_parent() {
    for (const t_service* service : program_->services()) {
      if (service->get_extends() == nullptr) {
        return true;
      }
    }
    return false;
  }
  template <typename F>
  void foreach_type(F&& f) const {
    for (const auto* strct : program_->structs()) {
      for (const auto& field : strct->fields()) {
        f(field.get_type());
      }
    }
    for (const auto* service : program_->services()) {
      for (const auto& function : service->functions()) {
        for (const auto& param : function.get_paramlist()->fields()) {
          f(param.get_type());
        }
        f(function.get_returntype());
      }
    }
    for (auto typedf : program_->typedefs()) {
      f(typedf);
    }
  }
  mstch::node rust_has_nonstandard_types() {
    bool has_nonstandard_types = false;
    foreach_type([&](const t_type* type) {
      if (has_nonstandard_type_annotation(type)) {
        has_nonstandard_types = true;
      }
    });
    return has_nonstandard_types;
  }
  mstch::node rust_nonstandard_types() {
    // Sort/deduplicate by value of `rust.type` annotation.
    struct rust_type_less {
      bool operator()(const t_type* lhs, const t_type* rhs) const {
        auto& lhs_annotation = lhs->get_annotation("rust.type");
        auto& rhs_annotation = rhs->get_annotation("rust.type");
        if (lhs_annotation != rhs_annotation) {
          return lhs_annotation < rhs_annotation;
        }
        return lhs->get_full_name() < rhs->get_full_name();
      }
    };
    std::set<const t_type*, rust_type_less> nonstandard_types;
    foreach_type([&](const t_type* type) {
      if (has_nonstandard_type_annotation(type)) {
        nonstandard_types.insert(type);
      }
    });
    std::vector<const t_type*> elements(
        nonstandard_types.begin(), nonstandard_types.end());
    return generate_types(elements);
  }
  mstch::node rust_has_docs() { return program_->has_doc(); }
  mstch::node rust_docs() { return quoted_rust_doc(program_); }
  mstch::node rust_include_srcs() {
    mstch::array elements;
    for (auto elem : options_.include_srcs) {
      mstch::map node;
      node["program:include_src"] = elem;
      elements.push_back(node);
    }
    return elements;
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
            {"struct:exhaustive?", &mstch_rust_struct::rust_is_exhaustive},
            {"struct:fields_by_name", &mstch_rust_struct::rust_fields_by_name},
            {"struct:docs?", &mstch_rust_struct::rust_has_doc},
            {"struct:docs", &mstch_rust_struct::rust_doc},
            {"struct:derive", &mstch_rust_struct::rust_derive},
            {"struct:has_exception_message?",
             &mstch_rust_struct::has_exception_message},
            {"struct:is_exception_message_optional?",
             &mstch_rust_struct::is_exception_message_optional},
            {"struct:exception_message", &mstch_rust_struct::exception_message},
        });
  }
  mstch::node rust_name() {
    if (!strct_->has_annotation("rust.name")) {
      return mangle_type(strct_->get_name());
    }
    return strct_->get_annotation("rust.name");
  }
  mstch::node rust_package() {
    return get_import_name(strct_->program(), options_);
  }
  mstch::node rust_is_ord() {
    if (strct_->has_annotation("rust.ord")) {
      return true;
    }
    for (const auto& field : strct_->fields()) {
      if (!can_derive_ord(field.get_type())) {
        return false;
      }
    }
    return true;
  }
  mstch::node rust_is_copy() { return strct_->has_annotation("rust.copy"); }
  mstch::node rust_is_exhaustive() {
    return strct_->has_annotation("rust.exhaustive");
  }
  mstch::node rust_fields_by_name() {
    auto fields = strct_->fields().copy();
    std::sort(fields.begin(), fields.end(), [](auto a, auto b) {
      return a->get_name() < b->get_name();
    });
    return generate_fields(fields);
  }
  mstch::node rust_has_doc() { return strct_->has_doc(); }
  mstch::node rust_doc() { return quoted_rust_doc(strct_); }
  mstch::node rust_derive() {
    if (!strct_->has_annotation("rust.derive")) {
      return nullptr;
    }
    return strct_->get_annotation("rust.derive");
  }
  mstch::node has_exception_message() {
    return strct_->has_annotation("message");
  }
  mstch::node is_exception_message_optional() {
    if (!strct_->has_annotation("message")) {
      return nullptr;
    }
    return strct_->get_field_by_name(strct_->get_annotation("message"))
               ->get_req() == t_field::e_req::optional;
  }
  mstch::node exception_message() { return strct_->get_annotation("message"); }

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
            {"service:rust_exceptions",
             &mstch_rust_service::rust_all_exceptions},
            {"service:package", &mstch_rust_service::rust_package},
            {"service:snake", &mstch_rust_service::rust_snake},
            {"service:requestContext?",
             &mstch_rust_service::rust_request_context},
            {"service:extendedServices",
             &mstch_rust_service::rust_extended_services},
            {"service:docs?", &mstch_rust_service::rust_has_doc},
            {"service:docs", &mstch_rust_service::rust_doc},
        });
  }
  mstch::node rust_functions();
  mstch::node rust_package() {
    return get_import_name(service_->program(), options_);
  }
  mstch::node rust_snake() {
    return service_->get_annotation(
        "rust.mod", mangle_type(snakecase(service_->get_name())));
  }
  mstch::node rust_request_context() {
    return service_->has_annotation("rust.request_context");
  }
  mstch::node rust_extended_services() {
    mstch::array extended_services;
    const t_service* service = service_;
    std::string type_prefix = get_import_name(service_->program(), options_);
    std::string as_ref_impl = "&self.parent";
    while (true) {
      const t_service* parent_service = service->get_extends();
      if (parent_service == nullptr) {
        break;
      }
      if (parent_service->program() != service->program()) {
        type_prefix += "::dependencies::" + parent_service->program()->name();
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

  mstch::node rust_all_exceptions();
  mstch::node rust_has_doc() { return service_->has_doc(); }
  mstch::node rust_doc() { return quoted_rust_doc(service_); }

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
        function_upcamel_names_(function_upcamel_names),
        success_return(function->get_returntype(), "Success", 0) {
    register_methods(
        this,
        {
            {"function:rust_name", &mstch_rust_function::rust_name},
            {"function:upcamel", &mstch_rust_function::rust_upcamel},
            {"function:index", &mstch_rust_function::rust_index},
            {"function:void?", &mstch_rust_function::rust_void},
            {"function:uniqueExceptions",
             &mstch_rust_function::rust_unique_exceptions},
            {"function:uniqueStreamExceptions",
             &mstch_rust_function::rust_unique_stream_exceptions},
            {"function:args_by_name", &mstch_rust_function::rust_args_by_name},
            {"function:returns_by_name",
             &mstch_rust_function::rust_returns_by_name},
            {"function:docs?", &mstch_rust_function::rust_has_doc},
            {"function:docs", &mstch_rust_function::rust_doc},
        });
  }
  mstch::node rust_name() {
    if (!function_->has_annotation("rust.name")) {
      return mangle(function_->get_name());
    }
    return function_->get_annotation("rust.name");
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
  mstch::node rust_index() { return index_; }
  mstch::node rust_void() { return function_->get_returntype()->is_void(); }
  mstch::node rust_unique_exceptions() {
    return rust_make_unique_exceptions(function_->get_xceptions());
  }
  mstch::node rust_unique_stream_exceptions() {
    return rust_make_unique_exceptions(function_->get_stream_xceptions());
  }
  mstch::node rust_make_unique_exceptions(const t_struct* a) {
    // When generating From<> impls for an error type, we must not generate one
    // where more than one variant contains the same type of exception. Find
    // only those exceptions that map uniquely to a variant.

    const auto& exceptions = a->fields();
    std::map<const t_type*, unsigned> type_count;
    for (const auto& x : exceptions) {
      type_count[x.get_type()] += 1;
    }

    std::vector<const t_field*> unique_exceptions;
    for (const auto& x : exceptions) {
      if (type_count.at(x.get_type()) == 1) {
        unique_exceptions.emplace_back(&x);
      }
    }

    return generate_fields(unique_exceptions);
  }
  mstch::node rust_args_by_name() {
    auto params = function_->get_paramlist()->fields().copy();
    std::sort(params.begin(), params.end(), [](auto a, auto b) {
      return a->get_name() < b->get_name();
    });
    return generate_fields(params);
  }
  mstch::node rust_returns_by_name() {
    auto returns = function_->get_xceptions()->fields().copy();
    returns.push_back(&success_return);
    std::sort(returns.begin(), returns.end(), [](auto a, auto b) {
      return a->get_name() < b->get_name();
    });
    return generate_fields(returns);
  }
  mstch::node rust_has_doc() { return function_->has_doc(); }
  mstch::node rust_doc() { return quoted_rust_doc(function_); }

 private:
  int32_t index_;
  const std::unordered_multiset<std::string>& function_upcamel_names_;
  t_field success_return;
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
            {"enum_value:rust_name", &mstch_rust_enum_value::rust_name},
            {"enum_value:docs?", &mstch_rust_enum_value::rust_has_doc},
            {"enum_value:docs", &mstch_rust_enum_value::rust_doc},
        });
  }
  mstch::node rust_name() {
    if (!enm_value_->has_annotation("rust.name")) {
      return mangle(enm_value_->get_name());
    }
    return enm_value_->get_annotation("rust.name");
  }
  mstch::node rust_has_doc() { return enm_value_->has_doc(); }
  mstch::node rust_doc() { return quoted_rust_doc(enm_value_); }
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
            {"enum:variants_by_name", &mstch_rust_enum::variants_by_name},
            {"enum:variants_by_number", &mstch_rust_enum::variants_by_number},
            {"enum:docs?", &mstch_rust_enum::rust_has_doc},
            {"enum:docs", &mstch_rust_enum::rust_doc},
        });
  }
  mstch::node rust_name() {
    if (!enm_->has_annotation("rust.name")) {
      return mangle_type(enm_->get_name());
    }
    return enm_->get_annotation("rust.name");
  }
  mstch::node rust_package() {
    return get_import_name(enm_->program(), options_);
  }
  mstch::node variants_by_name() {
    std::vector<t_enum_value*> variants = enm_->get_enum_values();
    std::sort(variants.begin(), variants.end(), [](auto a, auto b) {
      return a->get_name() < b->get_name();
    });
    return generate_enum_values(variants);
  }
  mstch::node variants_by_number() {
    std::vector<t_enum_value*> variants = enm_->get_enum_values();
    std::sort(variants.begin(), variants.end(), [](auto a, auto b) {
      return a->get_value() < b->get_value();
    });
    return generate_enum_values(variants);
  }
  mstch::node rust_has_doc() { return enm_->has_doc(); }
  mstch::node rust_doc() { return quoted_rust_doc(enm_); }

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
            {"type:rust_name_snake", &mstch_rust_type::rust_name_snake},
            {"type:package", &mstch_rust_type::rust_package},
            {"type:rust", &mstch_rust_type::rust_type},
            {"type:nonstandard?", &mstch_rust_type::rust_nonstandard},
        });
  }
  mstch::node rust_name() {
    if (!type_->has_annotation("rust.name")) {
      return mangle_type(type_->get_name());
    }
    return type_->get_annotation("rust.name");
  }
  mstch::node rust_name_snake() {
    return snakecase(mangle_type(type_->get_name()));
  }
  mstch::node rust_package() {
    return get_import_name(type_->program(), options_);
  }
  mstch::node rust_type() {
    const std::string& rust_type = type_->get_annotation("rust.type");
    if (!rust_type.empty() && rust_type.find("::") == std::string::npos) {
      return "fbthrift::builtin_types::" + rust_type;
    }
    return rust_type;
  }
  mstch::node rust_nonstandard() {
    return has_nonstandard_type_annotation(type_) &&
        !(type_->is_typedef() && type_->has_annotation("rust.newtype"));
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
    while (type_->is_typedef() && !type_->has_annotation("rust.newtype")) {
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
            {"value:bool_value", &mstch_rust_value::bool_value},
            {"value:integer?", &mstch_rust_value::is_integer},
            {"value:integer_value", &mstch_rust_value::integer_value},
            {"value:floatingPoint?", &mstch_rust_value::is_floating_point},
            {"value:floatingPointValue",
             &mstch_rust_value::floating_point_value},
            {"value:string?", &mstch_rust_value::is_string},
            {"value:binary?", &mstch_rust_value::is_binary},
            {"value:quoted", &mstch_rust_value::string_quoted},
            {"value:list?", &mstch_rust_value::is_list},
            {"value:list_elements", &mstch_rust_value::list_elements},
            {"value:set?", &mstch_rust_value::is_set},
            {"value:setMembers", &mstch_rust_value::set_members},
            {"value:map?", &mstch_rust_value::is_map},
            {"value:mapEntries", &mstch_rust_value::map_entries},
            {"value:struct?", &mstch_rust_value::is_struct},
            {"value:structFields", &mstch_rust_value::struct_fields},
            {"value:exhaustive?", &mstch_rust_value::is_exhaustive},
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
    return type_->is_typedef() && type_->has_annotation("rust.newtype");
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
  mstch::node is_bool() { return type_->is_bool(); }
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
  mstch::node is_string() { return type_->is_string(); }
  mstch::node is_binary() { return type_->is_binary(); }
  mstch::node string_quoted() { return quote(const_value_->get_string()); }
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
  mstch::node set_members() { return list_elements(); }
  mstch::node is_map() {
    return type_->is_map() &&
        (const_value_->get_type() == value_type::CV_MAP ||
         (const_value_->get_type() == value_type::CV_LIST &&
          const_value_->get_list().empty()));
  }
  mstch::node map_entries();
  mstch::node is_struct() {
    return (type_->is_struct() || type_->is_xception()) && !type_->is_union() &&
        const_value_->get_type() == value_type::CV_MAP;
  }
  mstch::node struct_fields();
  mstch::node is_exhaustive();
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

    for (auto&& field : struct_type->fields()) {
      if (field.name() == variant) {
        return std::make_shared<mstch_rust_value>(
            content,
            field.get_type(),
            depth_ + 1,
            generators_,
            cache_,
            pos_,
            options_);
      }
    }
    return mstch::node();
  }
  mstch::node is_enum() { return type_->is_enum(); }
  mstch::node enum_package() {
    if (const_value_->is_enum()) {
      return get_import_name(const_value_->get_enum()->program(), options_);
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
  mstch::node indent() { return std::string(4 * depth_, ' '); }

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
            {"field:docs?", &mstch_rust_struct_field::rust_has_docs},
            {"field:docs", &mstch_rust_struct_field::rust_docs},
        });
  }
  mstch::node rust_name() {
    if (!field_->has_annotation("rust.name")) {
      return mangle(field_->get_name());
    }
    return field_->get_annotation("rust.name");
  }
  mstch::node is_optional() {
    return field_->get_req() == t_field::e_req::optional;
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
  mstch::node is_boxed() { return field_kind(*field_) == FieldKind::Box; }
  mstch::node is_arc() { return field_kind(*field_) == FieldKind::Arc; }
  mstch::node rust_has_docs() { return field_->has_doc(); }
  mstch::node rust_docs() { return quoted_rust_doc(field_); }

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
  for (auto&& field : struct_type->fields()) {
    auto value = map_entries[field.name()];
    if (!value) {
      value = field.default_value();
    }
    fields.push_back(std::make_shared<mstch_rust_struct_field>(
        &field, value, depth_ + 1, generators_, cache_, pos_, options_));
  }
  return fields;
}

mstch::node mstch_rust_value::is_exhaustive() {
  auto struct_type = dynamic_cast<const t_struct*>(type_);
  return struct_type && struct_type->has_annotation("rust.exhaustive");
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
            {"constant:docs?", &mstch_rust_const::rust_has_docs},
            {"constant:docs", &mstch_rust_const::rust_docs},
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
  mstch::node rust_has_docs() { return cnst_->has_doc(); }
  mstch::node rust_docs() { return quoted_rust_doc(cnst_); }

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
      field_generator_context const* field_context,
      const rust_codegen_options& options)
      : mstch_field(field, generators, cache, pos, index, field_context),
        options_(options) {
    register_methods(
        this,
        {
            {"field:rust_name", &mstch_rust_field::rust_name},
            {"field:primitive?", &mstch_rust_field::rust_primitive},
            {"field:rename?", &mstch_rust_field::rust_rename},
            {"field:default", &mstch_rust_field::rust_default},
            {"field:box?", &mstch_rust_field::rust_is_boxed},
            {"field:arc?", &mstch_rust_field::rust_is_arc},
            {"field:docs?", &mstch_rust_field::rust_has_docs},
            {"field:docs", &mstch_rust_field::rust_docs},
        });
  }
  mstch::node rust_name() {
    if (!field_->has_annotation("rust.name")) {
      return mangle(field_->get_name());
    }
    return field_->get_annotation("rust.name");
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
  mstch::node rust_is_boxed() { return field_kind(*field_) == FieldKind::Box; }
  mstch::node rust_is_arc() { return field_kind(*field_) == FieldKind::Arc; }
  mstch::node rust_has_docs() { return field_->has_doc(); }
  mstch::node rust_docs() { return quoted_rust_doc(field_); }

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
            {"typedef:rust_type", &mstch_rust_typedef::rust_type},
            {"typedef:nonstandard?", &mstch_rust_typedef::rust_nonstandard},
            {"typedef:docs?", &mstch_rust_typedef::rust_has_docs},
            {"typedef:docs", &mstch_rust_typedef::rust_docs},
        });
  }
  mstch::node rust_name() {
    if (!typedf_->has_annotation("rust.name")) {
      return mangle_type(typedf_->name());
    }
    return typedf_->get_annotation("rust.name");
  }
  mstch::node rust_newtype() { return typedf_->has_annotation("rust.newtype"); }
  mstch::node rust_type() {
    const std::string& rust_type = typedf_->get_annotation("rust.type");
    if (!rust_type.empty() && rust_type.find("::") == std::string::npos) {
      return "fbthrift::builtin_types::" + rust_type;
    }
    return rust_type;
  }
  mstch::node rust_ord() {
    return typedf_->has_annotation("rust.ord") ||
        can_derive_ord(typedf_->get_type());
  }
  mstch::node rust_copy() {
    auto inner = typedf_->get_true_type();
    if (inner->is_bool() || inner->is_byte() || inner->is_i16() ||
        inner->is_i32() || inner->is_i64() || inner->is_enum() ||
        inner->is_void()) {
      return true;
    }
    if (typedf_->has_annotation("rust.copy")) {
      return true;
    }
    return false;
  }
  mstch::node rust_nonstandard() {
    return typedf_->get_annotation("rust.type").find("::") != string::npos;
  }
  mstch::node rust_has_docs() { return typedf_->has_doc(); }
  mstch::node rust_docs() { return quoted_rust_doc(typedf_); }
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
            annotation.first,
            annotation.second,
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
  mstch::node rust_has_value() { return !val_.value.empty(); }
  mstch::node rust_name() {
    return boost::algorithm::replace_all_copy(key_, ".", "_");
  }
  mstch::node rust_value() { return quote(val_.value); }
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
      service_->get_functions(), &function_generator, function_upcamel_names_);
}

class field_rust_generator : public field_generator {
 public:
  explicit field_rust_generator(const rust_codegen_options& options)
      : options_(options) {}

  std::shared_ptr<mstch_base> generate(
      const t_field* field,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      int32_t index,
      field_generator_context const* field_context = nullptr) const override {
    return std::make_shared<mstch_rust_field>(
        field, generators, cache, pos, index, field_context, options_);
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

mstch::node mstch_rust_service::rust_all_exceptions() {
  std::map<const t_type*, std::vector<const t_function*>> function_map;
  std::map<const t_type*, std::vector<const t_field*>> field_map;
  for (const auto& fun : service_->functions()) {
    for (const auto& fld : fun.get_xceptions()->fields()) {
      function_map[fld.type()->get_true_type()].push_back(&fun);
      field_map[fld.type()->get_true_type()].push_back(&fld);
    }
  }

  mstch::array output;
  for (const auto& funcs : function_map) {
    mstch::map data;
    type_rust_generator gen(options_);
    data["rust_exception:type"] = gen.generate(
        funcs.first, generators_, cache_, ELEMENT_POSITION::NONE, 0);

    function_rust_generator function_generator;

    auto functions = generate_elements(
        funcs.second, &function_generator, function_upcamel_names_);
    auto fields = generate_fields(field_map[funcs.first]);

    mstch::array function_data;
    for (size_t i = 0; i < fields.size(); i++) {
      mstch::map inner;
      inner["rust_exception_function:function"] = std::move(functions[i]);
      inner["rust_exception_function:field"] = std::move(fields[i]);
      function_data.push_back(std::move(inner));
    }

    data["rust_exception:functions"] = std::move(function_data);
    output.push_back(data);
  }

  return output;
}

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
  const auto& prog = cached_program(program);

  render_to_file(prog, "lib.rs", "lib.rs");
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
  for (auto& field : s->fields()) {
    bool box = field.has_annotation("rust.box");
    bool arc = field.has_annotation("rust.arc");
    if (box && arc) {
      add_error(
          field.lineno(),
          "Field `" + field.name() + "` cannot be both Box'ed and Arc'ed");
    }
  }
  return true;
}
} // namespace

void t_mstch_rust_generator::fill_validator_list(validator_list& l) const {
  l.add<annotation_validator>();
}

THRIFT_REGISTER_GENERATOR(
    mstch_rust,
    "Rust",
    "    serde:           Derive serde Serialize/Deserialize traits for types\n"
    "    noserver:        Don't emit server code\n"
    "    include_prefix=: Set program:include_prefix.\n"
    "    include_srcs=:   Additional Rust source file to include in output, `:` separated\n"
    "    cratemap=map:    Mapping file from services to crate names\n");
} // namespace compiler
} // namespace thrift
} // namespace apache
