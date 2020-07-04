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

#include <array>
#include <memory>
#include <set>
#include <string>
#include <system_error>

#include <boost/optional.hpp>

#include "thrift/compiler/ast/t_program.h"
#include "thrift/compiler/ast/t_program_bundle.h"
#include "thrift/compiler/ast/t_scope.h"

#include "thrift/compiler/parse/yy_scanner.h"

/**
 * Provide the custom fbthrift_compiler_parse_lex signature to flex.
 */
#define YY_DECL                                     \
  apache::thrift::compiler::yy::parser::symbol_type \
  fbthrift_compiler_parse_lex(                      \
      apache::thrift::compiler::parsing_driver& driver, yyscan_t yyscanner)

namespace apache {
namespace thrift {
namespace compiler {

namespace yy {
class parser;
}

enum class diagnostic_level {
  FAILURE = 0,
  YY_ERROR = 1,
  WARNING = 2,
  VERBOSE = 3,
  DBG = 4,
};

struct diagnostic_message {
  diagnostic_level level;
  std::string filename;
  int lineno;
  std::string last_token;
  std::string message;

  diagnostic_message(
      diagnostic_level level_,
      std::string filename_,
      int lineno_,
      std::string last_token_,
      std::string message_)
      : level{level_},
        filename{std::move(filename_)},
        lineno{lineno_},
        last_token{std::move(last_token_)},
        message{std::move(message_)} {}
};

enum class parsing_mode {
  INCLUDES = 1,
  PROGRAM = 2,
};

struct parsing_params {
  // Default values are taken from the original global variables.

  parsing_params() noexcept {} // Disable aggregate initialization

  bool debug = false;
  bool verbose = false;
  int warn = 1;

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
   * Whether or not negative enum values.
   */
  bool allow_neg_enum_vals = false;

  /**
   * Whether or not 64-bit constants will generate a warning.
   *
   * Some languages don't support 64-bit constants, but many do, so we can
   * suppress this warning for projects that don't use any non-64-bit-safe
   * languages.
   */
  bool allow_64bit_consts = false;

  /**
   * Search path for inclusions
   */
  std::vector<std::string> incl_searchpath;
};

// This elaborate dance is required to avoid triggering -Wformat-security in the
// case where we have no format arguments.

template <typename... Arg>
int snprintf_with_param_pack(
    char* str,
    size_t size,
    const char* fmt,
    Arg&&... arg) {
  return snprintf(str, size, fmt, std::forward<Arg>(arg)...);
}

template <>
inline int snprintf_with_param_pack(char* str, size_t size, const char* fmt) {
  return snprintf(str, size, "%s", fmt);
}

class parsing_driver {
 public:
  parsing_params params;

  /**
   * The last parsed doctext comment.
   */
  boost::optional<std::string> doctext;

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
  std::unique_ptr<apache::thrift::yy_scanner> scanner;

  parsing_driver(std::string path, parsing_params parse_params);
  ~parsing_driver();

  /**
   * Parses a program and returns the resulted AST.
   * Diagnostic messages (warnings, debug messages, etc.) are stored in the
   * vector passed in via params.messages.
   */
  std::unique_ptr<t_program_bundle> parse(
      std::vector<diagnostic_message>& messages);

  /**
   * Diagnostic message callbacks.
   */
  template <typename... Arg>
  void debug(const char* fmt, Arg&&... arg) {
    if (!params.debug) {
      return;
    }

    auto message = construct_diagnostic_message(
        diagnostic_level::DBG, fmt, std::forward<Arg>(arg)...);
    diagnostic_messages_.push_back(std::move(message));
  }

  template <typename... Arg>
  void verbose(const char* fmt, Arg&&... arg) {
    if (!params.verbose) {
      return;
    }

    auto message = construct_diagnostic_message(
        diagnostic_level::VERBOSE, fmt, std::forward<Arg>(arg)...);
    diagnostic_messages_.push_back(std::move(message));
  }

  template <typename... Arg>
  void yyerror(const char* fmt, Arg&&... arg) {
    auto message = construct_diagnostic_message(
        diagnostic_level::YY_ERROR, fmt, std::forward<Arg>(arg)...);
    diagnostic_messages_.push_back(std::move(message));
  }

