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

#include <cctype>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include <thrift/compiler/ast/ast_visitor.h>
#include <thrift/compiler/ast/t_container.h>
#include <thrift/compiler/ast/t_enum.h>
#include <thrift/compiler/ast/t_primitive_type.h>
#include <thrift/compiler/ast/t_program_bundle.h>
#include <thrift/compiler/ast/t_structured.h>
#include <thrift/compiler/codemod/codemod.h>
#include <thrift/compiler/codemod/file_manager.h>
#include <thrift/compiler/generate/cpp/name_resolver.h>

using apache::thrift::compiler::const_ast_visitor;
using apache::thrift::compiler::cpp_name_resolver;
using apache::thrift::compiler::kCppAdapterUri;
using apache::thrift::compiler::kCppTypeUri;
using apache::thrift::compiler::source_location;
using apache::thrift::compiler::source_manager;
using apache::thrift::compiler::source_range;
using apache::thrift::compiler::t_const;
using apache::thrift::compiler::t_container;
using apache::thrift::compiler::t_field;
using apache::thrift::compiler::t_list;
using apache::thrift::compiler::t_map;
using apache::thrift::compiler::t_named;
using apache::thrift::compiler::t_primitive_type;
using apache::thrift::compiler::t_program;
using apache::thrift::compiler::t_program_bundle;
using apache::thrift::compiler::t_set;
using apache::thrift::compiler::t_type;
using apache::thrift::compiler::t_type_ref;
using apache::thrift::compiler::t_typedef;
namespace codemod = apache::thrift::compiler::codemod;

