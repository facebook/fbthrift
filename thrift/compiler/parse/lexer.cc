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

#include <thrift/compiler/parse/lexer.h>

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <algorithm>
#include <unordered_map>
#include <utility>

#include <thrift/compiler/diagnostic.h>

using apache::thrift::compiler::yy::parser;

namespace apache {
namespace thrift {
namespace compiler {

namespace {

bool is_whitespace(char c) {
  return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

bool is_letter(char c) {
  return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

bool is_bin_digit(char c) {
  return c == '0' || c == '1';
}
bool is_oct_digit(char c) {
  return c >= '0' && c <= '7';
}
bool is_dec_digit(char c) {
  return c >= '0' && c <= '9';
}
bool is_hex_digit(char c) {
  return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') ||
      (c >= 'A' && c <= 'F');
}

bool is_identifier_char(char c) {
  return is_letter(c) || is_dec_digit(c) || c == '_' || c == '.';
}

// Lexes a decimal constant of the form [0-9]+. Returns a pointer past the end
// if the constant has been lexed; `none` otherwise.
const char* lex_dec_constant(const char* p, const char* none = nullptr) {
  if (!is_dec_digit(*p)) {
    return none;
  }
  do {
    ++p;
  } while (is_dec_digit(*p));
  return p;
}

// Lexes a float exponent of the form [eE][+-]?[0-9]+. Returns a pointer past
// the end if the exponent has been lexed; `none` otherwise.
const char* lex_float_exponent(const char* p, const char* none = nullptr) {
  if (*p != 'e' && *p != 'E') {
    return none;
  }
  ++p; // Consume 'e' or 'E'.
  if (*p == '+' || *p == '-') {
    ++p; // Consume the sign.
  }
  return lex_dec_constant(p);
}

// Lexes a float constant in the form [0-9]+ followed by an optional exponent.
// Returns a pointer past the end if the constant has been lexed; nullptr
// otherwise.
const char* lex_float_constant(const char* p) {
  p = lex_dec_constant(p);
  return p ? lex_float_exponent(p, p) : nullptr;
}

// Bison may pass location by value or by reference.
using location_arg = std::conditional<
    std::is_same<
        decltype(parser::make_tok_void),
        parser::symbol_type(source_range)>::value,
    source_range,
    const source_range&>::type;
using make_token_fun = parser::symbol_type (*)(location_arg);

const std::unordered_map<std::string, make_token_fun> keywords = {
    {"false",
     [](location_arg loc) {
       return parser::make_tok_bool_constant(false, loc);
     }},
    {"true",
     [](location_arg loc) {
       return parser::make_tok_bool_constant(true, loc);
     }},
    {"include", parser::make_tok_include},
    {"cpp_include", parser::make_tok_cpp_include},
    {"hs_include", parser::make_tok_hs_include},
    {"package", parser::make_tok_package},
    {"namespace", parser::make_tok_namespace},
    {"void", parser::make_tok_void},
    {"bool", parser::make_tok_bool},
    {"byte", parser::make_tok_byte},
    {"i16", parser::make_tok_i16},
    {"i32", parser::make_tok_i32},
    {"i64", parser::make_tok_i64},
    {"double", parser::make_tok_double},
    {"float", parser::make_tok_float},
    {"string", parser::make_tok_string},
    {"binary", parser::make_tok_binary},
    {"map", parser::make_tok_map},
    {"list", parser::make_tok_list},
    {"set", parser::make_tok_set},
    {"sink", parser::make_tok_sink},
    {"stream", parser::make_tok_stream},
    {"interaction", parser::make_tok_interaction},
    {"performs", parser::make_tok_performs},
    {"oneway", parser::make_tok_oneway},
    {"idempotent", parser::make_tok_idempotent},
    {"readonly", parser::make_tok_readonly},
    {"safe", parser::make_tok_safe},
    {"transient", parser::make_tok_transient},
    {"stateful", parser::make_tok_stateful},
    {"permanent", parser::make_tok_permanent},
    {"server", parser::make_tok_server},
    {"client", parser::make_tok_client},
    {"typedef", parser::make_tok_typedef},
    {"struct", parser::make_tok_struct},
    {"union", parser::make_tok_union},
    {"exception", parser::make_tok_exception},
    {"extends", parser::make_tok_extends},
    {"throws", parser::make_tok_throws},
    {"service", parser::make_tok_service},
    {"enum", parser::make_tok_enum},
    {"const", parser::make_tok_const},
    {"required", parser::make_tok_required},
    {"optional", parser::make_tok_optional},
};

} // namespace

lexer::lexer(
    source_manager& sm,
    lex_handler& handler,
    diagnostics_engine& diags,
    source src)
    : source_mgr_(&sm),
      handler_(&handler),
      diags_(&diags),
      source_(src.text),
      start_(src.start) {
  ptr_ = source_.data();
  token_start_ = ptr_;
}

parser::symbol_type lexer::make_int_constant(int offset, int base) {
  std::string text = token_text();
  errno = 0;
  uint64_t val = strtoull(text.c_str() + offset, nullptr, base);
  if (errno == ERANGE) {
    return report_error(
        [&](auto& o) { o << "This integer is too big: " << text; });
  }
  return parser::make_tok_int_constant(val, token_source_range());
}

parser::symbol_type lexer::make_float_constant() {
  std::string text = token_text();
  errno = 0;
  double val = strtod(text.c_str(), nullptr);
  if (errno == ERANGE) {
    if (val == 0) {
      return report_error(
          [&](auto& o) { o << "This number is too infinitesimal: " << text; });
    } else if (val == HUGE_VAL) {
      return report_error(
          [&](auto& o) { o << "This number is too big: " << text; });
    } else if (val == -HUGE_VAL) {
      return report_error(
          [&](auto& o) { o << "This number is too small: " << text; });
    }
    // Allow subnormals.
  }
  return parser::make_tok_dub_constant(val, token_source_range());
}

template <typename... T>
parser::symbol_type lexer::report_error(T&&... args) {
  auto loc = resolved_location(location(), *source_mgr_);
  diags_->report(
      diagnostic_level::failure,
      loc.file_name(),
      loc.line(),
      token_text(),
      std::forward<T>(args)...);
  return parser::make_tok_error(token_source_range());
}

parser::symbol_type lexer::unexpected_token() {
  return report_error([&](auto& o) {
    o << "Unexpected token in input: " << token_text() << "\n";
  });
}

void lexer::skip_line_comment() {
  ptr_ = std::find(ptr_, end(), '\n');
}

bool lexer::lex_doc_comment() {
  assert(strncmp(ptr_, "///", 3) == 0);
  start_token();
  bool is_inline = ptr_[3] == '<';
  const char* prefix = ptr_;
  size_t prefix_size = is_inline ? 4 : 3;
  do {
    if (!is_inline && ptr_[3] == '/') {
      break;
    }
    ptr_ += prefix_size; // Skip "///" or "///<".
    skip_line_comment();
    while (is_whitespace(*ptr_)) {
      ++ptr_;
    }
  } while (strncmp(ptr_, prefix, prefix_size) == 0);
  if (!is_inline) {
    handler_->on_doc_comment(token_text().c_str(), location());
  }
  return is_inline;
}

lexer::comment_lex_result lexer::lex_block_comment() {
  assert(strncmp(ptr_, "/*", 2) == 0);
  const char* p = ptr_ + 2; // Skip "/*".
  start_token();
  do {
    p = std::find(p, end(), '*');
    if (!*p) { // EOF while lexing a block comment.
      return comment_lex_result::unterminated;
    }
    ++p; // Skip '*'.
  } while (*p != '/');
  ptr_ = p + 1; // Skip '/'.
  if (token_start_[2] == '*') {
    if (token_start_[3] == '<') {
      return comment_lex_result::doc_comment;
    }
    // Ignore comments containing only '*'s.
    auto non_star = std::find_if(
        token_start_ + 2, ptr_ - 1, [](char c) { return c != '*'; });
    if (non_star != ptr_ - 1) {
      handler_->on_doc_comment(token_text().c_str(), location());
    }
  }
  return comment_lex_result::skipped;
}

lexer::comment_lex_result lexer::lex_whitespace_or_comment() {
  for (;;) {
    switch (*ptr_) {
      case '\n':
      case ' ':
      case '\t':
      case '\r':
        ++ptr_;
        break;
      case '/':
        if (ptr_[1] == '/') {
          if (ptr_[2] == '/' && ptr_[3] != '/') {
            if (lex_doc_comment()) {
              return comment_lex_result::doc_comment;
            }
            continue;
          }
          ptr_ += 2; // Skip "//".
          skip_line_comment();
          break;
        }
        if (ptr_[1] == '*') {
          comment_lex_result res = lex_block_comment();
          if (res != comment_lex_result::skipped) {
            return res;
          }
          continue;
        }
        return comment_lex_result::skipped;
      case '#':
        ++ptr_;
        skip_line_comment();
        break;
      default:
        return comment_lex_result::skipped;
    }
  }
}

parser::symbol_type lexer::get_next_token() {
  if (lex_whitespace_or_comment() == comment_lex_result::doc_comment) {
    return parser::make_tok_inline_doc(token_text(), token_source_range());
  }

  start_token();

  char c = *ptr_++;
  if (is_letter(c) || c == '_') {
    // Lex an identifier or a keyword.
    while (is_identifier_char(*ptr_)) {
      ++ptr_;
    }
    auto text = token_text();
    auto it = keywords.find(text);
    if (it != keywords.end()) {
      return it->second(token_source_range());
    }
    return parser::make_tok_identifier(text, token_source_range());
  } else if (c == '.') {
    if (const char* p = lex_float_constant(ptr_)) {
      ptr_ = p;
      return make_float_constant();
    }
  } else if (is_dec_digit(c)) {
    if (c == '0') {
      switch (*ptr_) {
        case 'x':
        case 'X':
          // Lex a hexadecimal constant.
          if (!is_hex_digit(ptr_[1])) {
            return unexpected_token();
          }
          ptr_ += 2;
          while (is_hex_digit(*ptr_)) {
            ++ptr_;
          }
          return make_int_constant(2, 16);
        case 'b':
        case 'B':
          // Lex a binary constant.
          if (!is_bin_digit(ptr_[1])) {
            return unexpected_token();
          }
          ptr_ += 2;
          while (is_bin_digit(*ptr_)) {
            ++ptr_;
          }
          return make_int_constant(2, 2);
      }
    }
    // Lex a decimal, octal or floating-point constant.
    ptr_ = lex_dec_constant(ptr_, ptr_);
    switch (*ptr_) {
      case '.':
        if (const char* p = lex_float_constant(ptr_ + 1)) {
          ptr_ = p;
          return make_float_constant();
        }
        break;
      case 'e':
      case 'E':
        if (const char* p = lex_float_exponent(ptr_)) {
          ptr_ = p;
          return make_float_constant();
        }
        break;
    }
    if (c != '0') {
      // Lex a decimal constant.
      return make_int_constant(0, 10);
    }
    // Lex an octal constant.
    const char* p = std::find_if(
        token_start_, ptr_, [](char c) { return !is_oct_digit(c); });
    if (p != ptr_) {
      return unexpected_token();
    }
    return make_int_constant(1, 8);
  } else if (c == '"' || c == '\'') {
    // Lex a string literal.
    const char* p = std::find(ptr_, end(), c);
    if (*p) {
      ptr_ = p + 1;
      return parser::make_tok_literal(
          std::string(token_start_ + 1, p), token_source_range());
    }
  } else if (!c && ptr_ > end()) {
    --ptr_; // Put '\0' back in case get_next_token() is called again.
    return parser::make_tok_eof(token_source_range());
  }

  // Lex operators and punctuators.
  switch (c) {
    case '{':
      return parser::make_tok_char_bracket_curly_l(token_source_range());
    case '}':
      return parser::make_tok_char_bracket_curly_r(token_source_range());
    case ',':
      return parser::make_tok_char_comma(token_source_range());
    case ';':
      return parser::make_tok_char_semicolon(token_source_range());
    case '=':
      return parser::make_tok_char_equal(token_source_range());
    case '[':
      return parser::make_tok_char_bracket_square_l(token_source_range());
    case ']':
      return parser::make_tok_char_bracket_square_r(token_source_range());
    case ':':
      return parser::make_tok_char_colon(token_source_range());
    case '(':
      return parser::make_tok_char_bracket_round_l(token_source_range());
    case ')':
      return parser::make_tok_char_bracket_round_r(token_source_range());
    case '<':
      return parser::make_tok_char_bracket_angle_l(token_source_range());
    case '>':
      return parser::make_tok_char_bracket_angle_r(token_source_range());
    case '@':
      return parser::make_tok_char_at_sign(token_source_range());
    case '-':
      return parser::make_tok_char_minus(token_source_range());
    case '+':
      return parser::make_tok_char_plus(token_source_range());
  }

  return unexpected_token();
}

} // namespace compiler
} // namespace thrift
} // namespace apache
