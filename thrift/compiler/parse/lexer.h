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

#include <string>
#include <vector>

// This is a macro because of a difference between the OSS and internal builds.
#ifndef THRIFTY_HH
#define THRIFTY_HH <thrift/compiler/parse/thrifty.yy.h>
#endif
#include THRIFTY_HH

namespace apache {
namespace thrift {
namespace compiler {

class parsing_driver;

// A Thrift lexer.
class lexer {
 private:
  parsing_driver* driver_;
  std::string filename_;
  std::vector<char> source_; // Source being lexed with a terminating '\0'.
  const char* ptr_; // Current position in the source.
  const char* token_start_;
  yy::position token_pos_;
  const char* line_start_;
  int lineno_ = 0;

  lexer(parsing_driver& driver, std::string filename, std::vector<char> source);

  const char* end() const { return source_.data() + source_.size() - 1; }

  yy::position make_position() {
    return yy::position(&filename_, lineno_, ptr_ - line_start_ + 1);
  }
  yy::location make_location() { return {token_pos_, make_position()}; }

  void start_token() {
    token_start_ = ptr_;
    token_pos_ = make_position();
  }

  yy::parser::symbol_type unexpected_token();

  void update_line();

  enum class comment_lex_result { skipped, doc_comment, unterminated };

  void skip_line_comment();
  bool lex_doc_comment();
  comment_lex_result lex_block_comment();
  comment_lex_result lex_whitespace_or_comment();

 public:
  lexer(lexer&& other) = default;
  lexer& operator=(lexer&& other) = default;

  // Creates a lexer from a file.
  static lexer from_file(parsing_driver& driver, std::string filename);

  // Creates a lexer from a string; filename is used for source locations.
  static lexer from_string(
      parsing_driver& driver, std::string filename, std::string source) {
    const char* start = source.c_str();
    return {driver, std::move(filename), {start, start + source.size() + 1}};
  }

  yy::parser::symbol_type get_next_token();

  int get_lineno() const { return lineno_; }

  // Returns the current token as a string.
  std::string get_text() const { return {token_start_, ptr_}; }
};

} // namespace compiler
} // namespace thrift
} // namespace apache