namespace {

struct template_instantiation {
  std::string name;
  std::vector<std::string> arguments;
};

struct generated_typedef {
  std::string annotation_key;
  std::string annotation_value;
  std::string thrift_type;
  std::string name;
};

// Maps (Thrift type structure, canonical C++ type) to a typedef name.
using typedef_key = std::pair<std::string, std::string>;

std::string_view trim(std::string_view value) {
  while (!value.empty() &&
         std::isspace(static_cast<unsigned char>(value.front()))) {
    value.remove_prefix(1);
  }
  while (!value.empty() &&
         std::isspace(static_cast<unsigned char>(value.back()))) {
    value.remove_suffix(1);
  }
  return value;
}

std::optional<template_instantiation> parse_template_instantiation(
    std::string_view value) {
  value = trim(value);
  const size_t open = value.find('<');
  if (open == std::string_view::npos || trim(value.substr(0, open)).empty()) {
    return std::nullopt;
  }

  int depth = 0;
  size_t argument_begin = open + 1;
  template_instantiation result{std::string(trim(value.substr(0, open))), {}};
  for (size_t i = open; i < value.size(); ++i) {
    switch (value[i]) {
      case '<':
        ++depth;
        break;
      case '>':
        if (--depth < 0) {
          return std::nullopt;
        }
        if (depth == 0) {
          auto argument =
              trim(value.substr(argument_begin, i - argument_begin));
          if (argument.empty() || !trim(value.substr(i + 1)).empty()) {
            return std::nullopt;
          }
          result.arguments.emplace_back(argument);
          return result;
        }
        break;
      case ',':
        if (depth == 1) {
          auto argument =
              trim(value.substr(argument_begin, i - argument_begin));
          if (argument.empty()) {
            return std::nullopt;
          }
          result.arguments.emplace_back(argument);
          argument_begin = i + 1;
        }
        break;
    }
  }
  return std::nullopt;
}

bool is_identifier_character(char value) {
  return std::isalnum(static_cast<unsigned char>(value)) || value == '_';
}

std::string join(
    std::vector<std::string>::const_iterator begin,
    std::vector<std::string>::const_iterator end,
    std::string_view separator) {
  std::string result;
  for (auto it = begin; it != end; ++it) {
    if (it != begin) {
      result += separator;
    }
    result += *it;
  }
  return result;
}

std::string join(
    const std::vector<std::string>& values, std::string_view separator) {
  return join(values.begin(), values.end(), separator);
}

std::string_view strip_global_qualifier(std::string_view name) {
  if (name.starts_with("::")) {
    name.remove_prefix(2);
  }
  return name;
}

std::vector<const t_type_ref*> element_types(const t_container& container) {
  if (const auto* list = container.try_as<t_list>()) {
    return {&list->elem_type()};
  }
  if (const auto* set = container.try_as<t_set>()) {
    return {&set->elem_type()};
  }
  const auto& map = container.as<t_map>();
  return {&map.key_type(), &map.val_type()};
}

bool is_resolved(const t_type_ref& type_ref) {
  if (!type_ref.resolved()) {
    return false;
  }
  if (const auto* type = type_ref->try_as<t_typedef>()) {
    return is_resolved(type->type());
  }
  if (const auto* container = type_ref->try_as<t_container>()) {
    for (const t_type_ref* element : element_types(*container)) {
      if (!is_resolved(*element)) {
        return false;
      }
    }
  }
  return true;
}

enum class cpp_token_kind { identifier, number, scope, open, close, comma };

struct cpp_token {
  cpp_token_kind kind;
  std::string_view text;
};

std::optional<std::vector<cpp_token>> tokenize_cpp_type(std::string_view text) {
  std::vector<cpp_token> tokens;
  for (size_t i = 0; i < text.size();) {
    const char c = text[i];
    if (std::isspace(static_cast<unsigned char>(c))) {
      ++i;
    } else if (is_identifier_character(c)) {
      size_t end = i;
      while (end < text.size() && is_identifier_character(text[end])) {
        ++end;
      }
      tokens.push_back(
          {std::isdigit(static_cast<unsigned char>(c))
               ? cpp_token_kind::number
               : cpp_token_kind::identifier,
           text.substr(i, end - i)});
      i = end;
    } else if (text.substr(i, 2) == "::") {
      tokens.push_back({cpp_token_kind::scope, text.substr(i, 2)});
      i += 2;
    } else if (c == '<' || c == '>' || c == ',') {
      tokens.push_back(
          {c == '<'       ? cpp_token_kind::open
               : c == '>' ? cpp_token_kind::close
                          : cpp_token_kind::comma,
           text.substr(i, 1)});
      ++i;
    } else {
      return std::nullopt;
    }
  }
  return tokens;
}

bool is_integer_keyword(std::string_view word) {
  return word == "signed" || word == "unsigned" || word == "short" ||
      word == "long" || word == "int" || word == "char";
}

// Canonicalizes C++ type spellings so that two spellings with equal canonical
// forms denote the same type. Anything that cannot be resolved with certainty
// (e.g. an unqualified name that is not a known Thrift definition) fails.
//
// Names are looked up like C++ unqualified name lookup from the given
// namespace scope, but only against namespaces and types generated by Thrift;
// qualified names that do not start with a Thrift-known namespace component in
// any enclosing scope are treated as fully qualified.
//
// Integer spellings assume `int`/`short`/`signed char` are the 32/16/8-bit
// fixed-width types. `long` and `long long` are kept distinct from `int64_t`
// since the latter is either one depending on the platform.
class cpp_type_canonicalizer {
 public:
  cpp_type_canonicalizer(
      cpp_name_resolver& resolver, const t_program_bundle& bundle)
      : resolver_(resolver) {
    for (const t_program& program : bundle.programs()) {
      std::vector<std::string> components =
          cpp_name_resolver::gen_namespace_components(program);
      for (size_t i = 1; i <= components.size(); ++i) {
        namespaces_.insert(
            join(components.begin(), components.begin() + i, "::"));
      }
      auto add = [&](const t_type& type, const std::string& name) {
        types_.try_emplace(std::string(strip_global_qualifier(name)), &type);
      };
      for (const t_typedef* type : program.typedefs()) {
        add(*type, resolver_.get_namespaced_name(*type));
      }
      for (const auto* type : program.enums()) {
        add(*type, resolver_.get_namespaced_name(*type));
      }
      for (const auto* type : program.structured_definitions()) {
        add(*type, resolver_.get_namespaced_name(*type));
        // The underlying class of an adapted struct may shadow other names.
        add(*type, resolver_.get_underlying_namespaced_name(*type));
        if (const std::string* extra = resolver_.get_extra_namespace(*type)) {
          namespaces_.insert(join(components, "::") + "::" + *extra);
        }
      }
    }
  }

