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

#pragma once

#include <cstddef>
#include <limits>
#include <memory>
#include <set>
#include <stack>
#include <string>
#include <unordered_set>
#include <vector>

#include <boost/optional.hpp>

#include <thrift/compiler/ast/diagnostic_context.h>
#include <thrift/compiler/ast/node_list.h>
#include <thrift/compiler/ast/t_const_value.h>
#include <thrift/compiler/ast/t_exception.h>
#include <thrift/compiler/ast/t_field.h>
#include <thrift/compiler/ast/t_interaction.h>
#include <thrift/compiler/ast/t_named.h>
#include <thrift/compiler/ast/t_node.h>
#include <thrift/compiler/ast/t_package.h>
#include <thrift/compiler/ast/t_program.h>
#include <thrift/compiler/ast/t_program_bundle.h>
#include <thrift/compiler/ast/t_scope.h>
#include <thrift/compiler/ast/t_union.h>
#include <thrift/compiler/parse/t_ref.h>

// This is a macro because of a difference between the OSS and internal builds.
#ifndef LOCATION_HH
#define LOCATION_HH "thrift/compiler/parse/location.hh"
#endif
#include LOCATION_HH

namespace apache {
namespace thrift {
namespace compiler {

class lexer;

namespace yy {
class parser;
}

// Parsing only representations.
struct t_annotations {
  std::map<std::string, annotation_value> strings;
  std::map<std::string, std::shared_ptr<const t_const>> objects;
};
using t_doc = boost::optional<std::string>;
// TODO (partisan): Rename to t_stmt_attrs.
struct t_def_attrs {
  t_doc doc;
  std::unique_ptr<node_list<t_const>> struct_annotations;
};

template <typename T>
class t_ref;

enum class parsing_mode {
  INCLUDES = 1,
  PROGRAM = 2,
};

struct parsing_params {
  // Default values are taken from the original global variables.

  parsing_params() noexcept {} // Disable aggregate initialization

  /**
   * Strictness level
   */
  int strict = 127;

  /**
   * Whether or not negative field keys are accepted.
   *
   * When a field does not have a user-specified key, thrift automatically
   * assigns a negative value.  However, this is fragile since changes to the
   * file may unintentionally change the key numbering, resulting in a new
   * protocol that is not backwards compatible.
   *
   * When allow_neg_field_keys is enabled, users can explicitly specify
   * negative keys.  This way they can write a .thrift file with explicitly
   * specified keys that is still backwards compatible with older .thrift files
   * that did not specify key values.
   */
  bool allow_neg_field_keys = false;

  /**
   * Whether or not 64-bit constants will generate a warning.
   *
   * Some languages don't support 64-bit constants, but many do, so we can
   * suppress this warning for projects that don't use any non-64-bit-safe
   * languages.
   */
  bool allow_64bit_consts = false;

  /**
   * Which experimental features should be allowed.
   *
   * 'all' can be used to enable all experimental features.
   */
  std::unordered_set<std::string> allow_experimental_features;

  /**
   * Search path for inclusions
   */
  std::vector<std::string> incl_searchpath;
};

class parsing_driver {
 private:
  std::unique_ptr<lexer> lexer_;

  int get_lineno() const;
  std::string get_text() const;

 public:
  parsing_params params;

  /**
   * The last parsed doctext comment.
   */
  t_doc doctext;

  /**
   * The location of the last parsed doctext comment.
   */
  int doctext_lineno;

  /**
   * The parsing pass that we are on. We do different things on each pass.
   */
  parsing_mode mode;

  /**
   * The master program parse tree. This is accessed from within the parser code
   * to build up the program elements.
   */
  t_program* program;

  std::unique_ptr<t_program_bundle> program_bundle;

  /**
   * Global scope cache for faster compilations
   */
  t_scope* scope_cache;

  /**
   * A global map that holds a pointer to all programs already cached
   */
  std::map<std::string, t_program*> program_cache;

  parsing_driver(
      diagnostic_context& ctx, std::string path, parsing_params parse_params);
  ~parsing_driver();

  const lexer& get_lexer() const { return *lexer_; }
  lexer& get_lexer() { return *lexer_; }

  /**
   * Parses a program and returns the resulted AST.
   * Diagnostic messages (warnings, debug messages, etc.) are reported via the
   * context provided in the constructor.
   */
  std::unique_ptr<t_program_bundle> parse();

  /**
   * Bison's type.
   */
  using YYSTYPE = int;
  YYSTYPE yylval_ = 0;

  /**
   * Bison's structure to store location.
   */
  using YYLTYPE = apache::thrift::compiler::yy::location;
  YYLTYPE yylloc_;

  /**
   * Diagnostic message callbacks.
   */
  // TODO(afuller): Remove these, and have the parser call the functions on ctx_
  // directly.
  template <typename... Args>
  void debug(Args&&... args) {
    ctx_.debug(get_lineno(), get_text(), std::forward<Args>(args)...);
  }

  template <typename... Args>
  void verbose(Args&&... args) {
    ctx_.info(get_lineno(), get_text(), std::forward<Args>(args)...);
  }

