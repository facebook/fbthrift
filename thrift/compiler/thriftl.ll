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

#include <errno.h>

#include "thrift/compiler/main.h"
#include "thrift/compiler/globals.h"
#include "thrift/compiler/parse/t_program.h"

/**
 * Must be included AFTER parse/t_program.h, but I can't remember why anymore
 * because I wrote this a while ago.
 */
#include "thrift/compiler/thrifty.h"

void thrift_reserved_keyword(char* keyword) {
  yyerror("Cannot use reserved language keyword: \"%s\"\n", keyword);
  exit(1);
}

void integer_overflow(char* text) {
  yyerror("This integer is too big: \"%s\"\n", text);
  exit(1);
}

void unexpected_token(char* text) {
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
  return yytext[0];
}
"}"                  {
  --g_scope_level;
  return yytext[0];
}
{symbol}             { return yytext[0];                }

"namespace"          { return tok_namespace;            }
"cpp_namespace"      { return tok_cpp_namespace;        }
"cpp_include"        { return tok_cpp_include;          }
"cpp_type"           {
  yyerror("\"cpp_type\" is no longer allowed. "
    "Use the cpp.type annotation instead.\n");
  exit(1);
}
"java_package"       { return tok_java_package;         }
"cocoa_prefix"       { return tok_cocoa_prefix;         }
"csharp_namespace"   { return tok_csharp_namespace;     }
"php_namespace"      { return tok_php_namespace;        }
"py_module"          { return tok_py_module;            }
"perl_package"       { return tok_perl_package;         }
"ruby_namespace"     { return tok_ruby_namespace;       }
"smalltalk_category" { return tok_smalltalk_category;   }
"smalltalk_prefix"   { return tok_smalltalk_prefix;     }
"xsd_all"            { return tok_xsd_all;              }
"xsd_optional"       { return tok_xsd_optional;         }
"xsd_nillable"       { return tok_xsd_nillable;         }
"xsd_namespace"      { return tok_xsd_namespace;        }
"xsd_attrs"          { return tok_xsd_attrs;            }
"include"            { return tok_include;              }
"void"               { return tok_void;                 }
"bool"               { return tok_bool;                 }
"byte"               { return tok_byte;                 }
"i16"                { return tok_i16;                  }
"i32"                { return tok_i32;                  }
"i64"                { return tok_i64;                  }
"double"             { return tok_double;               }
"float"              { return tok_float;                }
"string"             { return tok_string;               }
"binary"             { return tok_binary;               }
"slist"              { return tok_slist;                }
"senum"              { return tok_senum;                }
"map"                { return tok_map;                  }
"hash_map"           { return tok_hash_map;             }
"list"               { return tok_list;                 }
"set"                { return tok_set;                  }
"stream"             { return tok_stream;               }
"oneway"             { return tok_oneway;               }
"typedef"            { return tok_typedef;              }
"struct"             { return tok_struct;               }
"union"              { return tok_union;                }
"view"               {
  if (g_scope_level != 0) {
    yylval.id = strdup(yytext);
    return tok_identifier;
  }
  return tok_view;
}
"exception"          { return tok_xception;             }
"extends"            { return tok_extends;              }
"throws"             { return tok_throws;               }
"service"            { return tok_service;              }
"enum"               { return tok_enum;                 }
"const"              { return tok_const;                }
"required"           { return tok_required;             }
"optional"           { return tok_optional;             }
"async" {
  pwarning(0, "\"async\" is deprecated.  It is called \"oneway\" now.\n");
  return tok_oneway;
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
"const"              { thrift_reserved_keyword(yytext); }
"continue"           { thrift_reserved_keyword(yytext); }
"declare"            { thrift_reserved_keyword(yytext); }
"def"                { thrift_reserved_keyword(yytext); }
"default"            { thrift_reserved_keyword(yytext); }
"del"                { thrift_reserved_keyword(yytext); }
"delete"             { thrift_reserved_keyword(yytext); }
"do"                 { thrift_reserved_keyword(yytext); }
"double"             { thrift_reserved_keyword(yytext); }
"float"              { thrift_reserved_keyword(yytext); }
"elif"               { thrift_reserved_keyword(yytext); }
"else"               { thrift_reserved_keyword(yytext); }
"elseif"             { thrift_reserved_keyword(yytext); }
"except"             { thrift_reserved_keyword(yytext); }
"exec"               { thrift_reserved_keyword(yytext); }
"extern"             { thrift_reserved_keyword(yytext); }
"false"              { thrift_reserved_keyword(yytext); }
"finally"            { thrift_reserved_keyword(yytext); }
"float"              { thrift_reserved_keyword(yytext); }
"for"                { thrift_reserved_keyword(yytext); }
"foreach"            { thrift_reserved_keyword(yytext); }
"from"               {
  pwarning(0, "\"%s\" will soon become a reserved keyword.", yytext);
  yylval.id = strdup(yytext);
  return tok_identifier;
}
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
"struct"             { thrift_reserved_keyword(yytext); }
"switch"             { thrift_reserved_keyword(yytext); }
"synchronized"       { thrift_reserved_keyword(yytext); }
"template"           { thrift_reserved_keyword(yytext); }
"this"               { thrift_reserved_keyword(yytext); }
"throw"              { thrift_reserved_keyword(yytext); }
"transient"          { thrift_reserved_keyword(yytext); }
"true"               { thrift_reserved_keyword(yytext); }
"try"                { thrift_reserved_keyword(yytext); }
"typedef"            { thrift_reserved_keyword(yytext); }
"union"              { thrift_reserved_keyword(yytext); }
"unsigned"           { thrift_reserved_keyword(yytext); }
"var"                { thrift_reserved_keyword(yytext); }
"virtual"            { thrift_reserved_keyword(yytext); }
"void"               { thrift_reserved_keyword(yytext); }
"volatile"           { thrift_reserved_keyword(yytext); }
"while"              { thrift_reserved_keyword(yytext); }
"with"               { thrift_reserved_keyword(yytext); }
"union"              { thrift_reserved_keyword(yytext); }
"yield"              { thrift_reserved_keyword(yytext); }
"Object"             { thrift_reserved_keyword(yytext); }
"Client"             { thrift_reserved_keyword(yytext); }
"IFace"              { thrift_reserved_keyword(yytext); }
"Processor"          { thrift_reserved_keyword(yytext); }

{octconstant} {
  errno = 0;
  yylval.iconst = strtoll(yytext+1, NULL, 8);
  if (errno == ERANGE) {
    integer_overflow(yytext);
  }
  return tok_int_constant;
}

{intconstant} {
  errno = 0;
  yylval.iconst = strtoll(yytext, NULL, 10);
  if (errno == ERANGE) {
    integer_overflow(yytext);
  }
  return tok_int_constant;
}

{hexconstant} {
  errno = 0;
  yylval.iconst = strtoll(yytext+2, NULL, 16);
  if (errno == ERANGE) {
    integer_overflow(yytext);
  }
  return tok_int_constant;
}

{dubconstant} {
  yylval.dconst = atof(yytext);
  return tok_dub_constant;
}

{identifier} {
  yylval.id = strdup(yytext);
  return tok_identifier;
}

{st_identifier} {
  yylval.id = strdup(yytext);
  return tok_st_identifier;
}

{dliteral} {
  yylval.id = strdup(yytext+1);
  yylval.id[strlen(yylval.id)-1] = '\0';
  return tok_literal;
}

{sliteral} {
  yylval.id = strdup(yytext+1);
  yylval.id[strlen(yylval.id)-1] = '\0';
  return tok_literal;
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


%%

/* vim: filetype=lex
*/