  std::optional<std::string> canonicalize(
      std::string_view type, const std::vector<std::string>& scope) {
    constexpr int kMaxDepth = 64;
    if (depth_ >= kMaxDepth) {
      return std::nullopt;
    }
    const auto tokens = tokenize_cpp_type(type);
    if (!tokens) {
      return std::nullopt;
    }
    size_t pos = 0;
    ++depth_;
    auto result = parse_type(*tokens, pos, scope);
    --depth_;
    if (!result || pos != tokens->size()) {
      return std::nullopt;
    }
    return result;
  }

  bool is_known(const std::string& name) const {
    return types_.contains(name) || namespaces_.contains(name);
  }

 private:
  static std::optional<std::string> parse_integer(
      const std::vector<cpp_token>& tokens, size_t& pos) {
    std::map<std::string_view, int> counts;
    while (pos < tokens.size() &&
           tokens[pos].kind == cpp_token_kind::identifier &&
           is_integer_keyword(tokens[pos].text)) {
      ++counts[tokens[pos++].text];
    }
    const int sign = counts["signed"] + counts["unsigned"];
    const int shorts = counts["short"];
    const int longs = counts["long"];
    if (sign > 1 || counts["int"] > 1 || counts["char"] > 1 || shorts > 1 ||
        longs > 2 || (shorts > 0 && longs > 0)) {
      return std::nullopt;
    }
    const bool is_unsigned = counts["unsigned"] == 1;
    if (counts["char"] == 1) {
      if (shorts > 0 || longs > 0 || counts["int"] > 0) {
        return std::nullopt;
      }
      return sign == 0 ? "char" : is_unsigned ? "uint8_t" : "int8_t";
    }
    if (shorts == 1) {
      return is_unsigned ? "uint16_t" : "int16_t";
    }
    if (longs > 0) {
      return std::string(is_unsigned ? "unsigned " : "") +
          (longs == 2 ? "long long" : "long");
    }
    return is_unsigned ? "uint32_t" : "int32_t";
  }

  std::optional<std::string> parse_type(
      const std::vector<cpp_token>& tokens,
      size_t& pos,
      const std::vector<std::string>& scope) {
    auto at = [&](cpp_token_kind kind) {
      return pos < tokens.size() && tokens[pos].kind == kind;
    };
    if (at(cpp_token_kind::identifier) &&
        is_integer_keyword(tokens[pos].text)) {
      return parse_integer(tokens, pos);
    }

    const bool absolute = at(cpp_token_kind::scope);
    if (absolute) {
      ++pos;
    }
    std::vector<std::string> path;
    while (true) {
      if (!at(cpp_token_kind::identifier)) {
        return std::nullopt;
      }
      path.emplace_back(tokens[pos++].text);
      if (!at(cpp_token_kind::scope)) {
        break;
      }
      ++pos;
    }

    std::vector<std::string> arguments;
    const bool has_arguments = at(cpp_token_kind::open);
    if (has_arguments) {
      ++pos;
      while (true) {
        if (at(cpp_token_kind::number)) {
          arguments.emplace_back(tokens[pos++].text);
        } else if (auto argument = parse_type(tokens, pos, scope)) {
          arguments.push_back(std::move(*argument));
        } else {
          return std::nullopt;
        }
        if (at(cpp_token_kind::comma)) {
          ++pos;
          continue;
        }
        if (!at(cpp_token_kind::close)) {
          return std::nullopt;
        }
        ++pos;
        break;
      }
    }

    auto name = resolve_name(path, absolute, has_arguments, scope);
    if (!name || !has_arguments) {
      return name;
    }
    return *name + "<" + join(arguments, ",") + ">";
  }

