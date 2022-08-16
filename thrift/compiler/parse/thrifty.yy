%{
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

#define __STDC_LIMIT_MACROS
#define __STDC_FORMAT_MACROS

#include <string>
#include <boost/optional.hpp>

/**
 * Note macro expansion because this is different between OSS and internal
 * build, sigh.
 */
#include THRIFTY_HH

namespace apache {
namespace thrift {
namespace compiler {
namespace {

yy::parser::symbol_type parse_lex() { return {}; }
#define yylex apache::thrift::compiler::parse_lex

} // namespace
} // namespace compiler
} // namespace thrift
} // namespace apache

%}

%code requires
{
#include <thrift/compiler/ast/t_program.h>

using YYSTYPE = int;

namespace apache {
namespace thrift {
namespace compiler {

using t_docstring = boost::optional<std::string>;

} // namespace compiler
} // namespace thrift
} // namespace apache
}

%defines
%locations
%define api.location.type {apache::thrift::compiler::source_range}
%define api.token.constructor
%define api.value.type variant
%define api.namespace {apache::thrift::compiler::yy}

/**
 * Strings identifier
 */
%token<std::string>     tok_identifier
%token<std::string>     tok_literal
%token<t_docstring>     tok_doctext
%token<t_docstring>     tok_inline_doc

/**
 * Constant values
 */
%token<bool>      tok_bool_constant
%token<uint64_t>  tok_int_constant
%token<double>    tok_dub_constant

/**
 * Characters
 */
%token tok_char_comma               ","
%token tok_char_semicolon           ";"
%token tok_char_bracket_curly_l     "{"
%token tok_char_bracket_curly_r     "}"
%token tok_char_equal               "="
%token tok_char_bracket_square_l    "["
%token tok_char_bracket_square_r    "]"
%token tok_char_colon               ":"
%token tok_char_bracket_round_l     "("
%token tok_char_bracket_round_r     ")"
%token tok_char_bracket_angle_l     "<"
%token tok_char_bracket_angle_r     ">"
%token tok_char_at_sign             "@"
%token tok_char_plus                "+"
%token tok_char_minus               "-"

/**
 * Header keywords
 */
%token tok_include
%token tok_cpp_include
%token tok_hs_include
%token tok_package
%token tok_namespace

/**
 * Base datatype keywords
 */
%token tok_void
%token tok_bool
%token tok_byte
%token tok_string
%token tok_binary
%token tok_i16
%token tok_i32
%token tok_i64
%token tok_double
%token tok_float

/**
 * Complex type keywords
 */
%token tok_map
%token tok_list
%token tok_set
%token tok_stream
%token tok_sink

/**
 * Function qualifiers
 */
%token tok_oneway
%token tok_idempotent
%token tok_readonly

/**
 * Exception qualifiers
 */
%token tok_safe
%token tok_transient
%token tok_stateful
%token tok_permanent
%token tok_server
%token tok_client

/**
 * Thrift language keywords
 */
%token tok_typedef
%token tok_struct
%token tok_exception
%token tok_throws
%token tok_extends
%token tok_service
%token tok_enum
%token tok_const
%token tok_required
%token tok_optional
%token tok_union
%token tok_interaction
%token tok_performs

%token tok_eof 0
%token tok_error

%%

Program: /* empty */

%%

void apache::thrift::compiler::yy::parser::error(
  const source_range&, const std::string&) {}
