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

class parsing_driver {
 public:
  parsing_params params;

  explicit parsing_driver(parsing_params parse_params)
      : params(std::move(parse_params)) {
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

 private:
  std::string curdir_;
};

} // namespace thrift
} // namespace apache

/**
 * Declare the custom yylex function for the parser to use.
 */
YY_DECL;
