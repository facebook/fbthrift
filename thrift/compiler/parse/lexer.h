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
#include <boost/utility/string_view.hpp>

// This is a macro because of a difference between the OSS and internal builds.
#ifndef THRIFTY_HH
#define THRIFTY_HH <thrift/compiler/parse/thrifty.yy.h>
#endif
#include THRIFTY_HH

namespace apache {
namespace thrift {
namespace compiler {

class diagnostic_context;

// An interface that receives notifications of lexical elements.
class lex_handler {
 public:
  virtual ~lex_handler() {}

  // Invoked on a documentation comment such as `/** ... */` or `/// ...`.
  virtual void on_doc_comment(const char* text, int lineno) = 0;
};

// A Thrift lexer.
class lexer {
 private:
  lex_handler* handler_;
  diagnostic_context* diag_ctx_;
  std::string filename_;
  std::vector<char> source_; // Source being lexed with a terminating '\0'.
  const char* ptr_; // Current position in the source.
  const char* token_start_;
  yy::position token_pos_;
  const char* line_start_;
  int lineno_ = 0;

  lexer(
      lex_handler& handler,
      diagnostic_context& diag_ctx,
      std::string filename,
      std::vector<char> source);

  const char* end() const { return source_.data() + source_.size() - 1; }

  yy::position make_position() {
    return yy::position(&filename_, lineno_, ptr_ - line_start_ + 1);
  }
  yy::location make_location() { return {token_pos_, make_position()}; }

  void start_token() {
    token_start_ = ptr_;
    token_pos_ = make_position();
  }

  // Reports a failure if the parsed value cannot fit in the widest supported
  // representation, i.e. int64_t and double.
  yy::parser::symbol_type make_int_constant(int offset, int base);
  yy::parser::symbol_type make_float_constant();

  template <typename... T>
  yy::parser::symbol_type report_error(T&&... args);
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
  static lexer from_file(
      lex_handler& handler, diagnostic_context& diag_ctx, std::string filename);

  // Creates a lexer from a string; filename is used for source locations.
  static lexer from_string(
      lex_handler& handler,
      diagnostic_context& diag_ctx,
      std::string filename,
      std::string source) {
    const char* start = source.c_str();
    return {
        handler,
        diag_ctx,
        std::move(filename),
        {start, start + source.size() + 1}};
  }

  yy::parser::symbol_type get_next_token();

  int lineno() const { return lineno_; }

  // Returns the current token as a string.
  std::string token_text() const { return {token_start_, ptr_}; }

  boost::string_view source() const {
    return {source_.data(), source_.size() - 1};
  }
};

} // namespace compiler
} // namespace thrift
} // namespace apache