  std::optional<std::string> resolve_name(
      const std::vector<std::string>& path,
      bool absolute,
      bool has_arguments,
      const std::vector<std::string>& scope) {
    const std::string relative = join(path, "::");
    std::string name;
    if (!absolute) {
      for (size_t k = scope.size(); k > 0 && name.empty(); --k) {
        const std::string prefix =
            join(scope.begin(), scope.begin() + k, "::") + "::";
        if (is_known(prefix + path.front())) {
          name = prefix;
          name += relative;
        }
      }
    }
    if (name.empty()) {
      name = relative;
    }

    if (auto it = types_.find(name); it != types_.end()) {
      // Generated as `using <name> = <underlying type>;` in the namespace of
      // the typedef's program.
      const auto* type = it->second->try_as<t_typedef>();
      const t_program* program = type == nullptr ? nullptr : type->program();
      if (program != nullptr && !has_arguments && is_resolved(type->type()) &&
          cpp_name_resolver::get_cpp_name(*type) == type->name()) {
        if (auto underlying = canonicalize(
                resolver_.get_underlying_type_name(*type),
                cpp_name_resolver::gen_namespace_components(*program))) {
          return underlying;
        }
      }
      return name;
    }

    static const std::unordered_set<std::string_view> fixed_width_integers = {
        "int8_t",
        "int16_t",
        "int32_t",
        "int64_t",
        "uint8_t",
        "uint16_t",
        "uint32_t",
        "uint64_t",
    };
    if (name.starts_with("std::") &&
        fixed_width_integers.contains(std::string_view(name).substr(5))) {
      return name.substr(5);
    }
    if (fixed_width_integers.contains(name) || name == "bool" ||
        name == "float" || name == "double" || name == "void") {
      return name;
    }
    if (!absolute && path.size() == 1) {
      return std::nullopt;
    }
    return name;
  }

