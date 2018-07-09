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

%{

// Flex normally adds: #include <unistd.h>. This include is not supported
// by MSVC. With this, we get rid of that include to compile with MSVC.
#ifdef _WIN32
#  define YY_NO_UNISTD_H
#endif

#include <errno.h>

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

static void thrift_reserved_keyword(char* keyword) {
  yyerror("Cannot use reserved language keyword: \"%s\"\n", keyword);
  exit(1);
}

static void integer_overflow(char* text) {
  yyerror("This integer is too big: \"%s\"\n", text);
  exit(1);
}

static void unexpected_token(char* text) {
  yyerror("Unexpected token in input: \"%s\"\n", text);
  exit(1);
}

/**
 * Current level of '{}' blocks. Some keywords (e.g. 'view') are considered as
 * reserved only if appears at the top level and might be used for other
 * purposes like field or argument names.
 */
int g_scope_level = 0;

%}

/**
 * Provides the yylineno global, useful for debugging output
 */
%option lex-compat

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

  failure("Invalid symbol encountered.");
}

"false"              { return apache::thrift::yy::parser::make_tok_bool_constant(0); }
"true"               { return apache::thrift::yy::parser::make_tok_bool_constant(1); }

"namespace"          { return apache::thrift::yy::parser::make_tok_namespace();            }
"cpp_namespace"      { return apache::thrift::yy::parser::make_tok_cpp_namespace();        }
"cpp_include"        { return apache::thrift::yy::parser::make_tok_cpp_include();          }
"hs_include"         { return apache::thrift::yy::parser::make_tok_hs_include();           }
"cpp_type"           {
  yyerror("\"cpp_type\" is no longer allowed. "
    "Use the cpp.type annotation instead.\n");
  exit(1);
}
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
"slist"              { return apache::thrift::yy::parser::make_tok_slist();                }
"map"                { return apache::thrift::yy::parser::make_tok_map();                  }
"hash_map"           { return apache::thrift::yy::parser::make_tok_hash_map();             }
"list"               { return apache::thrift::yy::parser::make_tok_list();                 }
"set"                { return apache::thrift::yy::parser::make_tok_set();                  }
"hash_set"           { return apache::thrift::yy::parser::make_tok_hash_set();             }
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
"async" {
  pwarning(0, "\"async\" is deprecated.  It is called \"oneway\" now.\n");
  return apache::thrift::yy::parser::make_tok_oneway();
}


