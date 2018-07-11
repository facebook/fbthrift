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

#include "thrift/compiler/common.h"
#include "thrift/compiler/globals.h"
#include "thrift/compiler/parse/t_program.h"

/**
 * Must be included AFTER parse/t_program.h, but I can't remember why anymore
 * because I wrote this a while ago.
 *
 * Note macro expansion because this is different between OSS and internal
 * build, sigh.
 */
#include THRIFTY_HH

/**
 * Provide the custom yylex signature to flex.
 */
#define YY_DECL                                  \
  apache::thrift::yy::parser::symbol_type yylex( \
      apache::thrift::parsing_driver& driver)

namespace apache {
namespace thrift {

enum class parsing_mode {
  INCLUDES = 1,
  PROGRAM = 2,
};

class parsing_driver {
 public:
  parsing_params params;

  /**
   * The parsing pass that we are on. We do different things on each pass.
   */
  parsing_mode mode;

  explicit parsing_driver(parsing_params parse_params)
      : params(std::move(parse_params)), mode(parsing_mode::INCLUDES) {
    // Set current dir, which is used in the include_file function
    curdir_ = directory_name(params.program->get_path());
  }

  /**
   * Diagnostic message callbacks.
   */
  void debug(const char* fmt, ...) const;
  void verbose(const char* fmt, ...) const;
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

 private:
  std::string curdir_;
};

} // namespace thrift
} // namespace apache

/**
 * Declare the custom yylex function for the parser to use.
 */
YY_DECL;
