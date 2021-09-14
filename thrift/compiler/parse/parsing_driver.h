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

#include <cstddef>
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
#include <thrift/compiler/ast/t_program.h>
#include <thrift/compiler/ast/t_program_bundle.h>
#include <thrift/compiler/ast/t_scope.h>
#include <thrift/compiler/ast/t_union.h>
#include <thrift/compiler/parse/yy_scanner.h>

/**
 * Note macro expansion because this is different between OSS and internal
 * build, sigh.
 */
#ifndef LOCATION_HH
#define LOCATION_HH "thrift/compiler/parse/location.hh"
#endif

#include LOCATION_HH

namespace {

/**
 * YYLTYPE evaluates to a class named 'location', which has:
 *   position begin;
 *   position end;
 * The class 'position', in turn, has:
 *   std::string* filename;
 *   unsigned line;
 *   unsigned column;
 */
using YYLTYPE = apache::thrift::compiler::yy::location;
using YYSTYPE = int;

/**
 * Provide the custom fbthrift_compiler_parse_lex signature to flex.
 */
#define YY_DECL                                         \
  apache::thrift::compiler::yy::parser::symbol_type     \
  fbthrift_compiler_parse_lex(                          \
      apache::thrift::compiler::parsing_driver& driver, \
      apache::thrift::compiler::yyscan_t yyscanner,     \
      YYSTYPE* yylval_param,                            \
      YYLTYPE* yylloc_param)

} // namespace

namespace apache {
namespace thrift {
namespace compiler {

namespace yy {
class parser;
}

// Define an enum class for all types that have lineno embedded.
enum class LineType {
  Unspecified = 0,
  Typedef,
  Enum,
  EnumValue,
  Const,
  Struct,
  Service,
  Interaction,
  Function,
  Field,
  Exception,
  Union,
};

// Parsing only representations.
using t_struct_annotations = node_list<t_const>;
struct t_annotations {
  std::map<std::string, annotation_value> strings;
  std::map<std::string, std::shared_ptr<const t_const>> objects;
};
using t_doc = boost::optional<std::string>;
struct t_def_attrs {
  t_doc doc;
  std::unique_ptr<t_struct_annotations> struct_annotations;
};

// A const pointer to an AST node.
//
// This is needed to avoid ambiguity in the parser code gen for const pointers.
template <typename T>
class t_ref {
 public:
  constexpr t_ref() = default;
  constexpr t_ref(const t_ref&) noexcept = default;
  constexpr t_ref(std::nullptr_t) noexcept {}

  template <typename U>
  constexpr /* implicit */ t_ref(const t_ref<U>& u) noexcept : ptr_(u.get()) {}

  // Require an explicit cast for a non-const pointer, as non-const pointers
  // likely need to be owned, so this is probably a bug.
  template <typename U, std::enable_if_t<std::is_const<U>::value, int> = 0>
  constexpr /* implicit */ t_ref(U* ptr) : ptr_(ptr) {}
  template <typename U, std::enable_if_t<!std::is_const<U>::value, int> = 0>
  constexpr explicit t_ref(U* ptr) : ptr_(ptr) {}

  constexpr const T* get() const { return ptr_; }

  constexpr const T* operator->() const {
    assert(ptr_ != nullptr);
    return ptr_;
  }
  constexpr const T& operator*() const {
    assert(ptr_ != nullptr);
    return *ptr_;
  }

  constexpr explicit operator bool() const { return bool(ptr_); }
  constexpr /* implicit */ operator const T*() const { return ptr_; }

 private:
  const T* ptr_ = nullptr;
};

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

  /**
   * The Flex lexer used by the parser.
   */
  std::unique_ptr<yy_scanner> scanner;

  parsing_driver(
      diagnostic_context& ctx, std::string path, parsing_params parse_params);
  ~parsing_driver();

  /**
   * Parses a program and returns the resulted AST.
   * Diagnostic messages (warnings, debug messages, etc.) are reported via the
   * context provided in the constructor.
   */
  std::unique_ptr<t_program_bundle> parse();

  /**
   * Bison's type (default is int).
   */
  YYSTYPE yylval_{};

  /**
   * Bison's structure to store location.
   */
  YYLTYPE yylloc_{};

  /**
   * Diagnostic message callbacks.
   */
  // TODO(afuller): Remove these, and have the parser call the functions on ctx_
  // directly.
  template <typename... Args>
  void debug(Args&&... args) {
    ctx_.debug(
        scanner->get_lineno(),
        scanner->get_text(),
        std::forward<Args>(args)...);
  }

  template <typename... Args>
  void verbose(Args&&... args) {
    ctx_.info(
        scanner->get_lineno(),
        scanner->get_text(),
        std::forward<Args>(args)...);
  }

  template <typename... Args>
  void yyerror(Args&&... args) {
    ctx_.report(
        diagnostic_level::parse_error,
        scanner->get_lineno(),
        scanner->get_text(),
        std::forward<Args>(args)...);
  }

  template <typename... Args>
  void warning(Args&&... args) {
    ctx_.warning(
        scanner->get_lineno(),
        scanner->get_text(),
        std::forward<Args>(args)...);
  }