  cpp_name_resolver& resolver_;
  std::unordered_map<std::string, const t_type*> types_;
  std::unordered_set<std::string> namespaces_;
  int depth_ = 0;
};

std::optional<int> fixed_width_cpp_integer(std::string_view canonical_type) {
  if (canonical_type == "int8_t" || canonical_type == "uint8_t") {
    return 8;
  }
  if (canonical_type == "int16_t" || canonical_type == "uint16_t") {
    return 16;
  }
  if (canonical_type == "int32_t" || canonical_type == "uint32_t") {
    return 32;
  }
  if (canonical_type == "int64_t" || canonical_type == "uint64_t") {
    return 64;
  }
  return std::nullopt;
}

std::optional<int> thrift_integer_width(const t_type& type) {
  const auto* primitive = type.try_as<t_primitive_type>();
  if (primitive == nullptr) {
    return std::nullopt;
  }
  switch (primitive->primitive_type()) {
    case t_primitive_type::type::t_byte:
      return 8;
    case t_primitive_type::type::t_i16:
      return 16;
    case t_primitive_type::type::t_i32:
      return 32;
    case t_primitive_type::type::t_i64:
      return 64;
    default:
      return std::nullopt;
  }
}

// Thrift type with typedefs (and hence their annotations) erased.
std::string thrift_structure(const t_type& type) {
  const t_type* true_type = type.get_true_type();
  if (true_type == nullptr) {
    return type.get_full_name();
  }
  if (const auto* container = true_type->try_as<t_container>()) {
    std::vector<std::string> elements;
    for (const t_type_ref* element : element_types(*container)) {
      elements.push_back(thrift_structure(**element));
    }
    return std::string(
               container->is<t_list>()      ? "list"
                   : container->is<t_set>() ? "set"
                                            : "map") +
        "<" + join(elements, ",") + ">";
  }
  return true_type->get_full_name();
}

std::string make_typedef_name(std::string_view canonical_type) {
  std::string name;
  for (size_t i = 0; i < canonical_type.size();) {
    if (!is_identifier_character(canonical_type[i])) {
      ++i;
      continue;
    }
    size_t end = i;
    while (end < canonical_type.size() &&
           is_identifier_character(canonical_type[end])) {
      ++end;
    }
    std::string_view word = canonical_type.substr(i, end - i);
    if (word.ends_with("_t")) {
      word.remove_suffix(2);
    }
    if (!name.empty()) {
      name += '_';
    }
    name += word;
    i = end;
  }
  return name;
}

std::optional<size_t> find_closing_quote(
    std::string_view value, size_t opening_quote) {
  bool escaped = false;
  for (size_t i = opening_quote + 1; i < value.size(); ++i) {
    if (value[i] == value[opening_quote] && !escaped) {
      return i;
    }
    escaped = value[i] == '\\' && !escaped;
  }
  return std::nullopt;
}

// Replaces the `name = "..."` entry of a `@cpp.Type` annotation's source text
// with `template = "<template_name>"`.
std::optional<std::string> replace_name_with_template(
    std::string replacement, std::string_view template_name) {
  size_t key_pos = 0;
  while ((key_pos = replacement.find("name", key_pos)) != std::string::npos) {
    const bool starts_identifier =
        key_pos > 0 && is_identifier_character(replacement[key_pos - 1]);
    const size_t key_end = key_pos + std::string_view("name").size();
    const bool ends_identifier = key_end < replacement.size() &&
        is_identifier_character(replacement[key_end]);
    size_t equals = key_end;
    while (equals < replacement.size() &&
           std::isspace(static_cast<unsigned char>(replacement[equals]))) {
      ++equals;
    }
    if (!starts_identifier && !ends_identifier && equals < replacement.size() &&
        replacement[equals] == '=') {
      const size_t quote = replacement.find_first_of("\"'", equals + 1);
      if (quote == std::string::npos) {
        return std::nullopt;
      }
      const auto quote_end = find_closing_quote(replacement, quote);
      if (!quote_end) {
        return std::nullopt;
      }
      replacement.replace(quote + 1, *quote_end - quote - 1, template_name);
      replacement.replace(key_pos, std::string_view("name").size(), "template");
      return replacement;
    }
    key_pos = key_end;
  }
  return std::nullopt;
}

// Typedefs to be generated for a single annotation rewrite, committed only if
// the whole rewrite succeeds.
struct rewrite_plan {
  std::vector<generated_typedef> typedefs;
  std::map<typedef_key, std::string> typedef_names;
  std::set<std::string> names;
};

struct element_rewrite {
  std::string text;
  bool changed = false;
};

class container_type_migrator {
 public:
  container_type_migrator(
      codemod::file_manager& file_manager, t_program_bundle& bundle)
      : file_manager_(file_manager),
        canonicalizer_(resolver_, bundle),
        scope_(
            cpp_name_resolver::gen_namespace_components(
                *bundle.root_program())) {
    const t_program& program = *bundle.root_program();
    for (const t_program& other : bundle.programs()) {
      if (&other != &program &&
          cpp_name_resolver::gen_namespace_components(other) != scope_) {
        continue;
      }
      for (const t_named& definition : other.definitions()) {
        used_names_.insert(definition.name());
      }
    }

    // Fields of structs generated in an extra namespace are declared there.
    for (const auto* structured : program.structured_definitions()) {
      const std::string* extra = resolver_.get_extra_namespace(*structured);
      if (extra == nullptr) {
        continue;
      }
      std::vector<std::string> field_scope = scope_;
      std::string_view rest = *extra;
      for (size_t pos; (pos = rest.find("::")) != std::string_view::npos;
           rest.remove_prefix(pos + 2)) {
        field_scope.emplace_back(rest.substr(0, pos));
      }
      field_scope.emplace_back(rest);
      for (const t_field& field : structured->fields()) {
        field_scopes_.emplace(&field, field_scope);
      }
    }

    for (const t_typedef* type : program.typedefs()) {
      if (type->structured_annotations().size() != 1 ||
          !type->unstructured_annotations().empty() ||
          !is_resolved(type->type())) {
        continue;
      }
      const t_const* annotation =
          type->find_structured_annotation_or_null(kCppTypeUri);
      if (annotation == nullptr || annotation->value()->get_map().size() != 1) {
        continue;
      }
      if (auto canonical = canonicalizer_.canonicalize(
              resolver_.get_underlying_type_name(*type), scope_)) {
        typedef_names_.try_emplace(
            typedef_key{thrift_structure(*type->type()), *canonical},
            type->name());
      }
    }
  }