"abstract"           { thrift_reserved_keyword(yytext); }
"and"                { thrift_reserved_keyword(yytext); }
"args"               { thrift_reserved_keyword(yytext); }
"as"                 { thrift_reserved_keyword(yytext); }
"assert"             { thrift_reserved_keyword(yytext); }
"auto"               { thrift_reserved_keyword(yytext); }
"break"              { thrift_reserved_keyword(yytext); }
"case"               { thrift_reserved_keyword(yytext); }
"char"               { thrift_reserved_keyword(yytext); }
"class"              { thrift_reserved_keyword(yytext); }
"continue"           { thrift_reserved_keyword(yytext); }
"declare"            { thrift_reserved_keyword(yytext); }
"def"                { thrift_reserved_keyword(yytext); }
"default"            { thrift_reserved_keyword(yytext); }
"del"                { thrift_reserved_keyword(yytext); }
"delete"             { thrift_reserved_keyword(yytext); }
"do"                 { thrift_reserved_keyword(yytext); }
"elif"               { thrift_reserved_keyword(yytext); }
"else"               { thrift_reserved_keyword(yytext); }
"elseif"             { thrift_reserved_keyword(yytext); }
"except"             { thrift_reserved_keyword(yytext); }
"exec"               { thrift_reserved_keyword(yytext); }
"extern"             { thrift_reserved_keyword(yytext); }
"finally"            { thrift_reserved_keyword(yytext); }
"for"                { thrift_reserved_keyword(yytext); }
"foreach"            { thrift_reserved_keyword(yytext); }
"function"           { thrift_reserved_keyword(yytext); }
"global"             { thrift_reserved_keyword(yytext); }
"goto"               { thrift_reserved_keyword(yytext); }
"if"                 { thrift_reserved_keyword(yytext); }
"implements"         { thrift_reserved_keyword(yytext); }
"import"             { thrift_reserved_keyword(yytext); }
"in"                 { thrift_reserved_keyword(yytext); }
"int"                { thrift_reserved_keyword(yytext); }
"inline"             { thrift_reserved_keyword(yytext); }
"instanceof"         { thrift_reserved_keyword(yytext); }
"interface"          { thrift_reserved_keyword(yytext); }
"is"                 { thrift_reserved_keyword(yytext); }
"lambda"             { thrift_reserved_keyword(yytext); }
"long"               { thrift_reserved_keyword(yytext); }
"native"             { thrift_reserved_keyword(yytext); }
"new"                { thrift_reserved_keyword(yytext); }
"not"                { thrift_reserved_keyword(yytext); }
"or"                 { thrift_reserved_keyword(yytext); }
"pass"               { thrift_reserved_keyword(yytext); }
"public"             { thrift_reserved_keyword(yytext); }
"print"              { thrift_reserved_keyword(yytext); }
"private"            { thrift_reserved_keyword(yytext); }
"protected"          { thrift_reserved_keyword(yytext); }
"raise"              { thrift_reserved_keyword(yytext); }
"register"           { thrift_reserved_keyword(yytext); }
"return"             { thrift_reserved_keyword(yytext); }
"short"              { thrift_reserved_keyword(yytext); }
"signed"             { thrift_reserved_keyword(yytext); }
"sizeof"             { thrift_reserved_keyword(yytext); }
"static"             { thrift_reserved_keyword(yytext); }
"switch"             { thrift_reserved_keyword(yytext); }
"synchronized"       { thrift_reserved_keyword(yytext); }
"template"           { thrift_reserved_keyword(yytext); }
"this"               { thrift_reserved_keyword(yytext); }
"throw"              { thrift_reserved_keyword(yytext); }
"transient"          { thrift_reserved_keyword(yytext); }
"try"                { thrift_reserved_keyword(yytext); }
"unsigned"           { thrift_reserved_keyword(yytext); }
"var"                { thrift_reserved_keyword(yytext); }
"virtual"            { thrift_reserved_keyword(yytext); }
"volatile"           { thrift_reserved_keyword(yytext); }
"while"              { thrift_reserved_keyword(yytext); }
"with"               { thrift_reserved_keyword(yytext); }
"yield"              { thrift_reserved_keyword(yytext); }
"Object"             { thrift_reserved_keyword(yytext); }
"Client"             { thrift_reserved_keyword(yytext); }
"IFace"              { thrift_reserved_keyword(yytext); }
"Processor"          { thrift_reserved_keyword(yytext); }

{octconstant} {
  errno = 0;
  int64_t val = strtoll(yytext+1, NULL, 8);
  if (errno == ERANGE) {
    integer_overflow(yytext);
  }
  return apache::thrift::yy::parser::make_tok_int_constant(val);
}

{intconstant} {
  errno = 0;
  int64_t val = strtoll(yytext, NULL, 10);
  if (errno == ERANGE) {
    integer_overflow(yytext);
  }
  return apache::thrift::yy::parser::make_tok_int_constant(val);
}

{hexconstant} {
  errno = 0;
  int64_t val = strtoll(yytext+2, NULL, 16);
  if (errno == ERANGE) {
    integer_overflow(yytext);
  }
  return apache::thrift::yy::parser::make_tok_int_constant(val);
}

{dubconstant} {
  double val = atof(yytext);
  return apache::thrift::yy::parser::make_tok_dub_constant(val);
}

{identifier} {
  const char *val = strdup(yytext);
  return apache::thrift::yy::parser::make_tok_identifier(val);
}

{st_identifier} {
  const char *val = strdup(yytext);
  return apache::thrift::yy::parser::make_tok_st_identifier(val);
}

{dliteral} {
  char *val = strdup(yytext+1);
  val[strlen(val)-1] = '\0';
  const char *const_val = val;
  return apache::thrift::yy::parser::make_tok_literal(const_val);
}

{sliteral} {
  char *val = strdup(yytext+1);
  val[strlen(val)-1] = '\0';
  const char *const_val = val;
  return apache::thrift::yy::parser::make_tok_literal(const_val);
}

{doctext} {
 /* This does not show up in the parse tree. */
 /* Rather, the parser will grab it out of the global. */
  if (g_parse_mode == PROGRAM) {
    clear_doctext();
    g_doctext = strdup(yytext + 3);
    g_doctext[strlen(g_doctext) - 2] = '\0';
    g_doctext = clean_up_doctext(g_doctext);
    g_doctext_lineno = yylineno;
  }
}

. {
  unexpected_token(yytext);
}

<<EOF>> {
  return apache::thrift::yy::parser::make_tok_eof();
}

%%

/* vim: filetype=lex
*/
