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

/**
 * Thrift parser.
 *
 * This parser is used on a thrift definition file.
 *
 */

#define __STDC_LIMIT_MACROS
#define __STDC_FORMAT_MACROS

#include <cassert>
#include <stdio.h>
#include <inttypes.h>
#include <limits.h>
#include <stack>
#include <utility>

#include <boost/optional.hpp>

#include <thrift/compiler/ast/t_scope.h>
#include <thrift/compiler/ast/t_union.h>
#include <thrift/compiler/parse/parsing_driver.h>
#include <thrift/compiler/ast/t_container.h>

/**
 * Note macro expansion because this is different between OSS and internal
 * build, sigh.
 */
#include THRIFTY_HH

/**
 * Declare fbthrift_compiler_parse_lex() so we can use it.
 */
YY_DECL;
#define yylex fbthrift_compiler_parse_lex

namespace apache {
namespace thrift {
namespace compiler {
namespace {

// Assume ownership of a pointer.
template <typename T>
std::unique_ptr<T> own(T* ptr) {
  return std::unique_ptr<T>(ptr);
}

} // namespace
} // namespace compiler
} // namespace thrift
} // namespace apache

%}

%code requires
{

#include <thrift/compiler/ast/t_enum_value.h>
#include <thrift/compiler/ast/t_exception.h>
#include <thrift/compiler/ast/t_function.h>
#include <thrift/compiler/parse/t_ref.h>

namespace apache { namespace thrift { namespace compiler { namespace yy {
class location;
}}}}

/**
 * YYLTYPE evaluates to a class named 'location', which has:
 *   position begin;
 *   position end;
 * The class 'position', in turn, has:
 *   std::string* filename;
 *   unsigned line;
 *   unsigned column;
 */
using YYLTYPE = apache::thrift::compiler::yy::location;
using YYSTYPE = int;

namespace apache {
namespace thrift {
namespace compiler {

struct t_annotations;
struct t_def_attrs;

class t_base_type;
class t_container;
class t_container_type;
class t_interaction;
class t_service;
class t_sink;
class t_stream_response;
class t_throws;
class t_typedef;
class t_union;

class parsing_driver;

using t_docstring = boost::optional<std::string>;
using t_struct_annotations = node_list<t_const>;
using t_typethrowspair = std::pair<t_type_ref, t_throws*>;

} // namespace compiler
} // namespace thrift
} // namespace apache

}

%defines
%locations
%define api.token.constructor
%define api.value.type variant
%define api.namespace {apache::thrift::compiler::yy}

%param
    {apache::thrift::compiler::parsing_driver& driver}
    {void* raw_scanner}
    {YYSTYPE * yylval_param}
    {YYLTYPE * yylloc_param}

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

/**
 * Grammar nodes
 *
 * Memory management rules:
 * - mutable pointers in the process of being build, and
 * need to be owned when complete.
 * - t_refs are already owned.
 */

%type<t_ref<t_base_type>>          BaseType
%type<t_container*>                ContainerType
%type<t_container*>                MapType
%type<t_container*>                SetType
%type<t_container*>                ListType

%type<std::string>                 Identifier
%type<std::string>                 FieldTypeIdentifier
%type<t_def_attrs*>                StatementAttrs
%type<t_named*>                    Header
%type<t_named*>                    Definition
%type<t_named*>                    Statement
%type<t_named*>                    StatementAnnotated

%type<t_typedef*>                  Typedef

%type<t_annotation*>               Annotation
%type<t_annotations*>              Annotations
%type<t_annotations*>              AnnotationList

%type<t_const*>                    StructuredAnnotation
%type<t_struct_annotations*>       StructuredAnnotations
%type<t_struct_annotations*>       NonEmptyStructuredAnnotationList

%type<t_field*>                    Field
%type<t_field*>                    FieldAnnotated
%type<boost::optional<t_field_id>> FieldId
%type<t_field_qualifier>           FieldQualifier
%type<t_type_ref>                  FieldType
%type<t_stream_response*>          StreamReturnType
%type<t_sink*>                     SinkReturnType
%type<t_typethrowspair>            SinkFieldType
%type<t_const_value*>              FieldValue
%type<t_field_list*>               FieldList

%type<int64_t>                     Integer
%type<double>                      Double