  void migrate(
      const t_named& node, const t_type& type, const t_type_ref* type_ref) {
    const t_type* true_type = type.get_true_type();
    if (true_type == nullptr) {
      return;
    }
    const auto* container = true_type->try_as<t_container>();
    const t_const* annotation =
        node.find_structured_annotation_or_null(kCppTypeUri);
    if (container == nullptr || annotation == nullptr ||
        annotation->value()->get_map().size() != 1) {
      return;
    }
    // References to an adapted typedef resolve differently with `template`
    // than with `name`.
    if (dynamic_cast<const t_typedef*>(&node) != nullptr &&
        node.has_structured_annotation(kCppAdapterUri)) {
      return;
    }

    const auto* name =
        annotation->get_value_from_structured_annotation_or_null("name");
    if (name == nullptr) {
      return;
    }
    const auto parsed = parse_template_instantiation(name->get_string());
    if (!parsed) {
      return;
    }
    const auto elements = element_types(*container);
    if (parsed->arguments.size() != elements.size()) {
      return;
    }

    auto field_scope = field_scopes_.find(&node);
    lookup_scope_ =
        field_scope == field_scopes_.end() ? scope_ : field_scope->second;

    // Element types can only be rewritten if spelled inline in this node.
    const bool editable = type_ref != nullptr && type_ref->resolved() &&
        (*type_ref)->try_as<t_container>() == container;
    rewrite_plan plan;
    std::vector<codemod::replacement> replacements;
    for (size_t i = 0; i < elements.size(); ++i) {
      auto rewrite =
          rewrite_element(*elements[i], parsed->arguments[i], editable, plan);
      if (!rewrite) {
        return;
      }
      if (rewrite->changed) {
        const source_range range = elements[i]->src_range();
        replacements.push_back(
            {file_manager_.to_offset(range.begin),
             file_manager_.to_offset(range.end),
             std::move(rewrite->text)});
      }
    }

    const source_range range = annotation->src_range();
    const size_t begin = file_manager_.to_offset(range.begin);
    const size_t end = file_manager_.to_offset(range.end);
    auto annotation_replacement = replace_name_with_template(
        std::string(file_manager_.old_content().substr(begin, end - begin)),
        parsed->name);
    if (!annotation_replacement) {
      return;
    }

    for (auto& replacement : replacements) {
      file_manager_.add(std::move(replacement));
    }
    file_manager_.add({begin, end, std::move(*annotation_replacement)});
    typedef_names_.merge(plan.typedef_names);
    used_names_.merge(plan.names);
    for (auto& generated : plan.typedefs) {
      generated_typedefs_.push_back(std::move(generated));
    }
  }

  void add_generated_typedefs() {
    if (generated_typedefs_.empty()) {
      return;
    }
    std::string definitions = "\n";
    for (const auto& type : generated_typedefs_) {
      definitions += "@cpp.Type{" + type.annotation_key + " = \"" +
          type.annotation_value + "\"}\n";
      definitions += "typedef " + type.thrift_type + " " + type.name + "\n\n";
    }
    definitions.pop_back();
    file_manager_.add(
        {file_manager_.old_content().size(),
         file_manager_.old_content().size(),
         std::move(definitions)});
  }

 private:
  std::string_view source_text(const source_range& range) const {
    const size_t begin = file_manager_.to_offset(range.begin);
    const size_t end = file_manager_.to_offset(range.end);
    return file_manager_.old_content().substr(begin, end - begin);
  }

