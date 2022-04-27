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

#include <fmt/core.h>
#include <thrift/compiler/source_location.h>

// This is a macro because of a difference between the OSS and internal builds.
#ifndef THRIFTY_HH
#define THRIFTY_HH <thrift/compiler/parse/thrifty.yy.h>
#endif
#include THRIFTY_HH

namespace apache {
namespace thrift {
namespace compiler {

class diagnostics_engine;

// An interface that receives notifications of lexical elements.
class lex_handler {
 public:
  virtual ~lex_handler() {}

  // Invoked on a documentation comment such as `/** ... */` or `/// ...`.
  virtual void on_doc_comment(const char* text, source_location loc) = 0;
};

// A Thrift lexer.
class lexer {
 private:
  lex_handler* handler_;
  diagnostics_engine* diags_;
  fmt::string_view source_; // Source being lexed; has a terminating '\0'.
  source_location start_;
  const char* ptr_; // Current position in the source.
  const char* token_start_;

  const char* end() const { return source_.data() + source_.size() - 1; }

  // Converts a pointer within source into a location.
  source_location location(const char* p) const {
    return start_ + (p - source_.data());
  }

  source_range token_source_range() const {
    return {location(token_start_), location(ptr_)};
  }

  // Returns the string representation of the last token reported via
  // `get_next_token` or `lex_handler`. If no token has been reported returns
  // an empty string.
  std::string token_text() const { return {token_start_, ptr_}; }

  void start_token() { token_start_ = ptr_; }

  // Reports a failure if the parsed value cannot fit in the widest supported
  // representation, i.e. int64_t and double.
  yy::parser::symbol_type make_int_constant(int offset, int base);
  yy::parser::symbol_type make_float_constant();

  template <typename... T>
  yy::parser::symbol_type report_error(
      fmt::format_string<T...> msg, T&&... args);
  yy::parser::symbol_type unexpected_token();

  enum class comment_lex_result { skipped, doc_comment, unterminated };

  void skip_line_comment();
  bool lex_doc_comment();
  comment_lex_result lex_block_comment();
  comment_lex_result lex_whitespace_or_comment();

 public:
  lexer(lex_handler& handler, diagnostics_engine& diags, source src);

  lexer(lexer&& other) = default;
  lexer& operator=(lexer&& other) = default;

  yy::parser::symbol_type get_next_token();

  // Returns the current source location. If a token has been returned via
  // `get_next_token` or `lex_handler` the returned location points one past
  // the end of this token. Otherwise it points to the source start.
  source_location location() const { return location(ptr_); }
};

} // namespace compiler
} // namespace thrift
} // namespace apache