  template <typename... Args>
  void yyerror(Args&&... args) {
    ctx_.report(
        diagnostic_level::parse_error,
        get_lineno(),
        get_text(),
        std::forward<Args>(args)...);
  }

  template <typename... Args>
  void warning(Args&&... args) {
    ctx_.warning(get_lineno(), get_text(), std::forward<Args>(args)...);
  }

  template <typename... Args>
  void warning_strict(Args&&... args) {
    ctx_.warning_strict(get_lineno(), get_text(), std::forward<Args>(args)...);
  }

  template <typename... Args>
  [[noreturn]] void failure(Args&&... args) {
    ctx_.failure(get_lineno(), get_text(), std::forward<Args>(args)...);
    end_parsing();
  }

  [[noreturn]] void unexpected_token(const char* text) {
    failure(
        [&](auto& o) { o << "Unexpected token in input: " << text << "\n"; });
  }

  [[noreturn]] void end_parsing();

  /**
   * Gets the directory path of a filename.
   */
  static std::string directory_name(const std::string& filename);

  /**
   * Finds the appropriate file path for the given include filename.
   */
  std::string find_include_file(const std::string& filename);

  /**
   * Check the type of the parsed const information against its declared type.
   */
  void validate_const_type(t_const* c);

  /**
   * Check that the constant name does not refer to an ambiguous enum.
   * An ambiguous enum is one that is redefined but not referred to by
   * ENUM_NAME.ENUM_VALUE.
   */
  void validate_not_ambiguous_enum(const std::string& name);

  /**
   * Clears any previously stored doctext string.
   * Also prints a warning if we are discarding information.
   */
  void clear_doctext();

  /** Return any doctext previously push-ed */
  t_doc pop_doctext();

  /** Strip comment chars and align leading whitespace on multiline doctext
   */
  t_doc strip_doctext(const char* text);

  /** update doctext of given node */
  void set_doctext(t_node& node, t_doc doctext) const;

  /**
   * Cleans up text commonly found in doxygen-like comments.
   *
   * Warning: if you mix tabs and spaces in a non-uniform way,
   * you will get what you deserve.
   */
  t_doc clean_up_doctext(std::string docstring);

  // Checks if the given experimental features is enabled, and reports a failure
  // and returns false iff not.
  bool require_experimental_feature(const char* feature);

  /**
   * Hands a pointer to be deleted when the parsing driver itself destructs.
   */
  template <typename T>
  void delete_at_the_end(T* ptr) {
    deleters_.push_back(deleter{ptr});
  }

  // Returns the source range object containing the location information.
  source_range get_source_range(const YYLTYPE& loc) const;

  void reset_locations();
  void compute_locations(const char* source, size_t size);

  /*
   * To fix Bison's default location
   * (result's begin set to end of prev token and result's end set
   *  to begin of next token)
   */
  void avoid_tokens_loc(
      YYLTYPE& result_loc,
      const std::vector<std::pair<bool, YYLTYPE>>& last_loc_overrides,
      const std::vector<std::pair<bool, YYLTYPE>>& next_loc_overrides) {
    for (const auto& loc_override : last_loc_overrides) {
      if (!loc_override.first) {
        break;
      }
      result_loc.begin = loc_override.second.begin;
    }
    for (const auto& loc_override : next_loc_overrides) {
      if (!loc_override.first) {
        break;
      }
      result_loc.end = loc_override.second.end;
    }
  }

  // Populate the annotation on the given node.
  static void set_annotations(
      t_node* node, std::unique_ptr<t_annotations> annotations);

  std::unique_ptr<t_const> new_struct_annotation(
      std::unique_ptr<t_const_value> const_struct);

  std::unique_ptr<t_throws> new_throws(
      std::unique_ptr<t_field_list> exceptions);

  // Creates a reference to a known type, potentally with additional
  // annotations.
  t_type_ref new_type_ref(
      const t_type& type, std::unique_ptr<t_annotations> annotations);
  t_type_ref new_type_ref(t_type&& type, std::unique_ptr<t_annotations>) =
      delete;

  // Creates a reference to a newly instantiated templated type.
  t_type_ref new_type_ref(
      std::unique_ptr<t_templated_type> type,
      std::unique_ptr<t_annotations> annotations);
  // Creates a reference to a named type.
  t_type_ref new_type_ref(
      std::string name,
      std::unique_ptr<t_annotations> annotations,
      bool is_const = false);

  // Tries to set the given fields, reporting a failure on a collsion.
  // TODO(afuller): Disallow auto-id allocation.
  void set_fields(t_structured& tstruct, t_field_list&& fields);

  void set_functions(
      t_interface& node, std::unique_ptr<t_function_list> functions);

  // Populate the attributes on the given node.
  void set_attributes(
      t_named& node,
      std::unique_ptr<t_def_attrs> attrs,
      std::unique_ptr<t_annotations> annots,
      const YYLTYPE& loc) const;

  // Adds a definition to the program.
  t_ref<t_named> add_def(std::unique_ptr<t_named> node);