  // Returns the Thrift spelling of `type_ref` for which Thrift generates
  // exactly the C++ type `cpp_type`, extracting annotated typedefs as needed.
  std::optional<element_rewrite> rewrite_element(
      const t_type_ref& type_ref,
      std::string_view cpp_type,
      bool editable,
      rewrite_plan& plan) {
    if (!is_resolved(type_ref)) {
      return std::nullopt;
    }
    const t_type& type = *type_ref;
    const auto expected = canonicalizer_.canonicalize(cpp_type, lookup_scope_);
    if (!expected) {
      return std::nullopt;
    }
    const auto generated = canonicalizer_.canonicalize(
        resolver_.get_native_type(type), lookup_scope_);
    if (generated == expected) {
      return element_rewrite{
          editable ? std::string(source_text(type_ref.src_range()))
                   : std::string(),
          false};
    }
    if (!editable || type_ref.src_range().begin == source_location{}) {
      return std::nullopt;
    }

    // Generated typedefs are spelled in the file's namespace scope.
    const auto cpp_width = fixed_width_cpp_integer(*expected);
    if (cpp_width && cpp_width == thrift_integer_width(type) &&
        canonicalizer_.canonicalize(*expected, scope_) == expected) {
      return element_rewrite{
          get_typedef_name(
              plan, type, *expected, {"name", *expected, type.name(), ""}),
          true};
    }

    const auto* container = type.try_as<t_container>();
    const auto parsed = parse_template_instantiation(cpp_type);
    if (container == nullptr || !parsed) {
      return std::nullopt;
    }
    const auto elements = element_types(*container);
    if (parsed->arguments.size() != elements.size()) {
      return std::nullopt;
    }

    const source_range range = type_ref.src_range();
    const size_t begin = file_manager_.to_offset(range.begin);
    std::string text(source_text(range));
    size_t shift = 0;
    for (size_t i = 0; i < elements.size(); ++i) {
      auto rewrite =
          rewrite_element(*elements[i], parsed->arguments[i], true, plan);
      if (!rewrite) {
        return std::nullopt;
      }
      if (rewrite->changed) {
        const source_range element_range = elements[i]->src_range();
        const size_t element_begin =
            file_manager_.to_offset(element_range.begin) - begin + shift;
        const size_t element_size = file_manager_.to_offset(element_range.end) -
            file_manager_.to_offset(element_range.begin);
        text.replace(element_begin, element_size, rewrite->text);
        shift += rewrite->text.size() - element_size;
      }
    }

    const auto template_name =
        canonicalizer_.canonicalize(parsed->name, lookup_scope_);
    const auto default_template =
        parse_template_instantiation(resolver_.get_native_type(*container));
    if (default_template &&
        template_name ==
            canonicalizer_.canonicalize(
                default_template->name, lookup_scope_)) {
      return element_rewrite{std::move(text), true};
    }
    if (!template_name ||
        canonicalizer_.canonicalize(parsed->name, scope_) != template_name) {
      return std::nullopt;
    }
    return element_rewrite{
        get_typedef_name(
            plan,
            type,
            *expected,
            {"template", parsed->name, std::move(text), ""}),
        true};
  }

  std::string get_typedef_name(
      rewrite_plan& plan,
      const t_type& type,
      const std::string& canonical_type,
      generated_typedef definition) {
    const typedef_key key{thrift_structure(type), canonical_type};
    if (auto it = typedef_names_.find(key); it != typedef_names_.end()) {
      return it->second;
    }
    if (auto it = plan.typedef_names.find(key);
        it != plan.typedef_names.end()) {
      return it->second;
    }
    const std::string base = make_typedef_name(canonical_type);
    std::string name = base;
    for (int i = 2; used_names_.contains(name) || plan.names.contains(name) ||
         canonicalizer_.is_known(join(scope_, "::") + "::" + name);
         ++i) {
      name = base + "_" + std::to_string(i);
    }
    definition.name = name;
    plan.names.insert(name);
    plan.typedef_names.emplace(key, name);
    plan.typedefs.push_back(std::move(definition));
    return name;
  }

  codemod::file_manager& file_manager_;
  cpp_name_resolver resolver_;
  cpp_type_canonicalizer canonicalizer_;
  std::vector<std::string> scope_;
  std::map<const t_named*, std::vector<std::string>> field_scopes_;
  std::vector<std::string> lookup_scope_;
  std::set<std::string> used_names_;
  std::map<typedef_key, std::string> typedef_names_;
  std::vector<generated_typedef> generated_typedefs_;
};

} // namespace

int main(int argc, char** argv) {
  return apache::thrift::compiler::run_codemod(
      argc, argv, [](source_manager& source_manager, t_program_bundle& bundle) {
        t_program& program = *bundle.root_program();
        codemod::file_manager file_manager(source_manager, program);
        container_type_migrator migrator(file_manager, bundle);

        const_ast_visitor visitor;
        visitor.add_field_visitor([&](const t_field& field) {
          if (is_resolved(field.type())) {
            migrator.migrate(field, *field.type(), &field.type());
          }
        });
        visitor.add_typedef_visitor([&](const t_typedef& type) {
          if (is_resolved(type.type())) {
            migrator.migrate(type, *type.type(), &type.type());
          }
        });
        visitor.add_container_visitor([&](const t_container& container) {
          migrator.migrate(container, container, nullptr);
        });
        visitor(program);

        migrator.add_generated_typedefs();
        file_manager.apply_replacements();
      });
}