  template <typename... Args>
  void warning_strict(Args&&... args) {
    ctx_.warning_strict(
        scanner->get_lineno(),
        scanner->get_text(),
        std::forward<Args>(args)...);
  }

  template <typename... Args>
  [[noreturn]] void failure(Args&&... args) {
    ctx_.failure(
        scanner->get_lineno(),
        scanner->get_text(),
        std::forward<Args>(args)...);
    end_parsing();
  }

  [[noreturn]] void end_parsing();

  /**
   * Gets the directory path of a filename
   */
  static std::string directory_name(const std::string& filename);

  /**
   * Finds the appropriate file path for the given filename
   */
  std::string include_file(const std::string& filename);

  /**
   * Check the type of the parsed const information against its declared type
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

  /**
   * Cleans up text commonly found in doxygen-like comments
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

  // Record the line number for the start of the node.
  void start_node(LineType lineType);

  // Returns the source range object containing the location information.
  source_range get_source_range(const YYLTYPE& loc);

  // Configures the node and set the starting line number.
  void finish_node(
      t_named* node,
      LineType lineType,
      const YYLTYPE& loc,
      std::unique_ptr<t_def_attrs> attrs,
      std::unique_ptr<t_annotations> annotations);
  void finish_node(
      t_structured* node,
      LineType lineType,
      const YYLTYPE& loc,
      std::unique_ptr<t_def_attrs> attrs,
      std::unique_ptr<t_field_list> fields,
      std::unique_ptr<t_annotations> annotations);
  void finish_node(
      t_interface* node,
      LineType lineType,
      const YYLTYPE& loc,
      std::unique_ptr<t_def_attrs> attrs,
      std::unique_ptr<t_function_list> functions,
      std::unique_ptr<t_annotations> annotations);

  void reset_locations() {
    yylloc_.begin.line = 1;
    yylloc_.begin.column = 1;
    yylloc_.end.line = 1;
    yylloc_.end.column = 1;
    yylval_ = 0;
  }

  void compute_location(YYLTYPE& yylloc, YYSTYPE& yylval, const char* text) {
    /* Only computing locations during second pass. */
    if (mode == parsing_mode::PROGRAM) {
      compute_location_impl(yylloc, yylval, text);
    }
  }

  /*
   * To fix Bison's default location
   * (result's begin set to end of prev token)
   */
  void avoid_last_token_loc(
      bool null_first_elem, YYLTYPE& result_loc, YYLTYPE& next_non_null_loc) {
    if (null_first_elem) {
      result_loc.begin = next_non_null_loc.begin;
    }
  }

  /*
   * To fix Bison's default location
   * (result's end set to begin of next token)
   */
  void avoid_next_token_loc(
      bool null_last_elem, YYLTYPE& result_loc, YYLTYPE& next_non_null_loc) {
    if (null_last_elem) {
      result_loc.end = next_non_null_loc.end;
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

  // Adds a definition to the program.
  t_ref<t_const> add_def(std::unique_ptr<t_const> node);
  t_ref<t_interaction> add_def(std::unique_ptr<t_interaction> node);
  t_ref<t_service> add_def(std::unique_ptr<t_service> node);
  t_ref<t_typedef> add_def(std::unique_ptr<t_typedef> node);
  t_ref<t_struct> add_def(std::unique_ptr<t_struct> node);
  t_ref<t_union> add_def(std::unique_ptr<t_union> node);
  t_ref<t_exception> add_def(std::unique_ptr<t_exception> node);
  t_ref<t_enum> add_def(std::unique_ptr<t_enum> node);

  t_field_id as_field_id(int64_t int_const);
  t_field_id next_field_id() const { return next_field_id_; }
  t_field_id allocate_field_id(const std::string& name);
  void reserve_field_id(t_field_id id);

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
  std::stack<std::pair<LineType, int>> lineno_stack_;
  diagnostic_context& ctx_;

  /**
   * This variable is used for automatic numbering of field indices etc.
   * when parsing the members of a struct. Field values are automatically
   * assigned starting from -1 and working their way down.
   **/
  // TODO(afuller): Move auto field ids to a post parse phase.
  int next_field_id_ = 0; // Zero indicates not in field context.

  // Populate the attributes on the given node.
  static void set_attributes(
      t_named* node,
      std::unique_ptr<t_def_attrs> attrs,
      std::unique_ptr<t_annotations> annotations);

  /**
   * Parse a single .thrift file. The file to parse is stored in params.program.
   */
  void parse_file();

  // Returns the starting line number.
  int pop_node(LineType lineType);

  void append_fields(t_structured& tstruct, t_field_list&& fields);

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

  // Updates the scope cache and returns true
  // if the node should be added to the program. Otherwise,
  // the driver itself takes ownership of node.
  template <typename T>
  bool should_add_type(std::unique_ptr<T>& node) {
    if (should_add_node(node)) {
      scope_cache->add_type(scoped_name(*node), node.get());
      return true;
    }
    return false;
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
    return program->name() + "." + node.get_name();
  }
  std::string scoped_name(const t_named& owner, const t_named& node) {
    return program->name() + "." + owner.get_name() + "." + node.get_name();
  }
};

} // namespace compiler
} // namespace thrift
} // namespace apache