  template <typename... Arg>
  void warning(int level, const char* fmt, Arg&&... arg) {
    if (params.warn < level) {
      return;
    }

    auto message = construct_diagnostic_message(
        diagnostic_level::WARNING, fmt, std::forward<Arg>(arg)...);
    diagnostic_messages_.push_back(std::move(message));
  }

  // clang-format off
  // TODO: `clang-format` incorrectly indents the function after a [[noreturn]]
  // function for 4 extra spaces.
  template <typename... Arg>
  [[noreturn]] void failure(const char* fmt, Arg&&... arg) {
    auto msg = construct_diagnostic_message(
        diagnostic_level::FAILURE, fmt, std::forward<Arg>(arg)...);
    diagnostic_messages_.push_back(std::move(msg));
    end_parsing();
  }

  [[noreturn]] void end_parsing();
  // clang-format on

  /**
   * Gets the directory path of a filename
   */
  static std::string directory_name(const std::string& filename);

  /**
   * Finds the appropriate file path for the given filename
   */
  std::string include_file(const std::string& filename);

  /**
   * You know, when I started working on Thrift I really thought it wasn't going
   * to become a programming language because it was just a generator and it
   * wouldn't need runtime type information and all that jazz. But then we
   * decided to add constants, and all of a sudden that means runtime type
   * validation and inference, except the "runtime" is the code generator
   * runtime. Shit. I've been had.
   */
  void validate_const_rec(std::string name, t_type* type, t_const_value* value);

  /**
   * Check the type of the parsed const information against its declared type
   */
  void validate_const_type(t_const* c);

  /**
   * Check the type of a default value assigned to a field.
   */
  void validate_field_value(t_field* field, t_const_value* cv);

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
  boost::optional<std::string> clean_up_doctext(std::string docstring);

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

  std::vector<deleter> deleters_;

 public:
  /**
   * Hands a pointer to be deleted when the parsing driver itself destructs.
   */
  template <typename T>
  void delete_at_the_end(T* ptr) {
    deleters_.push_back(deleter{ptr});
  }

 private:
  std::set<std::string> already_parsed_paths_;
  std::set<std::string> circular_deps_;

  std::unique_ptr<yy::parser> parser_;

  std::vector<diagnostic_message> diagnostic_messages_;

  /**
   * Parse a single .thrift file. The file to parse is stored in params.program.
   */
  void parse_file();

  template <typename... Arg>
  diagnostic_message construct_diagnostic_message(
      diagnostic_level level,
      const char* fmt,
      Arg&&... arg) {
    const size_t buffer_size = 1024;
    std::array<char, buffer_size> buffer;
    std::string message;

    int ret = snprintf_with_param_pack(
        buffer.data(), buffer_size, fmt, std::forward<Arg>(arg)...);
    if (ret < 0) {
      // Technically we could be OOM here, so the following line would fail.
      // But...
      throw std::system_error(
          errno, std::generic_category(), "In snprintf(...)");
    }

    auto full_length = static_cast<size_t>(ret);
    if (full_length < buffer_size) {
      message = std::string{buffer.data()};
    } else {
      // In the (extremely) unlikely case that the message is 1024-char or
      // longer, we do dynamic allocation.
      //
      // "+ 1" for the NULL-terminator.
      std::vector<char> dyn_buffer(static_cast<size_t>(full_length + 1), '\0');

      ret = snprintf_with_param_pack(
          dyn_buffer.data(), dyn_buffer.size(), fmt, std::forward<Arg>(arg)...);
      if (ret < 0) {
        throw std::system_error(
            errno, std::generic_category(), "In second snprintf(...)");
      }

      assert(static_cast<size_t>(ret) < dyn_buffer.size());

      message = std::string{dyn_buffer.data()};
    }

    return diagnostic_message{level,
                              program->get_path(),
                              scanner->get_lineno(),
                              scanner->get_text(),
                              message};
  }
};

} // namespace compiler
} // namespace thrift
} // namespace apache
