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

int g_arglist = 0;

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

namespace apache {
namespace thrift {
namespace compiler {

using t_typethrowspair = std::pair<t_type_ref, t_throws*>;
class t_container_type;

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
    {apache::thrift::compiler::yyscan_t raw_scanner}
    {YYSTYPE * yylval_param}
    {YYLTYPE * yylloc_param}

/**
 * Strings identifier
 */
%token<std::string>     tok_identifier
%token<std::string>     tok_literal
%token<t_doc>
                        tok_doctext

/**
 * Constant values
 */
%token<int64_t>   tok_bool_constant
%token<int64_t>   tok_int_constant
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

/**
 * Header keywords
 */
%token tok_include
%token tok_namespace
%token tok_cpp_include
%token tok_hs_include

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
%type<t_def_attrs*>                DefinitionAttrs
%type<t_ref<t_node>>               Definition

%type<t_typedef*>                  Typedef

%type<t_annotation*>               TypeAnnotation
%type<t_annotations*>              TypeAnnotations
%type<t_annotations*>              TypeAnnotationList
%type<t_annotations*>              FunctionAnnotations

%type<t_const*>                    StructuredAnnotation
%type<t_struct_annotations*>       StructuredAnnotations
%type<t_struct_annotations*>       NonEmptyStructuredAnnotationList

%type<t_field*>                    Field
%type<boost::optional<t_field_id>> FieldIdentifier
%type<t_field_qualifier>           FieldQualifier
%type<t_type_ref>                  FieldType
%type<t_stream_response*>          ResponseAndStreamReturnType
%type<t_sink*>                     ResponseAndSinkReturnType
%type<t_stream_response*>          StreamReturnType
%type<t_sink*>                     SinkReturnType
%type<t_typethrowspair>            SinkFieldType
%type<t_const_value*>              FieldValue
%type<t_field_list*>               FieldList

%type<t_enum*>                     Enum
%type<t_enum_value_list*>          EnumValueList
%type<t_enum_value*>               EnumValueDef
%type<t_enum_value*>               EnumValue

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
%type<t_type_ref>                  FunctionType
%type<t_function_list*>            FunctionList

%type<t_paramlist*>                ParamList
%type<t_paramlist*>                EmptyParamList
%type<t_field*>                    Param

%type<t_throws*>                   MaybeThrows
%type<t_ref<t_service>>            Extends
%type<t_function_qualifier>        FunctionQualifier

%type<t_doc>                       CaptureDocText
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
  HeaderList DefinitionList
    {
      driver.debug("Program -> Headers DefinitionList");
      /*
      TODO(dreiss): Decide whether full-program doctext is worth the trouble.
      if ($1) {
        driver.program->set_doc($1);
      }
      */
      driver.clear_doctext();
    }

Identifier:
  tok_identifier
    {
      $$ = $1;
    }
/* context sensitive keywords that should be allowed in identifiers. */
| tok_sink
    {
      $$ = "sink";
    }
| tok_oneway
    {
      $$ = "oneway";
    }
| tok_readonly
    {
      $$ = "readonly";
    }
| tok_idempotent
    {
      $$ = "idempotent";
    }
| tok_safe
    {
      $$ = "safe";
    }
| tok_transient
    {
      $$ = "transient";
    }
| tok_stateful
    {
      $$ = "stateful";
    }
| tok_permanent
    {
      $$ = "permanent";
    }
| tok_server
    {
      $$ = "server";
    }
| tok_client
    {
      $$ = "client";
    }


CaptureDocText:
    {
      if (driver.mode == parsing_mode::PROGRAM) {
        $$ = driver.doctext;
        driver.doctext = boost::none;
      } else {
        $$ = boost::none;
      }
    }

/* TODO(dreiss): Try to DestroyDocText in all sorts or random places. */
DestroyDocText:
    {
      if (driver.mode == parsing_mode::PROGRAM) {
        driver.clear_doctext();
      }
    }

/* We have to DestroyDocText here, otherwise it catches the doctext
   on the first real element. */
HeaderList:
  HeaderList DestroyDocText Header
    {
      driver.debug("HeaderList -> HeaderList Header");
    }
|
    {
      driver.debug("HeaderList -> ");
    }

Header:
  Include
    {
      driver.debug("Header -> Include");
    }
| tok_namespace Identifier Identifier
    {
      driver.debug("Header -> tok_namespace Identifier Identifier");
      if (driver.mode == parsing_mode::PROGRAM) {
        driver.program->set_namespace($2, $3);
      }
    }
| tok_namespace Identifier tok_literal
    {
      driver.debug("Header -> tok_namespace Identifier tok_literal");
      if (driver.mode == parsing_mode::PROGRAM) {
        driver.program->set_namespace($2, $3);
      }
    }
| tok_cpp_include tok_literal
    {
      driver.debug("Header -> tok_cpp_include tok_literal");
      if (driver.mode == parsing_mode::PROGRAM) {
        driver.program->add_cpp_include($2);
      }
    }
| tok_hs_include tok_literal
    {
      driver.debug("Header -> tok_hs_include tok_literal");
      // Do nothing. This syntax is handled by the hs compiler
    }

Include:
  tok_include tok_literal
    {
      driver.debug("Include -> tok_include tok_literal");
      if (driver.mode == parsing_mode::INCLUDES) {
        std::string path = driver.include_file(std::string($2));
        if (!path.empty()) {
          if (driver.program_cache.find(path) == driver.program_cache.end()) {
            auto included_program = driver.program->add_include(path, std::string($2), driver.scanner->get_lineno());
            driver.program_cache[path] = included_program.get();
            driver.program_bundle->add_program(std::move(included_program));
          } else {
            auto include = std::make_unique<t_include>(driver.program_cache[path]);
            include->set_lineno(driver.scanner->get_lineno());
            driver.program->add_include(std::move(include));
          }
        }
      }
    }

DefinitionAttrs:
  CaptureDocText StructuredAnnotations
    {
      driver.debug("DefinitionAttrs -> CaptureDocText StructuredAnnotations");
      if ($1 || $2 != nullptr) {
        $$ = new t_def_attrs{std::move($1), own($2)};
        driver.avoid_last_token_loc(!$1, @$, @2);
      } else {
        $$ = nullptr;
      }
    }

DefinitionList:
  DefinitionList Definition
    {
      driver.debug("DefinitionList -> DefinitionList Definition");
    }
|
    {
      driver.debug("DefinitionList -> ");
    }

Definition:
  Const
    {
      driver.debug("Definition -> Const");
      $$ = driver.add_def(own($1));
    }
| Typedef
    {
      driver.debug("Definition -> Typedef");
      $$ = driver.add_def(own($1));
    }
| Enum
    {
      driver.debug("Definition -> Enum");
      $$ = driver.add_def(own($1));
    }
| Struct
    {
      driver.debug("Definition -> Struct");
      $$ = driver.add_def(own($1));
    }
| Union
    {
      driver.debug("Definition -> Union");
      $$ = driver.add_def(own($1));
    }
| Exception
    {
      driver.debug("Definition -> Exception");
      $$ = driver.add_def(own($1));
    }
| Service
    {
      driver.debug("Definition -> Service");
      $$ = driver.add_def(own($1));
    }
| Interaction
    {
      driver.debug("Definition -> Interaction");
      $$ = driver.add_def(own($1));
    }

Typedef:
  DefinitionAttrs tok_typedef FieldType Identifier
    {
      driver.start_node(LineType::Typedef);
    }
   TypeAnnotations
    {
      driver.debug("TypeDef => DefinitionAttrs tok_typedef FieldType "
          "Identifier TypeAnnotations");
      $$ = new t_typedef(driver.program, std::move($4), std::move($3));
      driver.avoid_last_token_loc($1 == nullptr, @$, @2);
      driver.finish_node(
          $$, LineType::Typedef, @$, own($1), own($6));
    }

CommaOrSemicolon:
  ","
    {}
| ";"
    {}

CommaOrSemicolonOptional:
  CommaOrSemicolon
    {
      $$ = true;
    }
|
    {
      $$ = false;
    }

Enum:
  DefinitionAttrs tok_enum Identifier
    {
      driver.start_node(LineType::Enum);
    }
  "{" EnumValueList "}" TypeAnnotations
    {
      driver.debug("Enum => DefinitionAttrs tok_enum Identifier { EnumValueList } TypeAnnotations");
      $$ = new t_enum(driver.program, std::move($3));
      auto values = own($6);
      $$->set_values(std::move(*values));
      driver.avoid_last_token_loc($1 == nullptr, @$, @2);
      driver.finish_node(
          $$, LineType::Enum, @$, own($1), own($8));
    }

EnumValueList:
  EnumValueList EnumValueDef
    {
      driver.debug("EnumValueList -> EnumValueList EnumValueDef");
      $$ = $1;
      $$->emplace_back(own($2));
    }
|
    {
      driver.debug("EnumValueList -> ");
      $$ = new t_enum_value_list;
    }

EnumValueDef:
  DefinitionAttrs EnumValue
    {
      driver.start_node(LineType::EnumValue);
    }
  TypeAnnotations CommaOrSemicolonOptional
    {
      driver.debug("EnumValueDef => DefinitionAttrs EnumValue "
        "TypeAnnotations CommaOrSemicolonOptional");
      driver.avoid_last_token_loc($1 == nullptr, @$, @2);
      driver.finish_node(
          $2, LineType::EnumValue, @$, own($1), own($4));
      $$ = $2;
    }

EnumValue:
  Identifier "=" tok_int_constant
    {
      driver.debug("EnumValue -> Identifier = tok_int_constant");
      if ($3 < INT32_MIN || $3 > INT32_MAX) {
        // Note: this used to be just a warning.  However, since thrift always
        // treats enums as i32 values, I'm changing it to a fatal error.
        // I doubt this will affect many people, but users who run into this
        // will have to update their thrift files to manually specify the
        // truncated i32 value that thrift has always been using anyway.
        driver.failure([&](auto& o) {
          o << "64-bit value supplied for enum " << $1 << " will be truncated.";
        });
      }
      $$ = new t_enum_value(std::move($1));
      $$->set_value($3);
      $$->set_lineno(driver.scanner->get_lineno());
    }
|
  Identifier
    {
      driver.debug("EnumValue -> Identifier");
      $$ = new t_enum_value(std::move($1));
      $$->set_lineno(driver.scanner->get_lineno());
    }

Const:
  DefinitionAttrs tok_const FieldType Identifier
    {
      driver.start_node(LineType::Const);
    }
  "=" ConstValue TypeAnnotations CommaOrSemicolonOptional
    {
      driver.debug("DefinitionAttrs Const => tok_const FieldType Identifier = ConstValue");
      if (driver.mode == parsing_mode::PROGRAM) {
        $$ = new t_const(driver.program, std::move($3), std::move($4), own($7));
        driver.avoid_last_token_loc($1 == nullptr, @$, @2);
        driver.finish_node(
            $$, LineType::Const, @$, own($1), own($8));
      } else {
        // TODO(afuller): Looks like a bug where driver.finish_node is never called in this case.
        delete $1;
        delete $7;
        delete $8;
        $$ = nullptr;
      }
    }

ConstValue:
  tok_bool_constant
    {
      driver.debug("ConstValue => tok_int_constant");
      $$ = new t_const_value();
      $$->set_bool($1);
    }
| tok_int_constant
    {
      driver.debug("ConstValue => tok_int_constant");
      $$ = new t_const_value();
      $$->set_integer($1);
      if (driver.mode == parsing_mode::PROGRAM) {
        if (!driver.params.allow_64bit_consts && ($1 < INT32_MIN || $1 > INT32_MAX)) {
          driver.warning([&](auto& o) {
            o << "64-bit constant \"" << $1 << "\" may not work in all languages.";
          });
        }
      }
    }
| tok_dub_constant
    {
      driver.debug("ConstValue => tok_dub_constant");
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
      const t_const* constant = driver.scope_cache->find_constant($1);
      driver.validate_not_ambiguous_enum($1);
      if (constant == nullptr) {
        auto name_with_program_name = driver.program->name() + "." + $1;
        constant = driver.scope_cache->find_constant(name_with_program_name);
        driver.validate_not_ambiguous_enum(name_with_program_name);
      }
      if (constant != nullptr) {
        // Copy const_value to perform isolated mutations
        t_const_value* const_value = constant->get_value();
        $$ = const_value->clone().release();

        // We only want to clone the value, while discarding all real type
        // information.
        $$->set_ttype(boost::none);
        $$->set_is_enum(false);
        $$->set_enum(nullptr);
        $$->set_enum_value(nullptr);
      } else {
        if (driver.mode == parsing_mode::PROGRAM) {
          driver.warning([&](auto& o) {
            o << "Constant strings should be quoted: " << $1;
          });
        }
        $$ = new t_const_value($1);
      }
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
|
  "{" "}"
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
  DefinitionAttrs tok_struct Identifier
    {
      driver.start_node(LineType::Struct);
    }
  "{" FieldList "}" TypeAnnotations
    {
      driver.debug("Struct => DefinitionAttrs tok_struct Identifier "
        "{ FieldList } TypeAnnotations");
      $$ = new t_struct(driver.program, std::move($3));
      driver.avoid_last_token_loc($1 == nullptr, @$, @2);
      driver.finish_node(
          $$, LineType::Struct, @$, own($1), own($6), own($8));
    }

Union:
  DefinitionAttrs tok_union Identifier
    {
      driver.start_node(LineType::Union);
    }
  "{" FieldList "}" TypeAnnotations
    {
      driver.debug("Union => DefinitionAttrs tok_union Identifier "
        "{ FieldList } TypeAnnotations");
      $$ = new t_union(driver.program, std::move($3));
      driver.avoid_last_token_loc($1 == nullptr, @$, @2);
      driver.finish_node(
          $$, LineType::Union, @$, own($1), own($6), own($8));
    }

Exception:
  // TODO(afuller): Either make the qualifiers order agnostic or produce a better error message.
  DefinitionAttrs ErrorSafety ErrorKind ErrorBlame tok_exception Identifier
    {
      driver.start_node(LineType::Exception);
    }
  "{" FieldList "}" TypeAnnotations
    {
      driver.debug("Exception => DefinitionAttrs tok_exception "
        "Identifier { FieldList } TypeAnnotations");
      $$ = new t_exception(driver.program, std::move($6));
      $$->set_safety($2);
      $$->set_kind($3);
      $$->set_blame($4);
      driver.avoid_last_token_loc($1 == nullptr, @$, @2);
      driver.finish_node(
          $$, LineType::Exception, @$, own($1), own($9), own($11));

      const char* annotations[] = {"message", "code"};
      for (auto& annotation: annotations) {
        if (driver.mode == parsing_mode::PROGRAM
            && $$->get_field_by_name(annotation) != nullptr
            && $$->has_annotation(annotation)
            && strcmp(annotation, $$->get_annotation(annotation).c_str()) != 0) {
          driver.warning([&](auto& o) {
            o << "Some generators (e.g. PHP) will ignore annotation '"
              << annotation << "' as it is also used as field";
          });
        }
      }

      // Check that value of "message" annotation is
      // - a valid member of struct
      // - of type STRING
      if (driver.mode == parsing_mode::PROGRAM && $$->has_annotation("message")) {
        const std::string& v = $$->get_annotation("message");
        const auto* field = $$->get_field_by_name(v);
        if (field == nullptr) {
          driver.failure([&](auto& o) {
            o << "member specified as exception 'message' should be a valid"
              << " struct member, '" << v << "' in '" << $$->name() << "' is not";
          });
        }

        if (!field->get_type()->is_string_or_binary()) {
          driver.failure([&](auto& o) {
            o << "member specified as exception 'message' should be of type "
              << "STRING, '" << v << "' in '" << $$->name() << "' is not";
          });
        }
      }
    }

ErrorSafety:
  tok_safe
    {
      $$ = t_error_safety::safe;
    }
|
    {
      $$ = {};
    }

ErrorKind:
  tok_transient
    {
      $$ = t_error_kind::transient;
    }
|
  tok_stateful
    {
      $$ = t_error_kind::stateful;
    }
|
  tok_permanent
    {
      $$ = t_error_kind::permanent;
    }
|
    {
      $$ = {};
    }

ErrorBlame:
  tok_client
    {
      $$ = t_error_blame::client;
    }
|
  tok_server
    {
      $$ = t_error_blame::server;
    }
|
    {
      $$ = {};
    }


Service:
  DefinitionAttrs tok_service Identifier
    {
      driver.start_node(LineType::Service);
    }
  Extends "{" FlagArgs FunctionList UnflagArgs "}" FunctionAnnotations
    {
      driver.debug("Service => DefinitionAttrs tok_service "
        "Identifier Extends { FlagArgs FunctionList UnflagArgs } "
        "FunctionAnnotations");
      $$ = new t_service(driver.program, std::move($3), $5);
      driver.avoid_last_token_loc($1 == nullptr, @$, @2);
      driver.finish_node(
          $$, LineType::Service, @$, own($1), own($8), own($11));
    }

FlagArgs:
    {
       g_arglist = 1;
    }

UnflagArgs:
    {
       g_arglist = 0;
    }

Extends:
  tok_extends Identifier
    {
      driver.debug("Extends -> tok_extends Identifier");
      $$ = nullptr;
      if (driver.mode == parsing_mode::PROGRAM) {
        $$ = driver.scope_cache->find_service($2);
        if (!$$) {
          $$ = driver.scope_cache->find_service(driver.program->name() + "." + $2);
        }
        if (!$$) {
          driver.failure([&](auto& o) {
            o << "Service \"" << $2 << "\" has not been defined.";
          });
        }
      }
    }
|
    {
      $$ = nullptr;
    }

Interaction:
  DefinitionAttrs tok_interaction Identifier
    {
      driver.start_node(LineType::Interaction);
    }
  "{" FlagArgs FunctionList UnflagArgs "}" TypeAnnotations
    {
      driver.debug("Interaction -> tok_interaction Identifier { FunctionList }");
      $$ = new t_interaction(driver.program, std::move($3));
      driver.avoid_last_token_loc($1 == nullptr, @$, @2);
      driver.finish_node(
          $$, LineType::Interaction, @$, own($1), own($7), own($10));

      // TODO(afuller): Move this to a post parse phase.
      for (auto* func : $$->get_functions()) {
        func->set_is_interaction_member();
        if (func->has_annotation("thread")) {
          driver.failure("Interaction methods cannot be individually annotated with "
            "thread='eb'. Use process_in_event_base on the interaction instead.");
        }
      }
      if ($$->has_annotation("process_in_event_base")) {
        if ($$->has_annotation("serial")) {
          driver.failure("EB interactions are already serial");
        }
        for (auto* func : $$->get_functions()) {
          func->set_annotation("thread", "eb");
        }
      }
    }

FunctionList:
  FunctionList Function
    {
      driver.debug("FunctionList -> FunctionList Function");
      $1->emplace_back(own($2));
      $$ = $1;
    }
|
    {
      driver.debug("FunctionList -> ");
      $$ = new t_function_list;
    }

Function:
  DefinitionAttrs FunctionQualifier FunctionType Identifier
    {
      driver.start_node(LineType::Function);
    }
  "(" ParamList ")" MaybeThrows FunctionAnnotations CommaOrSemicolonOptional
    {
      driver.debug("Function => DefinitionAttrs FunctionQualifier "
        "FunctionType Identifier ( ParamList ) MaybeThrows "
        "FunctionAnnotations CommaOrSemicolonOptional");
      // TODO(afuller): Leave the param list unnamed.
      $7->set_name(std::string($4) + "_args");
      auto func = std::make_unique<t_function>(std::move($3), std::move($4), own($7), $2);
      func->set_exceptions(own($9));

      driver.avoid_last_token_loc($1 == nullptr, @$, @2);
      driver.avoid_next_token_loc($10 == nullptr, @$, @9);
      driver.finish_node(
          func.get(), LineType::Function, @$, own($1), own($10));
      $$ = func.release();
    }
  | tok_performs FieldType ";"
    {
      driver.debug("Function => tok_performs FieldType");
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

ParamList:
  ParamList Param
    {
      driver.debug("ParamList -> ParamList , Param");
      $$ = $1;
      auto param = own($2);
      if (!$$->try_append_field(std::move(param))) {
        driver.failure([&](auto& o) {
          o << "Parameter identifier " << param->get_key() << " for \""
          << param->get_name() << "\" has already been used";
        });
      }
    }
| EmptyParamList
    {
      $$ = $1;
    }

EmptyParamList:
    {
      driver.debug("EmptyParamList -> nil");
      $$ = new t_paramlist(driver.program);
    }

Param:
  Field
    {
      driver.debug("Param -> Field");
      $$ = $1;
    }

FunctionQualifier:
  tok_oneway
    {
      $$ = t_function_qualifier::one_way;
    }
| tok_idempotent
    {
      $$ = t_function_qualifier::idempotent;
    }
| tok_readonly
    {
      $$ = t_function_qualifier::read_only;
    }
|
    {
      $$ = {};
    }

MaybeThrows:
  tok_throws "(" FieldList ")"
    {
      driver.debug("MaybeThrows -> tok_throws ( FieldList )");
      $$ = driver.new_throws(own($3)).release();
    }
|   {
      driver.debug("MaybeThrows -> ");
      $$ = nullptr;
		}

FieldList:
  FieldList Field
    {
      driver.debug("FieldList -> FieldList Field");
      $$ = $1;
      $$->emplace_back($2);
    }
|
    {
      driver.debug("FieldList -> ");
      $$ = new t_field_list;
    }

Field:
  DefinitionAttrs FieldIdentifier FieldQualifier FieldType Identifier
    {
      driver.start_node(LineType::Field);
    }
   FieldValue TypeAnnotations CommaOrSemicolonOptional
    {
      driver.debug("Field => DefinitionAttrs FieldIdentifier FieldQualifier "
        "FieldType Identifier FieldValue TypeAnnotations CommaOrSemicolonOptional");
      t_field_id id;
      if ($2 == boost::none) {
        // Auto assign an id.
        id = driver.allocate_field_id($5);
      } else {
        id = *$2;
        if (id <= 0) {
          // TODO(afuller): Move this validation to ast_validator.
          if (driver.params.allow_neg_field_keys) {
            /*
              * allow_neg_field_keys exists to allow users to add explicitly
              * specified id values to old .thrift files without breaking
              * protocol compatibility.
              */
            if (id != driver.next_field_id()) {
              /*
                * warn if the user-specified negative value isn't what
                * thrift would have auto-assigned.
                */
              driver.warning([&](auto& o) {
                o << "Nonpositive field id (" << id << ") differs from what would "
                  << "be auto-assigned by thrift (" << driver.next_field_id() << ").";
              });
            }
          } else if (id == driver.next_field_id()) {
            driver.warning([&](auto& o) {
              o << "Nonpositive value (" << id << ") not allowed as a field id.";
            });
          } else {
            // TODO(afuller): Make ignoring the user provided value a failure.
            driver.warning([&](auto& o) {
              o << "Nonpositive field id (" << id<< ") differs from what is auto-"
                  "assigned by thrift. The id must positive or " << driver.next_field_id() << ".";
            });
            // Ignore user provided value and auto assign an id.
            id = driver.allocate_field_id($5);
            $2 = boost::none;
          }
          driver.reserve_field_id(id);
        }
      }
      $$ = new t_field(std::move($4), std::move($5), id, $2 != boost::none);
      $$->set_qualifier($3);
      if ($7 != nullptr) {
        $$->set_default_value(own($7));
      }
      driver.avoid_last_token_loc($1 == nullptr, @$, @2);
      driver.finish_node(
          $$, LineType::Field, @$, own($1), own($8));
    }

FieldIdentifier:
  tok_int_constant ":"
    {
      $$ = driver.as_field_id($1);
    }
|
    {
      $$ = boost::none;
    }

FieldQualifier:
  tok_required
    {
      if (g_arglist) {
        if (driver.mode == parsing_mode::PROGRAM) {
          driver.warning("required keyword is ignored in argument lists.");
        }
        $$ = {};
      } else {
        $$ = t_field_qualifier::required;
      }
    }
| tok_optional
    {
      if (g_arglist) {
        if (driver.mode == parsing_mode::PROGRAM) {
          driver.warning("optional keyword is ignored in argument lists.");
        }
        $$ = {};
      } else {
        $$ = t_field_qualifier::optional;
      }
    }
|
    {
      $$ = {};
    }

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
|
    {
      $$ = nullptr;
    }

FunctionType:
  ResponseAndStreamReturnType
    {
      driver.debug("FunctionType -> ResponseAndStreamReturnType");
      $$ = driver.new_type_ref(own($1), nullptr);
    }
| ResponseAndSinkReturnType
    {
      driver.debug("FunctionType -> ResponseAndSinkReturnType");
      $$ = driver.new_type_ref(own($1), nullptr);
    }
| FieldType
    {
      driver.debug("FunctionType -> FieldType");
      $$ = $1;
    }
| tok_void
    {
      driver.debug("FunctionType -> tok_void");
      $$ = t_base_type::t_void();
    }

ResponseAndStreamReturnType:
  FieldType "," StreamReturnType
    {
      driver.debug("ResponseAndStreamReturnType -> FieldType, StreamReturnType");
      $3->set_first_response_type(std::move($1));
      $$ = $3;
    }
| StreamReturnType
    {
      driver.debug("ResponseAndStreamReturnType -> StreamReturnType");
      $$ = $1;
    }

StreamReturnType:
  tok_stream "<" FieldType MaybeThrows ">"
  {
    driver.debug("StreamReturnType -> tok_stream < FieldType MaybeThrows >");
    auto stream_res = std::make_unique<t_stream_response>(std::move($3));
    stream_res->set_exceptions(own($4));
    $$ = stream_res.release();
  }

ResponseAndSinkReturnType:
  FieldType "," SinkReturnType
    {
      driver.debug("ResponseAndSinkReturnType -> FieldType, SinkReturnType");
      $3->set_first_response_type(std::move($1));
      $$ = $3;
    }
| SinkReturnType
    {
      driver.debug("ResponseAndSinkReturnType -> SinkReturnType");
      $$ = $1;
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
      $$ = std::make_pair($1, $2);
    }

FieldType:
  Identifier TypeAnnotations
    {
      driver.debug("FieldType => Identifier TypeAnnotations");
      $$ = driver.new_type_ref(std::move($1), own($2));
    }
| BaseType TypeAnnotations
    {
      driver.debug("FieldType -> BaseType");
      $$ = driver.new_type_ref(*$1, own($2));
    }
| ContainerType TypeAnnotations
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
  MapType
    {
      driver.debug("ContainerType -> MapType");
      $$ = $1;
    }
| SetType
    {
      driver.debug("ContainerType -> SetType");
      $$ = $1;
    }
| ListType
    {
      driver.debug("ContainerType -> ListType");
      $$ = $1;
    }

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

TypeAnnotations:
  "(" TypeAnnotationList CommaOrSemicolonOptional ")"
    {
      driver.debug("TypeAnnotations => ( TypeAnnotationList CommaOrSemicolonOptional)");
      $$ = $2;
    }
| "(" ")"
    {
      driver.debug("TypeAnnotations => ( )");
      $$ = nullptr;
    }
|
    {
      driver.debug("TypeAnnotations =>");
      $$ = nullptr;
    }

TypeAnnotationList:
  TypeAnnotationList CommaOrSemicolon TypeAnnotation
    {
      driver.debug("TypeAnnotationList => TypeAnnotationList CommaOrSemicolon TypeAnnotation");
      $$ = $1;
      $$->strings[$3->first] = std::move($3->second);
      delete $3;
    }
| TypeAnnotation
    {
      driver.debug("TypeAnnotationList => TypeAnnotation");
      /* Just use a dummy structure to hold the annotations. */
      $$ = new t_annotations();
      $$->strings[$1->first] = std::move($1->second);
      delete $1;
    }

TypeAnnotation:
  Identifier "=" IntOrLiteral
    {
      driver.debug("TypeAnnotation -> Identifier = IntOrLiteral");
      $$ = new t_annotation{$1, {driver.get_source_range(@$), $3}};
    }
  | Identifier
    {
      driver.debug("TypeAnnotation -> Identifier");
      $$ = new t_annotation{$1, {driver.get_source_range(@$), "1"}};
    }

StructuredAnnotations:
  NonEmptyStructuredAnnotationList
    {
      driver.debug("StructuredAnnotations -> NonEmptyStructuredAnnotationList");
      $$ = $1;
    }
|
    {
      driver.debug("StructuredAnnotations ->");
      $$ = nullptr;
    }

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

FunctionAnnotations:
  TypeAnnotations
    {
      driver.debug("FunctionAnnotations => TypeAnnotations");
      if (!$1) {
        $$ = nullptr;
        break;
      }
      $$ = $1;
      auto priority = $$->strings.find("priority");
      if (priority == $$->strings.end()) {
        break;
      }
      const std::string& prio = priority->second.value;
      const std::string prio_list[] = {"HIGH_IMPORTANT", "HIGH", "IMPORTANT",
                                       "NORMAL", "BEST_EFFORT"};
      const auto end = prio_list + sizeof(prio_list)/sizeof(prio_list[0]);
      if (std::find(prio_list, end, prio) == end) {
        std::string s;
        for (const auto& prio : prio_list) {
          s += prio + "','";
        }
        s.erase(s.length() - 3);
        driver.failure([&](auto& o) {
          o << "Bad priority '" << prio << "'. Choose one of '" << s << "'.";
        });
      }
    }

IntOrLiteral:
  tok_literal
    {
      driver.debug("IntOrLiteral -> tok_literal");
      $$ = $1;
    }
|
  tok_bool_constant
    {
      char buf[21];  // max len of int64_t as string + null terminator
      driver.debug("IntOrLiteral -> tok_bool_constant");
      sprintf(buf, "%" PRIi64, $1);
      $$ = buf;
    }
|
  tok_int_constant
    {
      char buf[21];  // max len of int64_t as string + null terminator
      driver.debug("IntOrLiteral -> tok_int_constant");
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