%type<t_enum*>                     Enum
%type<t_enum_value_list*>          EnumValueList
%type<t_enum_value*>               EnumValue
%type<t_enum_value*>               EnumValueAnnotated

%type<t_const*>                    Const
%type<t_const_value*>              ConstValue
%type<t_const_value*>              ConstList
%type<t_const_value*>              ConstListContents
%type<t_const_value*>              ConstMap
%type<t_const_value*>              ConstMapContents
%type<t_const_value*>              ConstStruct
%type<t_type_ref>                  ConstStructType
%type<t_const_value*>              ConstStructContents

%type<t_struct*>                   Struct
%type<t_union*>                    Union

%type<t_error_kind>                ErrorKind
%type<t_error_blame>               ErrorBlame
%type<t_error_safety>              ErrorSafety
%type<t_exception*>                Exception

%type<t_service*>                  Service
%type<t_interaction*>              Interaction

%type<t_function*>                 Function
%type<t_function*>                 FunctionAnnotated
%type<t_function*>                 Performs
%type<std::pair<t_type_ref, boost::optional<t_type_ref>>> FunctionType
%type<t_function_list*>            FunctionList

%type<t_throws*>                   MaybeThrows
%type<t_ref<t_service>>            Extends
%type<t_function_qualifier>        FunctionQualifier

%type<t_docstring>                 CaptureDocText
%type<t_docstring>                 InlineDocOptional
%type<std::string>                 IntOrLiteral

%type<bool>                        CommaOrSemicolonOptional

%%

/**
 * Thrift Grammar Implementation.
 *
 * For the most part this source file works its way top down from what you
 * might expect to find in a typical .thrift file, i.e. type definitions and
 * namespaces up top followed by service definitions using those types.
 */

Program:
  StatementList
    {
      driver.debug("Program -> StatementList");
      driver.clear_doctext();
    }

/**
 * This rule is to avoid keywords as exceptions in field type identifiers.
 * This allows to avoid shift-reduce conflicts due to a previous ambuguity
 * in the resolution of the FunctionQualifier FunctionType part of the
 * Function rule, when one of the tok_oneway, tok_idempotent or tok_readonly
 * is encountered. It could either resolve the token as FunctionQualifier or
 * resolve "" as FunctionQualifier and resolve the token as FunctionType.
 */
FieldTypeIdentifier:
  tok_identifier { $$ = std::move($1); }

Identifier:
  FieldTypeIdentifier { $$ = std::move($1); }
/* context sensitive keywords that should be allowed in identifiers. */
| tok_package    { $$ = "package"; }
| tok_sink       { $$ = "sink"; }
| tok_oneway     { $$ = "oneway"; }
| tok_readonly   { $$ = "readonly"; }
| tok_idempotent { $$ = "idempotent"; }
| tok_safe       { $$ = "safe"; }
| tok_transient  { $$ = "transient"; }
| tok_stateful   { $$ = "stateful"; }
| tok_permanent  { $$ = "permanent"; }
| tok_server     { $$ = "server"; }
| tok_client     { $$ = "client"; }

CommaOrSemicolon:
  "," {}
| ";" {}

CommaOrSemicolonOptional:
  CommaOrSemicolon { $$ = true; }
|                  { $$ = false; }

CaptureDocText: { $$ = driver.pop_doctext(); }

ProgramDocText: {
    /* when there is any doctext, assign it to the top level `program` */
    driver.set_doctext(*driver.program, driver.pop_doctext());
}

InlineDocOptional:
  tok_inline_doc
    {
      driver.debug("Inline doc");
      $$ = std::move($1);
    }
|
  { $$ = boost::none; }

Header:
  tok_include tok_literal
    {
      driver.debug("Header -> tok_include tok_literal");
      driver.add_include(std::move($2));
      $$ = nullptr;
    }
| tok_package tok_literal
    {
      driver.debug("Header -> tok_package tok_literal");
      driver.set_package(std::move($2));
      $$ = nullptr;
    }
| tok_namespace Identifier Identifier
    {
      driver.debug("Header -> tok_namespace Identifier Identifier");
      if (driver.mode == parsing_mode::PROGRAM) {
        driver.program->set_namespace(std::move($2), std::move($3));
      }
      $$ = nullptr;
    }
