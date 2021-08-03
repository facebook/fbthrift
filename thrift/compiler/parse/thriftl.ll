/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

/**
 * Thrift scanner.
 *
 * Tokenizes a thrift definition file.
 */

%option noyywrap
%option reentrant
%option yylineno
%option nounistd
%option never-interactive
%option prefix="fbthrift_compiler_parse_"
%option bison-locations

%{

#include <errno.h>
#include <stdlib.h>

#include "thrift/compiler/parse/parsing_driver.h"

/**
 * Note macro expansion because this is different between OSS and internal
 * build, sigh.
 */
#include THRIFTY_HH

YY_DECL;

namespace {

using apache::thrift::compiler::parsing_driver;

void integer_overflow(parsing_driver& driver, const char* text) {
  driver.failure([&](auto& o) { o << "This integer is too big: " << text << "\n"; });
}

void unexpected_token(parsing_driver& driver, const char* text) {
  driver.failure([&](auto& o) { o << "Unexpected token in input: " << text << "\n"; });
}

#define YY_USER_ACTION driver.compute_location(*yylloc, *yylval, yytext);

} // namespace

%}

/**
 * Helper definitions, comments, constants, and whatnot
 */

intconstant   ([+-]?[1-9][0-9]*|"0")
octconstant   ("0"[0-7]+)
hexconstant   ("0x"[0-9A-Fa-f]+)
dubconstant   ([+-]?[0-9]*(\.[0-9]+)?([eE][+-]?[0-9]+)?)
identifier    ([a-zA-Z_][\.a-zA-Z_0-9]*)
whitespace    ([ \t\r\n]*)
sillycomm     ("/*""*"*"*/")
multicomm     ("/*"[^*]"/"*([^*/]|[^*]"/"|"*"[^/])*"*"*"*/")
doctext       (("/**"([^*/]|[^*]"/"|"*"[^/])*"*"*"*/")|("///"(\n|[^/\n][^\n]*){whitespace})+)
comment       ("//"[^\n]*)
unixcomment   ("#"[^\n]*)
symbol        ([:;\,\{\}\(\)\=<>\[\]@])
dliteral      ("\""[^"]*"\"")
sliteral      ("'"[^']*"'")

%%

{whitespace}         { /* do nothing */                 }
{sillycomm}          { /* do nothing */                 }
{multicomm}          { /* do nothing */                 }
{comment}            { /* do nothing */                 }
{unixcomment}        { /* do nothing */                 }

"{"                  {
  return apache::thrift::compiler::yy::parser::make_tok_char_bracket_curly_l(*yylloc);
}
"}"                  {
  return apache::thrift::compiler::yy::parser::make_tok_char_bracket_curly_r(*yylloc);
}

{symbol}             {
  switch (yytext[0]) {
  case ',':
    return apache::thrift::compiler::yy::parser::make_tok_char_comma(*yylloc);
  case ';':
    return apache::thrift::compiler::yy::parser::make_tok_char_semicolon(*yylloc);
  case '{':
    return apache::thrift::compiler::yy::parser::make_tok_char_bracket_curly_l(*yylloc);
  case '}':
    return apache::thrift::compiler::yy::parser::make_tok_char_bracket_curly_r(*yylloc);
  case '=':
    return apache::thrift::compiler::yy::parser::make_tok_char_equal(*yylloc);
  case '[':
    return apache::thrift::compiler::yy::parser::make_tok_char_bracket_square_l(*yylloc);
  case ']':
    return apache::thrift::compiler::yy::parser::make_tok_char_bracket_square_r(*yylloc);
  case ':':
    return apache::thrift::compiler::yy::parser::make_tok_char_colon(*yylloc);
  case '(':
    return apache::thrift::compiler::yy::parser::make_tok_char_bracket_round_l(*yylloc);
  case ')':
    return apache::thrift::compiler::yy::parser::make_tok_char_bracket_round_r(*yylloc);
  case '<':
    return apache::thrift::compiler::yy::parser::make_tok_char_bracket_angle_l(*yylloc);
  case '>':
    return apache::thrift::compiler::yy::parser::make_tok_char_bracket_angle_r(*yylloc);
  case '@':
    return apache::thrift::compiler::yy::parser::make_tok_char_at_sign(*yylloc);
  }

  driver.failure("Invalid symbol encountered.");
}

"false"              { return apache::thrift::compiler::yy::parser::make_tok_bool_constant(0, *yylloc); }
"true"               { return apache::thrift::compiler::yy::parser::make_tok_bool_constant(1, *yylloc); }

"namespace"          { return apache::thrift::compiler::yy::parser::make_tok_namespace(*yylloc);            }
"cpp_include"        { return apache::thrift::compiler::yy::parser::make_tok_cpp_include(*yylloc);          }
"hs_include"         { return apache::thrift::compiler::yy::parser::make_tok_hs_include(*yylloc);           }
"include"            { return apache::thrift::compiler::yy::parser::make_tok_include(*yylloc);              }
"void"               { return apache::thrift::compiler::yy::parser::make_tok_void(*yylloc);                 }
"bool"               { return apache::thrift::compiler::yy::parser::make_tok_bool(*yylloc);                 }
"byte"               { return apache::thrift::compiler::yy::parser::make_tok_byte(*yylloc);                 }
"i16"                { return apache::thrift::compiler::yy::parser::make_tok_i16(*yylloc);                  }
"i32"                { return apache::thrift::compiler::yy::parser::make_tok_i32(*yylloc);                  }
"i64"                { return apache::thrift::compiler::yy::parser::make_tok_i64(*yylloc);                  }
"double"             { return apache::thrift::compiler::yy::parser::make_tok_double(*yylloc);               }
"float"              { return apache::thrift::compiler::yy::parser::make_tok_float(*yylloc);                }
"string"             { return apache::thrift::compiler::yy::parser::make_tok_string(*yylloc);               }
"binary"             { return apache::thrift::compiler::yy::parser::make_tok_binary(*yylloc);               }
"map"                { return apache::thrift::compiler::yy::parser::make_tok_map(*yylloc);                  }
"list"               { return apache::thrift::compiler::yy::parser::make_tok_list(*yylloc);                 }
"set"                { return apache::thrift::compiler::yy::parser::make_tok_set(*yylloc);                  }
"sink"               { return apache::thrift::compiler::yy::parser::make_tok_sink(*yylloc);                 }
"stream"             { return apache::thrift::compiler::yy::parser::make_tok_stream(*yylloc);               }
"interaction"        { return apache::thrift::compiler::yy::parser::make_tok_interaction(*yylloc);          }
"performs"           { return apache::thrift::compiler::yy::parser::make_tok_performs(*yylloc);             }
"oneway"             { return apache::thrift::compiler::yy::parser::make_tok_oneway(*yylloc);               }
"idempotent"         { return apache::thrift::compiler::yy::parser::make_tok_idempotent(*yylloc);           }
"readonly"           { return apache::thrift::compiler::yy::parser::make_tok_readonly(*yylloc);             }
"safe"               { return apache::thrift::compiler::yy::parser::make_tok_safe(*yylloc);                 }
"transient"          { return apache::thrift::compiler::yy::parser::make_tok_transient(*yylloc);            }
"stateful"           { return apache::thrift::compiler::yy::parser::make_tok_stateful(*yylloc);             }
"permanent"          { return apache::thrift::compiler::yy::parser::make_tok_permanent(*yylloc);            }
"server"             { return apache::thrift::compiler::yy::parser::make_tok_server(*yylloc);               }
"client"             { return apache::thrift::compiler::yy::parser::make_tok_client(*yylloc);               }
"typedef"            { return apache::thrift::compiler::yy::parser::make_tok_typedef(*yylloc);              }
"struct"             { return apache::thrift::compiler::yy::parser::make_tok_struct(*yylloc);               }
"union"              { return apache::thrift::compiler::yy::parser::make_tok_union(*yylloc);                }
"exception"          { return apache::thrift::compiler::yy::parser::make_tok_exception(*yylloc);            }
"extends"            { return apache::thrift::compiler::yy::parser::make_tok_extends(*yylloc);              }
"throws"             { return apache::thrift::compiler::yy::parser::make_tok_throws(*yylloc);               }
"service"            { return apache::thrift::compiler::yy::parser::make_tok_service(*yylloc);              }
"enum"               { return apache::thrift::compiler::yy::parser::make_tok_enum(*yylloc);                 }
"const"              { return apache::thrift::compiler::yy::parser::make_tok_const(*yylloc);                }
"required"           { return apache::thrift::compiler::yy::parser::make_tok_required(*yylloc);             }
"optional"           { return apache::thrift::compiler::yy::parser::make_tok_optional(*yylloc);             }

{octconstant} {
  errno = 0;
  int64_t val = strtoll(yytext+1, NULL, 8);
  if (errno == ERANGE) {
    integer_overflow(driver, yytext);
  }
  return apache::thrift::compiler::yy::parser::make_tok_int_constant(val, *yylloc);
}

{intconstant} {
  errno = 0;
  int64_t val = strtoll(yytext, NULL, 10);
  if (errno == ERANGE) {
    integer_overflow(driver, yytext);
  }
  return apache::thrift::compiler::yy::parser::make_tok_int_constant(val, *yylloc);
}

{hexconstant} {
  errno = 0;
  int64_t val = strtoll(yytext+2, NULL, 16);
  if (errno == ERANGE) {
    integer_overflow(driver, yytext);
  }
  return apache::thrift::compiler::yy::parser::make_tok_int_constant(val, *yylloc);
}

{dubconstant} {
  double val = atof(yytext);
  return apache::thrift::compiler::yy::parser::make_tok_dub_constant(val, *yylloc);
}

{identifier} {
  return apache::thrift::compiler::yy::parser::make_tok_identifier(std::string{yytext}, *yylloc);
}

{dliteral} {
  std::string val{yytext + 1};
  val = val.substr(0, val.length() - 1);
  return apache::thrift::compiler::yy::parser::make_tok_literal(std::move(val), *yylloc);
}

{sliteral} {
  std::string val{yytext + 1};
  val = val.substr(0, val.length() - 1);
  return apache::thrift::compiler::yy::parser::make_tok_literal(std::move(val), *yylloc);
}

{doctext} {
 /* This does not show up in the parse tree. */
 /* Rather, the parser will grab it out of the global. */
  if (driver.mode == apache::thrift::compiler::parsing_mode::PROGRAM) {
    std::string doctext{yytext};

    /* Deal with prefix/suffix */
    if (doctext.compare(0, 3, "/**") == 0) {
      doctext = doctext.substr(3, doctext.length() - 3 - 2);
    } else if (doctext.compare(0, 3, "///") == 0) {
      doctext = doctext.substr(3, doctext.length() - 3);
    }

    driver.clear_doctext();
    driver.doctext = driver.clean_up_doctext(doctext);
    driver.doctext_lineno = yylineno;
  }
}

. {
  unexpected_token(driver, yytext);
}

<<EOF>> {
  return apache::thrift::compiler::yy::parser::make_tok_eof(*yylloc);
}

%%

/* vim: filetype=lex
*/
