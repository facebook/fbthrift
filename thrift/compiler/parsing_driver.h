/*
 * Copyright 2018-present Facebook, Inc.
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

#include <memory>

#include "thrift/compiler/common.h"
#include "thrift/compiler/globals.h"
#include "thrift/compiler/parse/t_program.h"

/**
 * Provide the custom yylex signature to flex.
 */
#define YY_DECL                                  \
  apache::thrift::yy::parser::symbol_type yylex( \
      apache::thrift::parsing_driver& driver)

namespace apache {
namespace thrift {

namespace yy {
class parser;
}

enum class parsing_mode {
  INCLUDES = 1,
  PROGRAM = 2,
};

class parsing_driver {
 public:
  parsing_params params;

  /**
   * The last parsed doctext comment.
   */
  char* doctext;

  /**
   * The location of the last parsed doctext comment.
   */
  int doctext_lineno;

  /**
   * The parsing pass that we are on. We do different things on each pass.
   */
  parsing_mode mode;

  explicit parsing_driver(parsing_params parse_params);
  ~parsing_driver();

  /**
   * Parses a program. The resulted AST is stored in the t_program object passed
   * in via params.program.
   */
  void parse();

  /**
   * Diagnostic message callbacks.
   */
  void debug(const char* fmt, ...) const;
  void verbose(const char* fmt, ...) const;
  void yyerror(const char* fmt, ...) const;
  void warning(int level, const char* fmt, ...) const;
  [[noreturn]] void failure(const char* fmt, ...) const;

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
  char* clean_up_doctext(char* doctext);

 private:
  std::set<std::string> already_parsed_paths_;
  std::set<std::string> circular_deps_;

  std::unique_ptr<apache::thrift::yy::parser> parser_;
};

} // namespace thrift
} // namespace apache