| tok_namespace Identifier tok_literal
    {
      driver.debug("Header -> tok_namespace Identifier tok_literal");
      if (driver.mode == parsing_mode::PROGRAM) {
        driver.program->set_namespace(std::move($2), std::move($3));
      }
      $$ = nullptr;
    }
| tok_cpp_include tok_literal
    {
      driver.debug("Header -> tok_cpp_include tok_literal");
      if (driver.mode == parsing_mode::PROGRAM) {
        driver.program->add_cpp_include($2);
      }
      $$ = nullptr;
    }
| tok_hs_include tok_literal
    {
      driver.debug("Header -> tok_hs_include tok_literal");
      // Do nothing. This syntax is handled by the hs compiler
      $$ = nullptr;
    }

Definition:
  Typedef     { $$ = $1; }
| Enum        { $$ = $1; }
| Const       { $$ = $1; }
| Struct      { $$ = $1; }
| Union       { $$ = $1; }
| Exception   { $$ = $1; }
| Service     { $$ = $1; }
| Interaction { $$ = $1; }

Statement:
  ProgramDocText Header
    {
      driver.debug("Statement -> ProgramDocText Header");
      driver.validate_header_location();
      $$ = $2;
    }
| Definition
    {
      driver.debug("Statement -> Definition");
      driver.set_parsed_definition();
      $$ = $1;
    }

StatementAnnotated:
  StatementAttrs Statement Annotations
    {
      driver.debug("StatementAnnotated -> StatementAttrs Statement Annotations");
      if ($2 == nullptr) {
        driver.validate_annotations_on_null_statement($1, $3);
        $$ = nullptr;
      } else {
        driver.avoid_tokens_loc(@$, {{$1 == nullptr, @2}}, {{$3 == nullptr, @2}});
        $$ = $2;
        driver.set_attributes(*$$, own($1), own($3), @$);
      }
    }

StatementAttrs:
  CaptureDocText StructuredAnnotations
    {
      driver.debug("StatementAttrs -> CaptureDocText StructuredAnnotations");
      $$ = nullptr;
      if ($1 || $2 != nullptr) {
        driver.avoid_tokens_loc(@$, {{!$1, @2}}, {});
        $$ = new t_def_attrs{std::move($1), own($2)};
      }
    }

StatementList:
  StatementList StatementAnnotated CommaOrSemicolonOptional
    {
      driver.debug("StatementList -> StatementList StatementAnnotated "
        "CommaOrSemicolonOptional");
      if ($2 != nullptr) {
        driver.add_def(own($2));
      }
    }
|   {
      driver.debug("StatementList -> ");
    }

Integer:
  tok_int_constant
    {
      driver.debug("Integer -> tok_int_constant");
      $$ = driver.to_int($1);
    }
| tok_char_plus tok_int_constant
    {
      driver.debug("Integer -> + tok_int_constant");
      $$ = driver.to_int($2);
    }
| tok_char_minus tok_int_constant
    {
      driver.debug("Integer -> - tok_int_constant");
      $$ = driver.to_int($2, true);
    }

Double:
  tok_dub_constant
    {
      driver.debug("Double -> tok_dub_constant");
      $$ = $1;
    }
| tok_char_plus tok_dub_constant
    {
      driver.debug("Double -> + tok_dub_constant");
      $$ = $2;
    }
| tok_char_minus tok_dub_constant
    {
      driver.debug("Double -> - tok_dub_constant");
      $$ = -$2;
    }

Typedef:
  tok_typedef FieldType Identifier
    {
      driver.debug("TypeDef => tok_typedef FieldType Identifier");
      $$ = new t_typedef(driver.program, std::move($3), std::move($2));
      $$->set_lineno(@3.end.line);
    }

Enum:
  tok_enum Identifier "{" EnumValueList "}"
    {
      driver.debug("Enum => tok_enum Identifier { EnumValueList }");
      $$ = new t_enum(driver.program, std::move($2));
      $$->set_values(std::move(*own($4)));
      $$->set_lineno(@2.end.line);
    }

EnumValueList:
  EnumValueList EnumValueAnnotated CommaOrSemicolonOptional InlineDocOptional
    {
      driver.debug("EnumValueList EnumValueAnnotated CommaOrSemicolonOptional");
      $$ = $1;
      if ($4 != boost::none) {
        driver.set_doctext(*$2, $4);
      }
      $$->emplace_back(own($2));
    }
