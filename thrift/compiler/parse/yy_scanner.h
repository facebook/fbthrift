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

#include <cassert>
#include <cstdio>

#include <memory>
#include <system_error>

typedef void* yyscan_t;

/**
 * These are provided by Flex.
 */
int yylex_init(yyscan_t* ptr_yy_globals);
int yylex_destroy(yyscan_t yyscanner);

void yyrestart(FILE* input_file, yyscan_t yyscanner);

int yyget_lineno(yyscan_t scanner);
void yyset_lineno(int line_number, yyscan_t scanner);
char* yyget_text(yyscan_t scanner);

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
    if (yylex_init(&scanner_) != 0) {
      throw std::system_error(errno, std::generic_category());
    }
  }

  yy_scanner(const yy_scanner&) = delete;
  yy_scanner& operator=(const yy_scanner&) = delete;
  yy_scanner(yy_scanner&&) = delete;
  yy_scanner& operator=(yy_scanner&&) = delete;

  ~yy_scanner() {
    if (!!scanner_) {
      // Why does yylex_destroy return an int??
      yylex_destroy(scanner_);
    }
  }

  /**
   * Initializes the lexer to lex a new file.
   *
   * Can be called multiple times to reuse the same lexer to lex multiple files.
   */
  void start(std::string path) {
    file_ = std::make_unique<readonly_file>(path);
    yyrestart(file_->get_file(), scanner_);
  }

  int get_lineno() const {
    assert(!!scanner_);
    return yyget_lineno(scanner_);
  }

  void set_lineno(int lineno) {
    assert(!!scanner_);
    yyset_lineno(lineno, scanner_);
  }

  std::string get_text() const {
    assert(!!scanner_);
    return std::string{yyget_text(scanner_)};
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
