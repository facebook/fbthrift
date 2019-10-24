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

#include <cassert>
#include <cstdio>

#include <memory>
#include <system_error>

typedef void* yyscan_t;

/**
 * These are provided by Flex.
 */
int fbthrift_compiler_parse_lex_init(yyscan_t* ptr_yy_globals);
int fbthrift_compiler_parse_lex_destroy(yyscan_t yyscanner);

void fbthrift_compiler_parse_restart(FILE* input_file, yyscan_t yyscanner);

int fbthrift_compiler_parse_get_lineno(yyscan_t scanner);
void fbthrift_compiler_parse_set_lineno(int line_number, yyscan_t scanner);
char* fbthrift_compiler_parse_get_text(yyscan_t scanner);

namespace apache {
namespace thrift {

/**
 * A simple RAII wrapper for read-only FILE.
 */
class readonly_file {
 public:
  readonly_file(std::string path) {
    file_ = fopen(path.c_str(), "r");
    if (!file_) {
      throw std::runtime_error("Could not open input file: \"" + path + "\"");
    }
  }

  ~readonly_file() {
    if (!!file_) {
      fclose(file_);
    }
  }

  FILE* get_file() const {
    return file_;
  }

 private:
  FILE* file_ = nullptr;
};

/**
 * A thin wrapper around the yy scanner type (yyscan_t) to provide RAII
 * semantic.
 */
class yy_scanner {
 public:
  yy_scanner() {
    if (fbthrift_compiler_parse_lex_init(&scanner_) != 0) {
      throw std::system_error(errno, std::generic_category());
    }
  }

  yy_scanner(const yy_scanner&) = delete;
  yy_scanner& operator=(const yy_scanner&) = delete;
  yy_scanner(yy_scanner&&) = delete;
  yy_scanner& operator=(yy_scanner&&) = delete;

  ~yy_scanner() {
    if (!!scanner_) {
      // Why does fbthrift_compiler_parse_lex_destroy return an int??
      fbthrift_compiler_parse_lex_destroy(scanner_);
    }
  }

  /**
   * Initializes the lexer to lex a new file.
   *
   * Can be called multiple times to reuse the same lexer to lex multiple files.
   */
  void start(std::string path) {
    file_ = std::make_unique<readonly_file>(path);
    fbthrift_compiler_parse_restart(file_->get_file(), scanner_);
  }

  int get_lineno() const {
    assert(!!scanner_);
    return fbthrift_compiler_parse_get_lineno(scanner_);
  }

  void set_lineno(int lineno) {
    assert(!!scanner_);
    fbthrift_compiler_parse_set_lineno(lineno, scanner_);
  }

  std::string get_text() const {
    assert(!!scanner_);
    char const* text = fbthrift_compiler_parse_get_text(scanner_);
    return (!!text) ? std::string{text} : "";
  }

  yyscan_t get_scanner() {
    return scanner_;
  }

 private:
  yyscan_t scanner_ = nullptr;
  std::unique_ptr<readonly_file> file_;
};

} // namespace thrift
} // namespace apache