|
    {
      driver.debug("EnumValueList -> ");
      $$ = new t_enum_value_list;
    }

EnumValueAnnotated:
  StatementAttrs EnumValue Annotations
    {
      driver.debug("StatementAttrs EnumValue Annotations");
      driver.avoid_tokens_loc(@$, {{$1 == nullptr, @2}}, {{$3 == nullptr, @2}});
      $$ = $2;
      driver.set_attributes(*$$, own($1), own($3), @$);
    }

EnumValue:
  Identifier "=" Integer
    {
      driver.debug("EnumValue -> Identifier = Integer");
      $$ = new t_enum_value(std::move($1));
      $$->set_value(driver.to_enum_value($3));
      $$->set_lineno(@1.end.line);
    }
| Identifier
    {
      driver.debug("EnumValue -> Identifier");
      $$ = new t_enum_value(std::move($1));
      $$->set_lineno(@1.end.line);
    }

Const:
  tok_const FieldType Identifier "=" ConstValue
    {
      driver.debug("Const => tok_const FieldType Identifier = ConstValue");
      $$ = new t_const(driver.program, std::move($2), std::move($3), own($5));
      $$->set_lineno(@3.end.line);
    }

ConstValue:
  tok_bool_constant
    {
      driver.debug("ConstValue => tok_bool_constant");
      $$ = new t_const_value();
      $$->set_bool($1);
    }
| Integer
    {
      driver.debug("ConstValue => Integer");
      $$ = driver.to_const_value($1).release();
    }
| Double
    {
      driver.debug("ConstValue => Double");
      $$ = new t_const_value();
      $$->set_double($1);
    }
| tok_literal
    {
      driver.debug("ConstValue => tok_literal");
      $$ = new t_const_value($1);
    }
| Identifier
    {
      driver.debug("ConstValue => Identifier");
      $$ = driver.copy_const_value($1).release();
    }
| ConstList
    {
      driver.debug("ConstValue => ConstList");
      $$ = $1;
    }
| ConstMap
    {
      driver.debug("ConstValue => ConstMap");
      $$ = $1;
    }
| ConstStruct
    {
      driver.debug("ConstValue => ConstStruct");
      $$ = $1;
    }

ConstList:
  "[" ConstListContents CommaOrSemicolonOptional "]"
    {
      driver.debug("ConstList => [ ConstListContents CommaOrSemicolonOptional ]");
      $$ = $2;
    }
| "[" "]"
    {
      driver.debug("ConstList => [ ]");
      $$ = new t_const_value();
      $$->set_list();
    }

ConstListContents:
  ConstListContents CommaOrSemicolon ConstValue
    {
      driver.debug("ConstListContents => ConstListContents CommaOrSemicolon ConstValue");
      $$ = $1;
      $$->add_list(own($3));
    }
| ConstValue
    {
      driver.debug("ConstListContents => ConstValue");
      $$ = new t_const_value();
      $$->set_list();
      $$->add_list(own($1));
    }

ConstMap:
  "{" ConstMapContents CommaOrSemicolonOptional "}"
    {
      driver.debug("ConstMap => { ConstMapContents CommaOrSemicolonOptional }");
      $$ = $2;
    }
| "{" "}"
    {
      driver.debug("ConstMap => { }");
      $$ = new t_const_value();
      $$->set_map();
    }

ConstMapContents:
  ConstMapContents CommaOrSemicolon ConstValue ":" ConstValue
    {
      driver.debug("ConstMapContents => ConstMapContents CommaOrSemicolon ConstValue : ConstValue");
      $$ = $1;
      $$->add_map(own($3), own($5));
    }
| ConstValue ":" ConstValue
    {
      driver.debug("ConstMapContents => ConstValue : ConstValue");
      $$ = new t_const_value();
      $$->set_map();
      $$->add_map(own($1), own($3));
    }

ConstStruct:
  ConstStructType "{" ConstStructContents CommaOrSemicolonOptional "}"
    {
      driver.debug("ConstStruct => ConstStructType { ConstStructContents CommaOrSemicolonOptional }");
      $$ = $3;
      $$->set_ttype($1);
    }
