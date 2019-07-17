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

%{

// Flex normally adds: #include <unistd.h>. This include is not supported
// by MSVC. With this, we get rid of that include to compile with MSVC.
#ifdef _WIN32
#  define YY_NO_UNISTD_H
// Provides isatty and fileno
#  include <io.h>
#endif

#include <errno.h>

#include "thrift/compiler/parse/parsing_driver.h"

using parsing_driver = apache::thrift::parsing_driver;

/**
 * Note macro expansion because this is different between OSS and internal
 * build, sigh.
 */
#include THRIFTY_HH

YY_DECL;

static void thrift_reserved_keyword(parsing_driver& driver, char* keyword) {
  driver.yyerror("Cannot use reserved language keyword: \"%s\"\n", keyword);
  driver.end_parsing();
}

static void integer_overflow(parsing_driver& driver, char* text) {
  driver.yyerror("This integer is too big: \"%s\"\n", text);
  driver.end_parsing();
}

static void unexpected_token(parsing_driver& driver, char* text) {
  driver.yyerror("Unexpected token in input: \"%s\"\n", text);
  driver.end_parsing();
}

/**
 * Current level of '{}' blocks. Some keywords (e.g. 'view') are considered as
 * reserved only if appears at the top level and might be used for other
 * purposes like field or argument names.
 */
int g_scope_level = 0;

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
doctext       ("/**"([^*/]|[^*]"/"|"*"[^/])*"*"*"*/")
comment       ("//"[^\n]*)
unixcomment   ("#"[^\n]*)
symbol        ([:;\,\{\}\(\)\=<>\[\]])
dliteral      ("\""[^"]*"\"")
sliteral      ("'"[^']*"'")
st_identifier ([a-zA-Z-][\.a-zA-Z_0-9-]*)


%%

{whitespace}         { /* do nothing */                 }
{sillycomm}          { /* do nothing */                 }
{multicomm}          { /* do nothing */                 }
{comment}            { /* do nothing */                 }
{unixcomment}        { /* do nothing */                 }

"{"                  {
  ++g_scope_level;
  return apache::thrift::yy::parser::make_tok_char_bracket_curly_l();
}
"}"                  {
  --g_scope_level;
  return apache::thrift::yy::parser::make_tok_char_bracket_curly_r();
}
{symbol}             {
  switch (yytext[0]) {
  case ',':
    return apache::thrift::yy::parser::make_tok_char_comma();
  case ';':
    return apache::thrift::yy::parser::make_tok_char_semicolon();
  case '{':
    return apache::thrift::yy::parser::make_tok_char_bracket_curly_l();
  case '}':
    return apache::thrift::yy::parser::make_tok_char_bracket_curly_r();
  case '=':
    return apache::thrift::yy::parser::make_tok_char_equal();
  case '[':
    return apache::thrift::yy::parser::make_tok_char_bracket_square_l();
  case ']':
    return apache::thrift::yy::parser::make_tok_char_bracket_square_r();
  case ':':
    return apache::thrift::yy::parser::make_tok_char_colon();
  case '(':
    return apache::thrift::yy::parser::make_tok_char_bracket_round_l();
  case ')':
    return apache::thrift::yy::parser::make_tok_char_bracket_round_r();
  case '<':
    return apache::thrift::yy::parser::make_tok_char_bracket_angle_l();
  case '>':
    return apache::thrift::yy::parser::make_tok_char_bracket_angle_r();
  }

  driver.failure("Invalid symbol encountered.");
}

"false"              { return apache::thrift::yy::parser::make_tok_bool_constant(0); }
"true"               { return apache::thrift::yy::parser::make_tok_bool_constant(1); }

"namespace"          { return apache::thrift::yy::parser::make_tok_namespace();            }
"cpp_namespace"      { return apache::thrift::yy::parser::make_tok_cpp_namespace();        }
"cpp_include"        { return apache::thrift::yy::parser::make_tok_cpp_include();          }
"hs_include"         { return apache::thrift::yy::parser::make_tok_hs_include();           }
"java_package"       { return apache::thrift::yy::parser::make_tok_java_package();         }
"cocoa_prefix"       { return apache::thrift::yy::parser::make_tok_cocoa_prefix();         }
"csharp_namespace"   { return apache::thrift::yy::parser::make_tok_csharp_namespace();     }
"php_namespace"      { return apache::thrift::yy::parser::make_tok_php_namespace();        }
"py_module"          { return apache::thrift::yy::parser::make_tok_py_module();            }
"perl_package"       { return apache::thrift::yy::parser::make_tok_perl_package();         }
"ruby_namespace"     { return apache::thrift::yy::parser::make_tok_ruby_namespace();       }
"smalltalk_category" { return apache::thrift::yy::parser::make_tok_smalltalk_category();   }
"smalltalk_prefix"   { return apache::thrift::yy::parser::make_tok_smalltalk_prefix();     }
"include"            { return apache::thrift::yy::parser::make_tok_include();              }
"void"               { return apache::thrift::yy::parser::make_tok_void();                 }
"bool"               { return apache::thrift::yy::parser::make_tok_bool();                 }
"byte"               { return apache::thrift::yy::parser::make_tok_byte();                 }
"i16"                { return apache::thrift::yy::parser::make_tok_i16();                  }
"i32"                { return apache::thrift::yy::parser::make_tok_i32();                  }
"i64"                { return apache::thrift::yy::parser::make_tok_i64();                  }
"double"             { return apache::thrift::yy::parser::make_tok_double();               }
"float"              { return apache::thrift::yy::parser::make_tok_float();                }
"string"             { return apache::thrift::yy::parser::make_tok_string();               }
"binary"             { return apache::thrift::yy::parser::make_tok_binary();               }
"map"                { return apache::thrift::yy::parser::make_tok_map();                  }
"list"               { return apache::thrift::yy::parser::make_tok_list();                 }
"set"                { return apache::thrift::yy::parser::make_tok_set();                  }
"stream"             { return apache::thrift::yy::parser::make_tok_stream();               }
"oneway"             { return apache::thrift::yy::parser::make_tok_oneway();               }
"typedef"            { return apache::thrift::yy::parser::make_tok_typedef();              }
"struct"             { return apache::thrift::yy::parser::make_tok_struct();               }
"union"              { return apache::thrift::yy::parser::make_tok_union();                }
"exception"          { return apache::thrift::yy::parser::make_tok_xception();             }
"extends"            { return apache::thrift::yy::parser::make_tok_extends();              }
"stream throws"      {
  /* this is a hack; lex doesn't allow whitespace in trailing context,
   * so match entire "stream throws" as a token
   */
  return apache::thrift::yy::parser::make_tok_streamthrows();
}
"throws"             { return apache::thrift::yy::parser::make_tok_throws();               }
"service"            { return apache::thrift::yy::parser::make_tok_service();              }
"enum"               { return apache::thrift::yy::parser::make_tok_enum();                 }
"const"              { return apache::thrift::yy::parser::make_tok_const();                }
"required"           { return apache::thrift::yy::parser::make_tok_required();             }
"optional"           { return apache::thrift::yy::parser::make_tok_optional();             }


"abstract"           { thrift_reserved_keyword(driver, yytext); }
"and"                { thrift_reserved_keyword(driver, yytext); }
"args"               { thrift_reserved_keyword(driver, yytext); }
"as"                 { thrift_reserved_keyword(driver, yytext); }
"assert"             { thrift_reserved_keyword(driver, yytext); }
"auto"               { thrift_reserved_keyword(driver, yytext); }
"break"              { thrift_reserved_keyword(driver, yytext); }
"case"               { thrift_reserved_keyword(driver, yytext); }
"char"               { thrift_reserved_keyword(driver, yytext); }
"class"              { thrift_reserved_keyword(driver, yytext); }
"continue"           { thrift_reserved_keyword(driver, yytext); }
"declare"            { thrift_reserved_keyword(driver, yytext); }
"def"                { thrift_reserved_keyword(driver, yytext); }
"default"            { thrift_reserved_keyword(driver, yytext); }
"del"                { thrift_reserved_keyword(driver, yytext); }
"do"                 { thrift_reserved_keyword(driver, yytext); }
"elif"               { thrift_reserved_keyword(driver, yytext); }
"else"               { thrift_reserved_keyword(driver, yytext); }
"elseif"             { thrift_reserved_keyword(driver, yytext); }
"except"             { thrift_reserved_keyword(driver, yytext); }
"exec"               { thrift_reserved_keyword(driver, yytext); }
"extern"             { thrift_reserved_keyword(driver, yytext); }
"finally"            { thrift_reserved_keyword(driver, yytext); }
"for"                { thrift_reserved_keyword(driver, yytext); }
"foreach"            { thrift_reserved_keyword(driver, yytext); }
"function"           { thrift_reserved_keyword(driver, yytext); }
"global"             { thrift_reserved_keyword(driver, yytext); }
"goto"               { thrift_reserved_keyword(driver, yytext); }
"if"                 { thrift_reserved_keyword(driver, yytext); }
"implements"         { thrift_reserved_keyword(driver, yytext); }
"import"             { thrift_reserved_keyword(driver, yytext); }
"in"                 { thrift_reserved_keyword(driver, yytext); }
"int"                { thrift_reserved_keyword(driver, yytext); }
"inline"             { thrift_reserved_keyword(driver, yytext); }
"instanceof"         { thrift_reserved_keyword(driver, yytext); }
"interface"          { thrift_reserved_keyword(driver, yytext); }
"is"                 { thrift_reserved_keyword(driver, yytext); }
"lambda"             { thrift_reserved_keyword(driver, yytext); }
"long"               { thrift_reserved_keyword(driver, yytext); }
"native"             { thrift_reserved_keyword(driver, yytext); }
"new"                { thrift_reserved_keyword(driver, yytext); }
"not"                { thrift_reserved_keyword(driver, yytext); }
"or"                 { thrift_reserved_keyword(driver, yytext); }
"pass"               { thrift_reserved_keyword(driver, yytext); }
"public"             { thrift_reserved_keyword(driver, yytext); }
"print"              { thrift_reserved_keyword(driver, yytext); }
"private"            { thrift_reserved_keyword(driver, yytext); }
"protected"          { thrift_reserved_keyword(driver, yytext); }
"raise"              { thrift_reserved_keyword(driver, yytext); }
"register"           { thrift_reserved_keyword(driver, yytext); }
"return"             { thrift_reserved_keyword(driver, yytext); }
"short"              { thrift_reserved_keyword(driver, yytext); }
"signed"             { thrift_reserved_keyword(driver, yytext); }
"sizeof"             { thrift_reserved_keyword(driver, yytext); }
"static"             { thrift_reserved_keyword(driver, yytext); }
"switch"             { thrift_reserved_keyword(driver, yytext); }
"synchronized"       { thrift_reserved_keyword(driver, yytext); }
"template"           { thrift_reserved_keyword(driver, yytext); }
"this"               { thrift_reserved_keyword(driver, yytext); }
"throw"              { thrift_reserved_keyword(driver, yytext); }
"transient"          { thrift_reserved_keyword(driver, yytext); }
"try"                { thrift_reserved_keyword(driver, yytext); }
"unsigned"           { thrift_reserved_keyword(driver, yytext); }
"var"                { thrift_reserved_keyword(driver, yytext); }
"virtual"            { thrift_reserved_keyword(driver, yytext); }
"volatile"           { thrift_reserved_keyword(driver, yytext); }
"while"              { thrift_reserved_keyword(driver, yytext); }
"with"               { thrift_reserved_keyword(driver, yytext); }
"yield"              { thrift_reserved_keyword(driver, yytext); }
"Object"             { thrift_reserved_keyword(driver, yytext); }
"Client"             { thrift_reserved_keyword(driver, yytext); }
"IFace"              { thrift_reserved_keyword(driver, yytext); }
"Processor"          { thrift_reserved_keyword(driver, yytext); }

{octconstant} {
  errno = 0;
  int64_t val = strtoll(yytext+1, NULL, 8);
  if (errno == ERANGE) {
    integer_overflow(driver, yytext);
  }
  return apache::thrift::yy::parser::make_tok_int_constant(val);
}

{intconstant} {
  errno = 0;
  int64_t val = strtoll(yytext, NULL, 10);
  if (errno == ERANGE) {
    integer_overflow(driver, yytext);
  }
  return apache::thrift::yy::parser::make_tok_int_constant(val);
}

{hexconstant} {
  errno = 0;
  int64_t val = strtoll(yytext+2, NULL, 16);
  if (errno == ERANGE) {
    integer_overflow(driver, yytext);
  }
  return apache::thrift::yy::parser::make_tok_int_constant(val);
}

{dubconstant} {
  double val = atof(yytext);
  return apache::thrift::yy::parser::make_tok_dub_constant(val);
}

{identifier} {
  return apache::thrift::yy::parser::make_tok_identifier(std::string{yytext});
}

{st_identifier} {
  return apache::thrift::yy::parser::make_tok_st_identifier(std::string{yytext});
}

{dliteral} {
  std::string val{yytext + 1};
  val = val.substr(0, val.length() - 1);
  return apache::thrift::yy::parser::make_tok_literal(std::move(val));
}

{sliteral} {
  std::string val{yytext + 1};
  val = val.substr(0, val.length() - 1);
  return apache::thrift::yy::parser::make_tok_literal(std::move(val));
}

{doctext} {
 /* This does not show up in the parse tree. */
 /* Rather, the parser will grab it out of the global. */
  if (driver.mode == apache::thrift::parsing_mode::PROGRAM) {
    std::string doctext{yytext + 3};
    doctext = doctext.substr(0, doctext.length() - 2);

    driver.clear_doctext();
    driver.doctext = driver.clean_up_doctext(doctext);
    driver.doctext_lineno = yylineno;
  }
}

. {
  unexpected_token(driver, yytext);
}

<<EOF>> {
  return apache::thrift::yy::parser::make_tok_eof();
}

%%

/* vim: filetype=lex
*/
