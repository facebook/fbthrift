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

#include <string>

#include "thrift/compiler/parse/parsing_driver.h"

/**
 * Note macro expansion because this is different between OSS and internal
 * build, sigh.
 */
#include THRIFTY_HH

YY_DECL;

using yyparser = ::apache::thrift::compiler::yy::parser;

namespace {

std::string trim_quotes(const char* text) {
  std::string val{text + 1};
  val.resize(val.length() - 1);
  return val;
}

#define YY_USER_ACTION driver.compute_location(*yylloc, *yylval, yytext);

} // namespace

%}

/**
 * Helper definitions, comments, constants, and whatnot
 */

octconstant   ("0"[0-7]*)
decconstant   ([1-9][0-9]*)
hexconstant   ("0"[xX][0-9A-Fa-f]+)
binconstant   ("0"[bB][01]+)
dubconstant   ([0-9]*(\.[0-9]+)?([eE][+-]?[0-9]+)?)
identifier    ([a-zA-Z_][\.a-zA-Z_0-9]*)
whitespace    ([ \t\r\n]*)
sillycomm     ("/*""*"*"*/")
multicomm     ("/*"[^*]"/"*([^*/]|[^*]"/"|"*"[^/])*"*"*"*/")
doctext       (("/**"([^*/]|[^*]"/"|"*"[^/])*"*"*"*/")|("///"(\n|[^/\n][^\n]*){whitespace})+)
comment       ("//"[^\n]*)
unixcomment   ("#"[^\n]*)
dliteral      ("\""[^"]*"\"")
sliteral      ("'"[^']*"'")

%%

{whitespace}         { /* do nothing */ }
{sillycomm}          { /* do nothing */ }
{multicomm}          { /* do nothing */ }
{comment}            { /* do nothing */ }
{unixcomment}        { /* do nothing */ }

"{"                  { return yyparser::make_tok_char_bracket_curly_l(*yylloc); }
"}"                  { return yyparser::make_tok_char_bracket_curly_r(*yylloc); }
","                  { return yyparser::make_tok_char_comma(*yylloc); }
";"                  { return yyparser::make_tok_char_semicolon(*yylloc); }
"="                  { return yyparser::make_tok_char_equal(*yylloc); }
"["                  { return yyparser::make_tok_char_bracket_square_l(*yylloc); }
"]"                  { return yyparser::make_tok_char_bracket_square_r(*yylloc); }
":"                  { return yyparser::make_tok_char_colon(*yylloc); }
"("                  { return yyparser::make_tok_char_bracket_round_l(*yylloc); }
")"                  { return yyparser::make_tok_char_bracket_round_r(*yylloc); }
"<"                  { return yyparser::make_tok_char_bracket_angle_l(*yylloc); }
">"                  { return yyparser::make_tok_char_bracket_angle_r(*yylloc); }
"@"                  { return yyparser::make_tok_char_at_sign(*yylloc); }
"-"                  { return yyparser::make_tok_char_minus(*yylloc); }
"+"                  { return yyparser::make_tok_char_plus(*yylloc); }

"false"              { return yyparser::make_tok_bool_constant(false, *yylloc); }
"true"               { return yyparser::make_tok_bool_constant(true, *yylloc); }

"namespace"          { return yyparser::make_tok_namespace(*yylloc); }
"cpp_include"        { return yyparser::make_tok_cpp_include(*yylloc); }
"hs_include"         { return yyparser::make_tok_hs_include(*yylloc); }
"include"            { return yyparser::make_tok_include(*yylloc); }
"void"               { return yyparser::make_tok_void(*yylloc); }
"bool"               { return yyparser::make_tok_bool(*yylloc); }
"byte"               { return yyparser::make_tok_byte(*yylloc); }
"i16"                { return yyparser::make_tok_i16(*yylloc); }
"i32"                { return yyparser::make_tok_i32(*yylloc); }
"i64"                { return yyparser::make_tok_i64(*yylloc); }
"double"             { return yyparser::make_tok_double(*yylloc); }
"float"              { return yyparser::make_tok_float(*yylloc); }
"string"             { return yyparser::make_tok_string(*yylloc); }
"binary"             { return yyparser::make_tok_binary(*yylloc); }
"map"                { return yyparser::make_tok_map(*yylloc); }
"list"               { return yyparser::make_tok_list(*yylloc); }
"set"                { return yyparser::make_tok_set(*yylloc); }
"sink"               { return yyparser::make_tok_sink(*yylloc); }
"stream"             { return yyparser::make_tok_stream(*yylloc); }
"interaction"        { return yyparser::make_tok_interaction(*yylloc); }
"performs"           { return yyparser::make_tok_performs(*yylloc); }
"oneway"             { return yyparser::make_tok_oneway(*yylloc); }
"idempotent"         { return yyparser::make_tok_idempotent(*yylloc); }
"readonly"           { return yyparser::make_tok_readonly(*yylloc); }
"safe"               { return yyparser::make_tok_safe(*yylloc); }
"transient"          { return yyparser::make_tok_transient(*yylloc); }
"stateful"           { return yyparser::make_tok_stateful(*yylloc); }
"permanent"          { return yyparser::make_tok_permanent(*yylloc); }
"server"             { return yyparser::make_tok_server(*yylloc); }
"client"             { return yyparser::make_tok_client(*yylloc); }
"typedef"            { return yyparser::make_tok_typedef(*yylloc); }
"struct"             { return yyparser::make_tok_struct(*yylloc); }
"union"              { return yyparser::make_tok_union(*yylloc); }
"exception"          { return yyparser::make_tok_exception(*yylloc); }
"extends"            { return yyparser::make_tok_extends(*yylloc); }
"throws"             { return yyparser::make_tok_throws(*yylloc); }
"service"            { return yyparser::make_tok_service(*yylloc); }
"enum"               { return yyparser::make_tok_enum(*yylloc); }
"const"              { return yyparser::make_tok_const(*yylloc); }
"required"           { return yyparser::make_tok_required(*yylloc); }
"optional"           { return yyparser::make_tok_optional(*yylloc); }

{octconstant}        { return yyparser::make_tok_int_constant(driver.parse_integer(yytext, 1, 8), *yylloc); }
{decconstant}        { return yyparser::make_tok_int_constant(driver.parse_integer(yytext, 0, 10), *yylloc); }
{hexconstant}        { return yyparser::make_tok_int_constant(driver.parse_integer(yytext, 2, 16), *yylloc); }
{binconstant}        { return yyparser::make_tok_int_constant(driver.parse_integer(yytext, 2, 2), *yylloc); }
{dubconstant}        { return yyparser::make_tok_dub_constant(driver.parse_double(yytext), *yylloc); }
{dliteral}           { return yyparser::make_tok_literal(trim_quotes(yytext), *yylloc); }
{sliteral}           { return yyparser::make_tok_literal(trim_quotes(yytext), *yylloc); }

{identifier}         { return yyparser::make_tok_identifier(std::string{yytext}, *yylloc); }

{doctext}            { driver.parse_doctext(yytext, yylineno); }

.                    { driver.unexpected_token(yytext); }
<<EOF>>              { return yyparser::make_tok_eof(*yylloc); }

%%

/* vim: filetype=lex
*/