| ConstStructType "{" "}"
    {
      driver.debug("ConstStruct => ConstStructType { }");
      $$ = new t_const_value();
      $$->set_map();
      $$->set_ttype($1);
    }

ConstStructType:
  Identifier
    {
      driver.debug("ConstStructType -> Identifier");
      $$ = driver.new_type_ref(std::move($1), nullptr, /*is_const=*/true);
    }

ConstStructContents:
  ConstStructContents CommaOrSemicolon Identifier "=" ConstValue
    {
      driver.debug("ConstStructContents => ConstStructContents CommaOrSemicolon Identifier = ConstValue");
      $$ = $1;
      $$->add_map(std::make_unique<t_const_value>($3), own($5));
    }
| Identifier "=" ConstValue
    {
      driver.debug("ConstStructContents => Identifier = ConstValue");
      $$ = new t_const_value();
      $$->set_map();
      $$->add_map(std::make_unique<t_const_value>($1), own($3));
    }

Struct:
  tok_struct Identifier "{" FieldList "}"
    {
      driver.debug("Struct => tok_struct Identifier { FieldList }");
      $$ = new t_struct(driver.program, std::move($2));
      driver.set_fields(*$$, std::move(*own($4)));
      $$->set_lineno(@2.end.line);
    }

Union:
  tok_union Identifier "{" FieldList "}"
    {
      driver.debug("Union => tok_union Identifier { FieldList }");
      $$ = new t_union(driver.program, std::move($2));
      driver.set_fields(*$$, std::move(*own($4)));
      $$->set_lineno(@2.end.line);
    }

Exception:
  // TODO(afuller): Either make the qualifiers order agnostic or produce a better error message.
  ErrorSafety ErrorKind ErrorBlame tok_exception Identifier "{" FieldList "}"
    {
      driver.debug("Exception => ErrorSafety ErrorKind ErrorBlame tok_exception "
        "Identifier { FieldList }");
      // TODO(afuller): Correct resulting source_loc.begin when safety qualifier is omitted.
      $$ = new t_exception(driver.program, std::move($5));
      $$->set_safety($1);
      $$->set_kind($2);
      $$->set_blame($3);
      driver.set_fields(*$$, std::move(*own($7)));
      $$->set_lineno(@5.end.line);
    }

ErrorSafety:
  tok_safe { $$ = t_error_safety::safe;}
|          { $$ = {}; }

ErrorKind:
  tok_transient { $$ = t_error_kind::transient; }
| tok_stateful  { $$ = t_error_kind::stateful; }
| tok_permanent { $$ = t_error_kind::permanent; }
|               { $$ = {}; }

ErrorBlame:
  tok_client { $$ = t_error_blame::client; }
| tok_server { $$ = t_error_blame::server; }
|            { $$ = {}; }

Service:
  tok_service Identifier Extends "{" FunctionList "}"
    {
      driver.debug("Service => tok_service Identifier Extends { FunctionList }");
      $$ = new t_service(driver.program, std::move($2), $3);
      driver.set_functions(*$$, own($5));
      $$->set_lineno(@2.end.line);
    }

Extends:
  tok_extends Identifier
    {
      driver.debug("Extends -> tok_extends Identifier");
      $$ = driver.find_service($2);
    }
|   { $$ = nullptr; }

Interaction:
  tok_interaction Identifier "{" FunctionList "}"
    {
      driver.debug("Interaction -> tok_interaction Identifier { FunctionList }");
      $$ = new t_interaction(driver.program, std::move($2));
      driver.set_functions(*$$, own($4));
      $$->set_lineno(@2.end.line);
    }

FunctionList:
  FunctionList FunctionAnnotated CommaOrSemicolonOptional
    {
      driver.debug("FunctionList -> FunctionList FunctionAnnotated CommaOrSemicolonOptional");
      $1->emplace_back(own($2));
      $$ = $1;
    }
| FunctionList Performs CommaOrSemicolon
    {
      driver.debug("FunctionList -> FunctionList Performs CommaOrSemicolon");
      $1->emplace_back(own($2));
      $$ = $1;
    }
|
    {
      driver.debug("FunctionList -> ");
      $$ = new t_function_list;
    }