  void add_include(std::string name);
  void set_package(std::string name);

  t_field_id to_field_id(int64_t int_const) {
    return narrow_int<t_field_id>(int_const, "field ids");
  }
  int32_t to_enum_value(int64_t int_const) {
    return narrow_int<int32_t>(int_const, "enum values");
  }
  std::unique_ptr<t_const_value> to_const_value(int64_t int_const);

  // Reports a failure if the parsed value cannot fit in the widest supported
  // representation, i.e. int64_t and double.
  uint64_t parse_integer(const char* text, int offset, int base);
  double parse_double(const char* text);

  /** Consume doctext, store it in `driver.doctext`
   *
   * It is non-trivial for a yacc-style LR(1) parser to accept doctext
   * as an optional prefix before either a definition or standalone at
   * the header. Hence this method of "pushing" it into the driver and
   * "pop-ing" it on the node as needed.
   */
  void push_doctext(const char* text, int lineno);
  int64_t to_int(uint64_t val, bool negative = false);

  const t_service* find_service(const std::string& name);
  const t_const* find_const(const std::string& name);

  std::unique_ptr<t_const_value> copy_const_value(const std::string& name);

  void set_parsed_definition();
  void validate_header_location();
  void validate_header_annotations(
      std::unique_ptr<t_def_attrs> statement_attrs,
      std::unique_ptr<t_annotations> annotations);
  void set_program_annotations(
      std::unique_ptr<t_def_attrs> statement_attrs,
      std::unique_ptr<t_annotations> annotations,
      const YYLTYPE& loc);

 private:
  class deleter {
   public:
    template <typename T>
    explicit deleter(T* ptr)
        : ptr_(ptr),
          delete_([](const void* ptr) { delete static_cast<const T*>(ptr); }) {}

    deleter(const deleter&) = delete;
    deleter& operator=(const deleter&) = delete;

    deleter(deleter&& rhs) noexcept : ptr_{rhs.ptr_}, delete_{rhs.delete_} {
      rhs.ptr_ = nullptr;
      rhs.delete_ = nullptr;
    }

    deleter& operator=(deleter&& rhs) {
      std::swap(ptr_, rhs.ptr_);
      std::swap(delete_, rhs.delete_);
      return *this;
    }

    ~deleter() {
      if (!!ptr_) {
        delete_(ptr_);
      }
    }

   private:
    const void* ptr_;
    void (*delete_)(const void*);
  };

  void compute_location_impl(
      YYLTYPE& yylloc, YYSTYPE& yylval, const char* text);

  std::set<std::string> already_parsed_paths_;
  std::set<std::string> circular_deps_;

  std::unique_ptr<yy::parser> parser_;

  std::vector<deleter> deleters_;
  diagnostic_context& ctx_;

  std::unordered_set<std::string> programs_that_parsed_definition_;

  /**
   * Parse a single .thrift file. The file to parse is stored in params.program.
   */
  void parse_file();

  // Returns true if the node should be
  // added to the program. Otherwise, the driver itself
  // takes ownership of node.
  template <typename T>
  bool should_add_node(std::unique_ptr<T>& node) {
    if (mode != parsing_mode::PROGRAM) {
      delete_at_the_end(node.release());
      return false;
    }
    return true;
  }

  // Adds an unnamed typedef to the program
  // TODO(afuller): Remove the need for these by an explicit t_type_ref node
  // that can annotatable.
  const t_type* add_unnamed_typedef(
      std::unique_ptr<t_typedef> node,
      std::unique_ptr<t_annotations> annotations);

  // Adds an placeholder typedef to the program
  // TODO(afuller): Remove the need for these by adding a explicit t_type_ref
  // node that can be resolved in a second passover the ast.
  const t_type* add_placeholder_typedef(
      std::unique_ptr<t_placeholder_typedef> node,
      std::unique_ptr<t_annotations> annotations);

  std::string scoped_name(const t_named& node) {
    return scoped_name(node.name());
  }
  std::string scoped_name(const std::string& name) {
    return program->name() + "." + name;
  }
  std::string scoped_name(const t_named& owner, const t_named& node) {
    return program->name() + "." + owner.get_name() + "." + node.get_name();
  }

  // Automatic numbering for field ids.
  //
  // Field id are assigned starting from -1 and working their way down.
  //
  // TODO(afuller): Move auto field ids to a post parse phase (or remove the
  // feature entirely).
  void allocate_field_id(t_field_id& next_id, t_field& field);
  void maybe_allocate_field_id(t_field_id& next_id, t_field& field);

  template <typename T>
  T narrow_int(int64_t int_const, const char* name) {
    using limits = std::numeric_limits<T>;
    if (int_const < limits::min() || int_const > limits::max()) {
      failure([&](auto& o) {
        o << "Integer constant (" << int_const << ") outside the range of "
          << name << " ([" << limits::min() << ", " << limits::max() << "]).";
      });
    }
    return int_const;
  }
};

} // namespace compiler
} // namespace thrift
} // namespace apache