Function:
  FunctionQualifier FunctionType Identifier "(" FieldList ")" MaybeThrows
    {
      driver.debug("Function => FunctionQualifier FunctionType Identifier ( FieldList ) MaybeThrows");
      driver.avoid_tokens_loc(
        @$,
        {{$1 == t_function_qualifier::unspecified, @2}},
        {{$7 == nullptr, @6}});
      $$ = new t_function(driver.program, std::move($2.first), std::move($3));
      if ($2.second) {
        $$->set_returned_interaction(std::move(*$2.second));
      }
      $$->set_qualifier($1);
      driver.set_fields($$->params(), std::move(*own($5)));
      $$->set_exceptions(own($7));
      $$->set_lineno(@3.end.line);
      // TODO(afuller): Leave the param list unnamed.
      $$->params().set_name($$->name() + "_args");
    }

FunctionAnnotated:
  StatementAttrs Function Annotations
    {
      driver.debug("FunctionAnnotated => StatementAttrs Function Annotations");
      driver.avoid_tokens_loc(@$, {{$1 == nullptr, @2}}, {{$3 == nullptr, @2}});
      $$ = $2;
      driver.set_attributes(*$$, own($1), own($3), @$);
    }

Performs:
  tok_performs FieldType
    {
      driver.debug("Performs => tok_performs FieldType");
      auto& ret = $2;
      std::string name = ret.get_type()
          ? "create" + ret.get_type()->get_name()
          : "<interaction placeholder>";
      $$ = new t_function(
        std::move(ret),
        std::move(name),
        std::make_unique<t_paramlist>(driver.program)
      );
      $$->set_lineno(driver.scanner->get_lineno());
      $$->set_is_interaction_constructor();
    }

FunctionQualifier:
  tok_oneway     { $$ = t_function_qualifier::one_way; }
| tok_idempotent { $$ = t_function_qualifier::idempotent; }
| tok_readonly   { $$ = t_function_qualifier::read_only; }
|                { $$ = {}; }

MaybeThrows:
  tok_throws "(" FieldList ")"
    {
      driver.debug("MaybeThrows -> tok_throws ( FieldList )");
      $$ = driver.new_throws(own($3)).release();
    }
|   { $$ = nullptr; }

FieldList:
  FieldList FieldAnnotated CommaOrSemicolonOptional InlineDocOptional
    {
      driver.debug("FieldList -> FieldAnnotated CommaOrSemicolonOptional");
      $$ = $1;
      if ($4 != boost::none) {
        driver.set_doctext(*$2, $4);
      }
      $$->emplace_back($2);
    }
|
    {
      driver.debug("FieldList -> ");
      $$ = new t_field_list;
    }

Field:
  FieldId FieldQualifier FieldType Identifier FieldValue
    {
      driver.debug("Field => FieldId FieldQualifier FieldType Identifier FieldValue");
      driver.avoid_tokens_loc(@$, {}, {{$5 == nullptr, @4}});
      $$ = new t_field(std::move($3), std::move($4), $1.value_or(0), $1 != boost::none);
      $$->set_qualifier($2);
      $$->set_default_value(own($5));
      $$->set_lineno(@4.end.line);
    }

FieldAnnotated:
  StatementAttrs Field Annotations
    {
      driver.debug("FieldAnnotated => StatementAttrs Field Annotations");
      driver.avoid_tokens_loc(@$, {{$1 == nullptr, @2}}, {{$3 == nullptr, @2}});
      $$ = $2;
      driver.set_attributes(*$$, own($1), own($3), @$);
    }

FieldId:
  Integer ":" { $$ = driver.to_field_id($1); }
|             { $$ = boost::none; }

FieldQualifier:
  tok_required { $$ = t_field_qualifier::required; }
| tok_optional { $$ = t_field_qualifier::optional; }
|              { $$ = {}; }

FieldValue:
  "=" ConstValue
    {
      if (driver.mode == parsing_mode::PROGRAM) {
        $$ = $2;
      } else {
        delete $2;
        $$ = nullptr;
      }
    }
|   { $$ = nullptr; }

FunctionType:
  StreamReturnType
    {
      driver.debug("FunctionType -> StreamReturnType");
      $$ = {driver.new_type_ref(own($1), nullptr), boost::none};
    }
| FieldType "," StreamReturnType
    {
      driver.debug("FunctionType -> FieldType, StreamReturnType");
      // This might actually be an interaction, corrected in standard mutator.
      $3->set_first_response_type(std::move($1));
      $$ = {driver.new_type_ref(own($3), nullptr), boost::none};
    }
|  FieldType "," FieldType "," StreamReturnType
    {
      driver.debug("FunctionType -> FieldType, FieldType, StreamReturnType");
      $5->set_first_response_type(std::move($3));
      $$ = {driver.new_type_ref(own($5), nullptr), $1};
    }
| SinkReturnType
    {
      driver.debug("FunctionType -> SinkReturnType");
      $$ = {driver.new_type_ref(own($1), nullptr), boost::none};
    }
| FieldType "," SinkReturnType
    {
      driver.debug("FunctionType -> FieldType, SinkReturnType");
      // This might actually be an interaction, corrected in standard mutator.
      $3->set_first_response_type(std::move($1));
      $$ = {driver.new_type_ref(own($3), nullptr), boost::none};
    }
|  FieldType "," FieldType "," SinkReturnType
    {
      driver.debug("FunctionType -> FieldType, FieldType, SinkReturnType");
      $5->set_first_response_type(std::move($3));
      $$ = {driver.new_type_ref(own($5), nullptr), $1};
    }
| FieldType
    {
      driver.debug("FunctionType -> FieldType");
      // This might actually be an interaction, corrected in standard mutator.
      $$ = {$1, boost::none};
    }
|  FieldType "," FieldType
    {
      driver.debug("FunctionType -> FieldType, FieldType");
      $$ = {$3, $1};
    }
| tok_void
    {
      driver.debug("FunctionType -> tok_void");
      $$ = {t_base_type::t_void(), boost::none};
    }

StreamReturnType:
  tok_stream "<" FieldType MaybeThrows ">"
  {
    driver.debug("StreamReturnType -> tok_stream < FieldType MaybeThrows >");
    auto stream_res = std::make_unique<t_stream_response>(std::move($3));
    stream_res->set_exceptions(own($4));
    $$ = stream_res.release();
  }

SinkReturnType:
  tok_sink "<" SinkFieldType "," SinkFieldType ">"
    {
      driver.debug("SinkReturnType -> tok_sink<FieldType, FieldType>");
      auto sink = std::make_unique<t_sink>(std::move($3.first), std::move($5.first));
      sink->set_sink_exceptions(own($3.second));
      sink->set_final_response_exceptions(own($5.second));
      $$ = sink.release();
    }

SinkFieldType:
  FieldType MaybeThrows
    {
      driver.debug("SinkFieldType => FieldType MaybeThrows");
      $$ = std::make_pair($1, $2);
    }

FieldType:
  FieldTypeIdentifier Annotations
    {
      driver.debug("FieldType => Identifier Annotations");
      $$ = driver.new_type_ref(std::move($1), own($2));
    }
| BaseType Annotations
    {
      driver.debug("FieldType -> BaseType");
      $$ = driver.new_type_ref(*$1, own($2));
    }
| ContainerType Annotations
    {
      driver.debug("FieldType -> ContainerType");
      $$ = driver.new_type_ref(own($1), own($2));
    }

BaseType:
  tok_string
    {
      driver.debug("BaseType -> tok_string");
      $$ = &t_base_type::t_string();
    }
| tok_binary
    {
      driver.debug("BaseType -> tok_binary");
      $$ = &t_base_type::t_binary();
    }
| tok_bool
    {
      driver.debug("BaseType -> tok_bool");
      $$ = &t_base_type::t_bool();
    }
| tok_byte
    {
      driver.debug("BaseType -> tok_byte");
      $$ = &t_base_type::t_byte();
    }
| tok_i16
    {
      driver.debug("BaseType -> tok_i16");
      $$ = &t_base_type::t_i16();
    }
| tok_i32
    {
      driver.debug("BaseType -> tok_i32");
      $$ = &t_base_type::t_i32();
    }
| tok_i64
    {
      driver.debug("BaseType -> tok_i64");
      $$ = &t_base_type::t_i64();
    }
| tok_double
    {
      driver.debug("BaseType -> tok_double");
      $$ = &t_base_type::t_double();
    }
| tok_float
    {
      driver.debug("BaseType -> tok_float");
      $$ = &t_base_type::t_float();
    }

ContainerType:
  MapType  { $$ = $1; }
| SetType  { $$ = $1; }
| ListType { $$ = $1; }

MapType:
  tok_map "<" FieldType "," FieldType ">"
    {
      driver.debug("MapType -> tok_map<FieldType, FieldType>");
      $$ = new t_map(std::move($3), std::move($5));
    }

SetType:
  tok_set "<" FieldType ">"
    {
      driver.debug("SetType -> tok_set<FieldType>");
      $$ = new t_set(std::move($3));
    }

ListType:
  tok_list "<" FieldType ">"
    {
      driver.debug("ListType -> tok_list<FieldType>");
      $$ = new t_list(std::move($3));
    }

Annotations:
  "(" AnnotationList CommaOrSemicolonOptional ")"
    {
      driver.debug("Annotations => ( AnnotationList CommaOrSemicolonOptional)");
      $$ = $2;
    }
| "(" ")"
    {
      driver.debug("Annotations => ( )");
      $$ = nullptr;
    }
|   { $$ = nullptr; }

AnnotationList:
  AnnotationList CommaOrSemicolon Annotation
    {
      driver.debug("NodeAnnotationList => NodeAnnotationList CommaOrSemicolon Annotation");
      $$ = $1;
      $$->strings[$3->first] = std::move($3->second);
      delete $3;
    }
| Annotation
    {
      driver.debug("AnnotationList => Annotation");
      /* Just use a dummy structure to hold the annotations. */
      $$ = new t_annotations();
      $$->strings[$1->first] = std::move($1->second);
      delete $1;
    }

Annotation:
  Identifier "=" IntOrLiteral
    {
      driver.debug("Annotation -> Identifier = IntOrLiteral");
      $$ = new t_annotation{$1, {driver.get_source_range(@$), $3}};
    }
  | Identifier
    {
      driver.debug("Annotation -> Identifier");
      $$ = new t_annotation{$1, {driver.get_source_range(@$), "1"}};
    }

StructuredAnnotations:
  NonEmptyStructuredAnnotationList
    {
      driver.debug("StructuredAnnotations -> NonEmptyStructuredAnnotationList");
      $$ = $1;
    }
|   { $$ = nullptr; }

NonEmptyStructuredAnnotationList:
  NonEmptyStructuredAnnotationList StructuredAnnotation
    {
      driver.debug("NonEmptyStructuredAnnotationList -> NonEmptyStructuredAnnotationList StructuredAnnotation");
      $$ = $1;
      $$->emplace_back($2);
    }
| StructuredAnnotation
    {
      driver.debug("NonEmptyStructuredAnnotationList ->");
      $$ = new t_struct_annotations;
      $$->emplace_back($1);
    }

StructuredAnnotation:
  "@" ConstStruct
    {
      driver.debug("StructuredAnnotation => @ConstStruct");
      $$ = driver.new_struct_annotation(own($2)).release();
    }
| "@" ConstStructType
    {
      driver.debug("StructuredAnnotation => @ConstStructType");
      auto value = std::make_unique<t_const_value>();
      value->set_map();
      value->set_ttype(std::move($2));
      $$ = driver.new_struct_annotation(std::move(value)).release();
    }

IntOrLiteral:
  tok_literal
    {
      driver.debug("IntOrLiteral -> tok_literal");
      $$ = $1;
    }
| tok_bool_constant
    {
      char buf[21];  // max len of int64_t as string + null terminator
      driver.debug("IntOrLiteral -> tok_bool_constant");
      sprintf(buf, "%" PRIi32, $1 ? 1 : 0);
      $$ = buf;
    }
| Integer
    {
      char buf[21];  // max len of int64_t as string + null terminator
      driver.debug("IntOrLiteral -> Integer");
      sprintf(buf, "%" PRIi64, $1);
      $$ = buf;
    }

%%

/**
 * Method that will be called by the generated parser upon errors.
 */
void apache::thrift::compiler::yy::parser::error(const location_type&, std::string const& message) {
  /*
   * TODO(urielrivas): Pass the location as argument to yyerror.
   */
  driver.yyerror(message);
}
