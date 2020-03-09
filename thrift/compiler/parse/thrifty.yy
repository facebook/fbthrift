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

#include "thrift/compiler/ast/t_annotated.h"
#include "thrift/compiler/ast/t_scope.h"
#include "thrift/compiler/ast/base_types.h"

#include "thrift/compiler/parse/parsing_driver.h"

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

/**
 * This global variable is used for automatic numbering of field indices etc.
 * when parsing the members of a struct. Field values are automatically
 * assigned starting from -1 and working their way down.
 */
int y_field_val = -1;
/**
 * This global variable is used for automatic numbering of enum values.
 * y_enum_val is the last value assigned; the next auto-assigned value will be
 * y_enum_val+1, and then it continues working upwards.  Explicitly specified
 * enum values reset y_enum_val to that value.
 */
int32_t y_enum_val = -1;
int g_arglist = 0;
const int struct_is_struct = 0;
const int struct_is_union = 1;
const char* y_enum_name = nullptr;

// Define an enum class for all types that have lineno embedded.
enum class LineType {
  kTypedef,
  kEnum,
  kEnumValue,
  kConst,
  kStruct,
  kService,
  kFunction,
  kField,
  kXception,
};
// The LinenoStack class is used for keeping track of line number and automatic
// type checking
class LinenoStack {
 public:
  void push(LineType type, int lineno) {
    stack_.emplace(type, lineno);
  }
  int pop(LineType type) {
    if (type != stack_.top().first) {
      throw std::logic_error("Popping wrong type from line number stack");
    }
    int lineno = stack_.top().second;
    stack_.pop();
    return lineno;
  }
 private:
  std::stack<std::pair<LineType, int>> stack_;
};
LinenoStack lineno_stack;

%}

%code requires
{

namespace apache {
namespace thrift {
namespace compiler {

class parsing_driver;
using t_structpair = std::pair<t_struct*, t_struct*>;
using t_typestructpair = std::pair<t_type*, t_struct*>;

} // namespace compiler
} // namespace thrift
} // namespace apache

}

%define api.token.constructor
%define api.value.type variant
%define api.namespace {apache::thrift::compiler::yy}

%param {apache::thrift::compiler::parsing_driver& driver} {yyscan_t raw_scanner}

/**
 * Strings identifier
 */
%token<std::string>     tok_identifier
%token<std::string>     tok_literal
%token<boost::optional<std::string>>
                        tok_doctext
%token<std::string>     tok_st_identifier

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
 * Function modifiers
 */
%token tok_oneway

/**
 * Thrift language keywords
 */
%token tok_typedef
%token tok_struct
%token tok_xception
%token tok_throws
%token tok_extends
%token tok_service
%token tok_enum
%token tok_const
%token tok_required
%token tok_optional
%token tok_union

%token tok_eof 0

/**
 * Grammar nodes
 */

%type<t_type*>          BaseType
%type<t_type*>          SimpleBaseType
%type<t_type*>          ContainerType
%type<t_type*>          SimpleContainerType
%type<t_type*>          MapType
%type<t_type*>          SetType
%type<t_type*>          ListType

%type<t_doc*>           Definition
%type<t_type*>          TypeDefinition

%type<t_typedef*>       Typedef

%type<t_type*>          TypeAnnotations
%type<t_type*>          TypeAnnotationList
%type<t_annotation*>    TypeAnnotation
%type<std::string>      TypeAnnotationValue
%type<t_type*>          FunctionAnnotations

%type<t_field*>         Field
%type<t_field_id>       FieldIdentifier
%type<t_field::e_req>   FieldRequiredness
%type<t_type*>          FieldType
%type<t_type*>          ResponseAndStreamReturnType
%type<t_type*>          ResponseAndSinkReturnType
%type<t_stream_response*>
                        StreamReturnType
%type<t_sink*>          SinkReturnType
%type<t_typestructpair> SinkFieldType
%type<t_const_value*>   FieldValue
%type<t_struct*>        FieldList

%type<t_enum*>          Enum
%type<t_enum*>          EnumDefList
%type<t_enum_value*>    EnumDef
%type<t_enum_value*>    EnumValue

%type<t_const*>         Const
%type<t_const_value*>   ConstValue
%type<t_const_value*>   ConstList
%type<t_const_value*>   ConstListContents
%type<t_const_value*>   ConstMap
%type<t_const_value*>   ConstMapContents

%type<int64_t>          StructHead
%type<t_struct*>        Struct
%type<t_struct*>        Xception
%type<t_service*>       Service

%type<t_function*>      Function
%type<t_type*>          FunctionType
%type<t_service*>       FunctionList

%type<t_struct*>        ParamList
%type<t_struct*>        EmptyParamList
%type<t_field*>         Param

%type<t_struct*>        Throws
%type<t_struct*>        MaybeThrows
%type<t_service*>       Extends
%type<bool>             Oneway

%type<boost::optional<std::string>>
                        CaptureDocText
%type<std::string>      IntOrLiteral

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
      if ($1 != NULL) {
        driver.program->set_doc($1);
      }
      */
      driver.clear_doctext();
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
| tok_namespace tok_identifier tok_identifier
    {
      driver.debug("Header -> tok_namespace tok_identifier tok_identifier");
      if (driver.mode == parsing_mode::PROGRAM) {
        driver.program->set_namespace($2, $3);
      }
    }
| tok_namespace tok_identifier tok_literal
    {
      driver.debug("Header -> tok_namespace tok_identifier tok_literal");
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

DefinitionList:
  DefinitionList CaptureDocText Definition
    {
      driver.debug("DefinitionList -> DefinitionList Definition");
      if (($3 != NULL) && $2) {
        $3->set_doc(std::string{*$2});
      }
    }
|
    {
      driver.debug("DefinitionList -> ");
    }

Definition:
  Const
    {
      driver.debug("Definition -> Const");
      if (driver.mode == parsing_mode::PROGRAM) {
        driver.program->add_const(std::unique_ptr<t_const>($1));
      } else {
        driver.delete_at_the_end($1);
      }
      $$ = $1;
    }
| TypeDefinition
    {
      driver.debug("Definition -> TypeDefinition");
      if (driver.mode == parsing_mode::PROGRAM) {
        driver.scope_cache->add_type(driver.program->get_name() + "." + $1->get_name(), $1);
      }
      $$ = $1;
    }
| Service
    {
      driver.debug("Definition -> Service");
      if (driver.mode == parsing_mode::PROGRAM) {
        driver.scope_cache->add_service(driver.program->get_name() + "." + $1->get_name(), $1);
        driver.program->add_service(std::unique_ptr<t_service>($1));
      } else {
        driver.delete_at_the_end($1);
      }
      $$ = $1;
    }

TypeDefinition:
  Typedef
    {
      driver.debug("TypeDefinition -> Typedef");
      if (driver.mode == parsing_mode::PROGRAM) {
        driver.program->add_typedef(std::unique_ptr<t_typedef>($1));
      } else {
        driver.delete_at_the_end($1);
      }
      $$ = $1;
    }
| Enum
    {
      driver.debug("TypeDefinition -> Enum");
      if (driver.mode == parsing_mode::PROGRAM) {
        driver.program->add_enum(std::unique_ptr<t_enum>($1));
      } else {
        driver.delete_at_the_end($1);
      }
      $$ = $1;
    }
| Struct
    {
      driver.debug("TypeDefinition -> Struct");
      if (driver.mode == parsing_mode::PROGRAM) {
        driver.program->add_struct(std::unique_ptr<t_struct>($1));
      } else {
        driver.delete_at_the_end($1);
      }
      $$ = $1;
    }
| Xception
    {
      driver.debug("TypeDefinition -> Xception");
      if (driver.mode == parsing_mode::PROGRAM) {
        driver.program->add_xception(std::unique_ptr<t_struct>($1));
      } else {
        driver.delete_at_the_end($1);
      }
      $$ = $1;
    }

Typedef:
  tok_typedef
    {
      lineno_stack.push(LineType::kTypedef, driver.scanner->get_lineno());
    }
  FieldType tok_identifier TypeAnnotations
    {
      driver.debug("TypeDef -> tok_typedef FieldType tok_identifier");
      t_typedef *td = new t_typedef(driver.program, $3, $4, driver.scope_cache);
      $$ = td;
      $$->set_lineno(lineno_stack.pop(LineType::kTypedef));
      if ($5 != NULL) {
        $$->annotations_ = $5->annotations_;
        $$->annotation_objects_ = $5->annotation_objects_;
        delete $5;
      }
    }

CommaOrSemicolonOptional:
  ","
    {}
| ";"
    {}
|
    {}

Enum:
  tok_enum
    {
      lineno_stack.push(LineType::kEnum, driver.scanner->get_lineno());
    }
  tok_identifier
    {
      assert(y_enum_name == nullptr);
      y_enum_name = $3.c_str();
    }
  "{" EnumDefList "}" TypeAnnotations
    {
      driver.debug("Enum -> tok_enum tok_identifier { EnumDefList }");
      $$ = $6;
      $$->set_name($3);
      $$->set_lineno(lineno_stack.pop(LineType::kEnum));
      if ($8 != NULL) {
        $$->annotations_ = $8->annotations_;
        $$->annotation_objects_ = $8->annotation_objects_;
        delete $8;
      }
      y_enum_name = nullptr;
    }

EnumDefList:
  EnumDefList EnumDef
    {
      driver.debug("EnumDefList -> EnumDefList EnumDef");
      $$ = $1;

      if (driver.mode == parsing_mode::PROGRAM) {
        auto const_val = std::make_unique<t_const_value>($2->get_value());

        const_val->set_is_enum();
        const_val->set_enum($$);
        const_val->set_enum_value($2);

        auto tconst = std::make_unique<t_const>(
            driver.program, i32_type(), $2->get_name(), std::move(const_val));

        assert(y_enum_name != nullptr);
        std::string type_prefix = std::string(y_enum_name) + ".";
        driver.scope_cache->add_constant(
            driver.program->get_name() + "." + $2->get_name(), tconst.get());
        driver.scope_cache->add_constant(
            driver.program->get_name() + "." + type_prefix + $2->get_name(), tconst.get());

        $$->append(std::unique_ptr<t_enum_value>($2), std::move(tconst));
      } else {
        driver.delete_at_the_end($2);
      }
    }
|
    {
      driver.debug("EnumDefList -> ");
      $$ = new t_enum(driver.program);
      y_enum_val = -1;
    }

EnumDef:
  CaptureDocText EnumValue TypeAnnotations CommaOrSemicolonOptional
    {
      driver.debug("EnumDef -> EnumValue");
      $$ = $2;
      if ($1) {
        $$->set_doc(std::string{*$1});
      }
      if ($3 != NULL) {
        $$->annotations_ = $3->annotations_;
        $$->annotation_objects_ = $3->annotation_objects_;
        delete $3;
      }
    }

EnumValue:
  tok_identifier "=" tok_int_constant
    {
      driver.debug("EnumValue -> tok_identifier = tok_int_constant");
      if ($3 < 0 && !driver.params.allow_neg_enum_vals) {
        driver.warning(1, "Negative value supplied for enum %s.", $1.c_str());
      }
      if ($3 < INT32_MIN || $3 > INT32_MAX) {
        // Note: this used to be just a warning.  However, since thrift always
        // treats enums as i32 values, I'm changing it to a fatal error.
        // I doubt this will affect many people, but users who run into this
        // will have to update their thrift files to manually specify the
        // truncated i32 value that thrift has always been using anyway.
        driver.failure("64-bit value supplied for enum %s will be truncated.", $1.c_str());
      }
      y_enum_val = $3;
      $$ = new t_enum_value($1, y_enum_val);
      $$->set_lineno(driver.scanner->get_lineno());
    }
|
  tok_identifier
    {
      driver.debug("EnumValue -> tok_identifier");
      if (y_enum_val == INT32_MAX) {
        driver.failure("enum value overflow at enum %s", $1.c_str());
      }
      $$ = new t_enum_value($1);

      ++y_enum_val;
      $$->set_value(y_enum_val);
      $$->set_lineno(driver.scanner->get_lineno());
    }

Const:
  tok_const
    {
      lineno_stack.push(LineType::kConst, driver.scanner->get_lineno());
    }
  FieldType tok_identifier "=" ConstValue CommaOrSemicolonOptional
    {
      driver.debug("Const -> tok_const FieldType tok_identifier = ConstValue");
      if (driver.mode == parsing_mode::PROGRAM) {
        $$ = new t_const(driver.program, $3, $4, std::unique_ptr<t_const_value>($6));
        $$->set_lineno(lineno_stack.pop(LineType::kConst));
        driver.validate_const_type($$);
        driver.scope_cache->add_constant(driver.program->get_name() + "." + $4, $$);
      } else {
        delete $6;
        $$ = NULL;
      }
    }

ConstValue:
  tok_bool_constant
    {
      driver.debug("ConstValue => tok_int_constant");
      $$ = new t_const_value();
      $$->set_bool($1);
    }
|  tok_int_constant
    {
      driver.debug("constvalue => tok_int_constant");
      $$ = new t_const_value();
      $$->set_integer($1);
      if (driver.mode == parsing_mode::PROGRAM) {
        if (!driver.params.allow_64bit_consts && ($1 < INT32_MIN || $1 > INT32_MAX)) {
          driver.warning(1, "64-bit constant \"%" PRIi64 "\" may not work in all languages.", $1);
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
| tok_identifier
    {
      driver.debug("ConstValue => tok_identifier");
      t_const* constant = driver.scope_cache->get_constant($1);
      driver.validate_not_ambiguous_enum($1);
      if (!constant) {
        auto name_with_program_name = driver.program->get_name() + "." + $1;
        constant = driver.scope_cache->get_constant(name_with_program_name);
        driver.validate_not_ambiguous_enum(name_with_program_name);
      }
      if (constant != nullptr) {
        // Copy const_value to perform isolated mutations
        t_const_value* const_value = constant->get_value();
        $$ = const_value->clone().release();

        // We only want to clone the value, while discarding all real type
        // information.
        $$->set_ttype(nullptr);
        $$->set_is_enum(false);
        $$->set_enum(nullptr);
        $$->set_enum_value(nullptr);
      } else {
        if (driver.mode == parsing_mode::PROGRAM) {
          driver.warning(1, "Constant strings should be quoted: %s", $1.c_str());
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

ConstList:
  "[" ConstListContents "]"
    {
      driver.debug("ConstList => [ ConstListContents ]");
      $$ = $2;
    }

ConstListContents:
  ConstListContents ConstValue CommaOrSemicolonOptional
    {
      driver.debug("ConstListContents => ConstListContents ConstValue CommaOrSemicolonOptional");
      $$ = $1;
      $$->add_list(std::unique_ptr<t_const_value>($2));
    }
|
    {
      driver.debug("ConstListContents =>");
      $$ = new t_const_value();
      $$->set_list();
    }

ConstMap:
  "{" ConstMapContents "}"
    {
      driver.debug("ConstMap => { ConstMapContents }");
      $$ = $2;
    }

ConstMapContents:
  ConstMapContents ConstValue ":" ConstValue CommaOrSemicolonOptional
    {
      driver.debug("ConstMapContents => ConstMapContents ConstValue CommaOrSemicolonOptional");
      $$ = $1;
      $$->add_map(std::unique_ptr<t_const_value>($2), std::unique_ptr<t_const_value>($4));
    }
|
    {
      driver.debug("ConstMapContents =>");
      $$ = new t_const_value();
      $$->set_map();
    }

StructHead:
  tok_struct
    {
      $$ = struct_is_struct;
    }
| tok_union
    {
      $$ = struct_is_union;
    }

Struct:
  StructHead
    {
        lineno_stack.push(LineType::kStruct, driver.scanner->get_lineno());
    }
  tok_identifier "{" FieldList "}" TypeAnnotations
    {
      driver.debug("Struct -> tok_struct tok_identifier { FieldList }");
      $5->set_union($1 == struct_is_union);
      $$ = $5;
      $$->set_name($3);
      $$->set_lineno(lineno_stack.pop(LineType::kStruct));
      if ($7 != NULL) {
        $$->annotations_ = $7->annotations_;
        $$->annotation_objects_ = $7->annotation_objects_;
        delete $7;
      }
      y_field_val = -1;
    }

Xception:
  tok_xception
    {
      lineno_stack.push(LineType::kXception, driver.scanner->get_lineno());
    }
  tok_identifier "{" FieldList "}" TypeAnnotations
    {
      driver.debug("Xception -> tok_xception tok_identifier { FieldList }");
      $5->set_name($3);
      $5->set_xception(true);
      $$ = $5;
      $$->set_lineno(lineno_stack.pop(LineType::kXception));
      if ($7 != NULL) {
        $$->annotations_ = $7->annotations_;
        $$->annotation_objects_ = $7->annotation_objects_;
        delete $7;
      }

      const char* annotations[] = {"message", "code"};
      for (auto& annotation: annotations) {
        if (driver.mode == parsing_mode::PROGRAM
            && $$->has_field_named(annotation)
            && $$->annotations_.find(annotation) != $$->annotations_.end()
            && strcmp(annotation, $$->annotations_.find(annotation)->second.c_str()) != 0) {
          driver.warning(1, "Some generators (eg. PHP) will ignore annotation '%s' "
                         "as it is also used as field", annotation);
        }
      }

      // Check that value of "message" annotation is
      // - a valid member of struct
      // - of type STRING
      if (driver.mode == parsing_mode::PROGRAM
          && $$->annotations_.find("message") != $$->annotations_.end()) {
        const std::string v = $$->annotations_.find("message")->second;

        if (!$$->has_field_named(v.c_str())) {
          driver.failure("member specified as exception 'message' should be a valid"
                         " struct member, '%s' in '%s' is not", v.c_str(), $3.c_str());
        }

        auto field = $$->get_field_named(v.c_str());
        if (!field->get_type()->is_string_or_binary()) {
          driver.failure("member specified as exception 'message' should be of type "
                         "STRING, '%s' in '%s' is not", v.c_str(), $3.c_str());
        }
      }

      y_field_val = -1;
    }

Service:
  tok_service
    {
      lineno_stack.push(LineType::kService, driver.scanner->get_lineno());
    }
  tok_identifier Extends "{" FlagArgs FunctionList UnflagArgs "}" FunctionAnnotations
    {
      driver.debug("Service -> tok_service tok_identifier { FunctionList }");
      $$ = $7;
      $$->set_name($3);
      $$->set_extends($4);
      $$->set_lineno(lineno_stack.pop(LineType::kService));
      if ($10) {
        $$->annotations_ = $10->annotations_;
        $$->annotation_objects_ = $10->annotation_objects_;
        delete $10;
      }
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
  tok_extends tok_identifier
    {
      driver.debug("Extends -> tok_extends tok_identifier");
      $$ = NULL;
      if (driver.mode == parsing_mode::PROGRAM) {
        $$ = driver.scope_cache->get_service($2);
        if (!$$) {
          $$ = driver.scope_cache->get_service(driver.program->get_name() + "." + $2);
        }
        if ($$ == NULL) {
          driver.yyerror("Service \"%s\" has not been defined.", $2.c_str());
          driver.end_parsing();
        }
      }
    }
|
    {
      $$ = NULL;
    }

FunctionList:
  FunctionList Function
    {
      driver.debug("FunctionList -> FunctionList Function");
      $$ = $1;
      $1->add_function(std::unique_ptr<t_function>($2));
    }
|
    {
      driver.debug("FunctionList -> ");
      $$ = new t_service(driver.program);
    }

Function:
  CaptureDocText Oneway FunctionType tok_identifier "(" ParamList ")" MaybeThrows FunctionAnnotations CommaOrSemicolonOptional
    {
      $6->set_name(std::string($4) + "_args");
      auto* rettype = $3;
      auto* arglist = $6;
      t_struct* streamthrows = rettype && rettype->is_streamresponse() ? static_cast<t_stream_response*>(rettype)->get_throws_struct() : nullptr;
      t_function* func;
      if (rettype && rettype->is_sink()) {
        func = new t_function(
          static_cast<t_sink*>(rettype),
          $4,
          std::unique_ptr<t_struct>(arglist),
          std::unique_ptr<t_struct>($8)
        );
      } else {
        func = new t_function(
          rettype,
          $4,
          std::unique_ptr<t_struct>(arglist),
          std::unique_ptr<t_struct>($8),
          std::unique_ptr<t_struct>(streamthrows),
          $2
        );
      }
      if ($9) {
        func->annotations_ = std::move($9->annotations_);
        func->annotation_objects_ = std::move($9->annotation_objects_);
      }
      delete $9;
      $$ = func;

      if ($1) {
        $$->set_doc(std::string{*$1});
      }
      $$->set_lineno(driver.scanner->get_lineno());
      y_field_val = -1;
    }

ParamList:
  ParamList Param
    {
      driver.debug("ParamList -> ParamList , Param");
      $$ = $1;
      if (!($$->append(std::unique_ptr<t_field>($2)))) {
        driver.yyerror("Parameter identifier %d for \"%s\" has already been used",
                       $2->get_key(), $2->get_name().c_str());
        driver.end_parsing();
      }
    }
| EmptyParamList
    {
      $$ = $1;
    }

EmptyParamList:
    {
      driver.debug("EmptyParamList -> nil");
      t_struct* paramlist = new t_struct(driver.program);
      paramlist->set_paramlist(true);
      $$ = paramlist;
    }

Param:
  Field
    {
      driver.debug("Param -> Field");
      $$ = $1;
    }

Oneway:
  tok_oneway
    {
      $$ = true;
    }
|
    {
      $$ = false;
    }


Throws:
  tok_throws "(" FieldList ")"
    {
      driver.debug("Throws -> tok_throws ( FieldList )");
      $$ = $3;
    }
MaybeThrows:
  Throws
		{
			$$ = $1;
		}
|   {
			$$ = new t_struct(driver.program);
		}

FieldList:
  FieldList Field
    {
      driver.debug("FieldList -> FieldList , Field");
      $$ = $1;
      if (!($$->append(std::unique_ptr<t_field>($2)))) {
        driver.yyerror("Field identifier %d for \"%s\" has already been used",
                       $2->get_key(), $2->get_name().c_str());
        driver.end_parsing();
      }
    }
|
    {
      driver.debug("FieldList -> ");
      $$ = new t_struct(driver.program);
    }

Field:
  CaptureDocText FieldIdentifier FieldRequiredness FieldType tok_identifier FieldValue TypeAnnotations CommaOrSemicolonOptional
    {
      driver.debug("tok_int_constant : Field -> FieldType tok_identifier");
      if ($2.auto_assigned) {
        driver.warning(1, "No field key specified for %s, resulting protocol may have conflicts or not be backwards compatible!", $5.c_str());
        if (driver.params.strict >= 192) {
          driver.yyerror("Implicit field keys are deprecated and not allowed with -strict");
          driver.end_parsing();
        }
      }

      $$ = new t_field($4, $5, $2.value);
      $$->set_req($3);
      $$->set_lineno(lineno_stack.pop(LineType::kField));
      if ($6 != NULL) {
        driver.validate_field_value($$, $6);
        $$->set_value(std::unique_ptr<t_const_value>($6));
      }
      if ($1) {
        $$->set_doc(std::string{*$1});
      }
      if ($7 != NULL) {
        for (const auto& it : $7->annotations_) {
          if (it.first == "cpp.ref" || it.first == "cpp2.ref") {
            if ($3 != t_field::T_OPTIONAL) {
              driver.warning(1, "cpp.ref field must be optional if it is recursive");
            }
            break;
          }
        }
        $$->annotations_ = $7->annotations_;
        $$->annotation_objects_ = $7->annotation_objects_;
        delete $7;
      }
    }

FieldIdentifier:
  tok_int_constant ":"
    {
      if ($1 <= 0) {
        if (driver.params.allow_neg_field_keys) {
          /*
           * allow_neg_field_keys exists to allow users to add explicitly
           * specified key values to old .thrift files without breaking
           * protocol compatibility.
           */
          if ($1 != y_field_val) {
            /*
             * warn if the user-specified negative value isn't what
             * thrift would have auto-assigned.
             */
            driver.warning(1, "Negative field key (%d) differs from what would be "
                           "auto-assigned by thrift (%d).", $1, y_field_val);
          }
          /*
           * Leave $1 as-is, and update y_field_val to be one less than $1.
           * The FieldList parsing will catch any duplicate key values.
           */
          y_field_val = $1 - 1;
          $$.value = $1;
          $$.auto_assigned = false;
        } else {
          driver.warning(1, "Nonpositive value (%d) not allowed as a field key.",
                         $1);
          $$.value = y_field_val--;
          $$.auto_assigned = true;
        }
      } else {
        $$.value = $1;
        $$.auto_assigned = false;
      }
      lineno_stack.push(LineType::kField, driver.scanner->get_lineno());
    }
|
    {
      $$.value = y_field_val--;
      $$.auto_assigned = true;
      lineno_stack.push(LineType::kField, driver.scanner->get_lineno());
    }

FieldRequiredness:
  tok_required
    {
      if (g_arglist) {
        if (driver.mode == parsing_mode::PROGRAM) {
          driver.warning(1, "required keyword is ignored in argument lists.");
        }
        $$ = t_field::T_OPT_IN_REQ_OUT;
      } else {
        $$ = t_field::T_REQUIRED;
      }
    }
| tok_optional
    {
      if (g_arglist) {
        if (driver.mode == parsing_mode::PROGRAM) {
          driver.warning(1, "optional keyword is ignored in argument lists.");
        }
        $$ = t_field::T_OPT_IN_REQ_OUT;
      } else {
        $$ = t_field::T_OPTIONAL;
      }
    }
|
    {
      $$ = t_field::T_OPT_IN_REQ_OUT;
    }

FieldValue:
  "=" ConstValue
    {
      if (driver.mode == parsing_mode::PROGRAM) {
        $$ = $2;
      } else {
        delete $2;
        $$ = NULL;
      }
    }
|
    {
      $$ = NULL;
    }

FunctionType:
  ResponseAndStreamReturnType
    {
      driver.debug("FunctionType -> ResponseAndStreamReturnType");
      $$ = $1;
    }
| ResponseAndSinkReturnType
    {
      driver.debug("FunctionType -> ResponseAndSinkReturnType");
      $$ = $1;
    }
| FieldType
    {
      driver.debug("FunctionType -> FieldType");
      $$ = $1;
    }
| tok_void
    {
      driver.debug("FunctionType -> tok_void");
      $$ = void_type();
    }

ResponseAndStreamReturnType:
  FieldType "," StreamReturnType
    {
      driver.debug("ResponseAndStreamReturnType -> FieldType, StreamReturnType");
      $3->set_first_response_type($1);
      $$ = $3;
    }
| StreamReturnType
    {
      driver.debug("ResponseAndStreamReturnType -> StreamReturnType");
      $$ = $1;
    }

StreamReturnType:
  tok_stream "<" FieldType ">"
  {
    driver.debug("StreamReturnType -> tok_stream < FieldType >");

    $$ = new t_stream_response($3);

    if (driver.mode == parsing_mode::INCLUDES) {
      driver.delete_at_the_end($$);
    } else {
      driver.program->add_unnamed_type(std::unique_ptr<t_type>{$$});
    }
  }
| tok_stream "<" FieldType Throws ">"
  {
    driver.debug("StreamReturnType -> tok_stream < FieldType Throws >");

    $$ = new t_stream_response($3, $4);

    if (driver.mode == parsing_mode::INCLUDES) {
      driver.delete_at_the_end($$);
    } else {
      driver.program->add_unnamed_type(std::unique_ptr<t_type>{$$});
    }
  }

ResponseAndSinkReturnType:
  FieldType "," SinkReturnType
    {
      driver.debug("ResponseAndSinkReturnType -> FieldType, SinkReturnType");
      $3->set_first_response($1);
      $$ = $3;    }
| SinkReturnType
    {
      driver.debug("ResponseAndSinkReturnType -> SinkReturnType");
      $$ = $1;
    }

SinkReturnType:
  tok_sink "<" SinkFieldType "," SinkFieldType ">"
    {
      driver.debug("SinkReturnType -> tok_sink<FieldType, FieldType>");
      $$ = new t_sink($3.first, $3.second, $5.first, $5.second);
      if (driver.mode == parsing_mode::INCLUDES) {
        driver.delete_at_the_end($$);
      } else {
        driver.program->add_unnamed_type(std::unique_ptr<t_type>{$$});
      }
    }
SinkFieldType:
  FieldType
    {
      $$ = std::make_pair($1, nullptr);
    }
| FieldType Throws
    {
      $$ = std::make_pair($1, $2);
    }

FieldType:
  tok_identifier TypeAnnotations
    {
      driver.debug("FieldType -> tok_identifier");
      if (driver.mode == parsing_mode::INCLUDES) {
        // Ignore identifiers in include mode
        $$ = NULL;

        delete $2;
      } else {
        // Lookup the identifier in the current scope
        $$ = driver.scope_cache->get_type($1);
        if (!$$) {
          $$ = driver.scope_cache->get_type(driver.program->get_name() + "." + $1);
        }
        // Create typedef in case we have annotations on the type.
        if ($$ != nullptr && $2 != nullptr) {
          auto td = new t_typedef(
              const_cast<t_program*>($$->get_program()), $$, $$->get_name(), driver.scope_cache);
          td->annotations_ = $2->annotations_;
          td->annotation_objects_ = $2->annotation_objects_;
          $$ = td;
          driver.program->add_unnamed_typedef(std::unique_ptr<t_typedef>{td});
          delete $2;
        }
        if ($$ == nullptr) {
          /*
           * Either this type isn't yet declared, or it's never
             declared.  Either way allow it and we'll figure it out
             during generation.
           */
          auto td = new t_typedef(driver.program, $1, driver.scope_cache);
          $$ = td;
          driver.program->add_placeholder_typedef(std::unique_ptr<t_typedef>{td});
          if ($2 != NULL) {
            $$->annotations_ = $2->annotations_;
            $$->annotation_objects_ = $2->annotation_objects_;
            delete $2;
          }
        }
      }
    }
| BaseType
    {
      driver.debug("FieldType -> BaseType");
      $$ = $1;
    }
| ContainerType
    {
      driver.debug("FieldType -> ContainerType");

      $$ = $1;

      if (driver.mode == parsing_mode::INCLUDES) {
        driver.delete_at_the_end($$);
      } else {
        driver.program->add_unnamed_type(std::unique_ptr<t_type>{$$});
      }
    }

BaseType: SimpleBaseType TypeAnnotations
    {
      driver.debug("BaseType -> SimpleBaseType TypeAnnotations");
      if ($2 != NULL) {
        $$ = new t_base_type(*static_cast<t_base_type*>($1));
        $$->annotations_ = $2->annotations_;
        $$->annotation_objects_ = $2->annotation_objects_;
        delete $2;

        if (driver.mode == parsing_mode::INCLUDES) {
          driver.delete_at_the_end($$);
        } else {
          driver.program->add_unnamed_type(std::unique_ptr<t_type>{$$});
        }
      } else {
        $$ = $1;
      }
    }

SimpleBaseType:
  tok_string
    {
      driver.debug("BaseType -> tok_string");
      $$ = string_type();
    }
| tok_binary
    {
      driver.debug("BaseType -> tok_binary");
      $$ = binary_type();
    }
| tok_bool
    {
      driver.debug("BaseType -> tok_bool");
      $$ = bool_type();
    }
| tok_byte
    {
      driver.debug("BaseType -> tok_byte");
      $$ = byte_type();
    }
| tok_i16
    {
      driver.debug("BaseType -> tok_i16");
      $$ = i16_type();
    }
| tok_i32
    {
      driver.debug("BaseType -> tok_i32");
      $$ = i32_type();
    }
| tok_i64
    {
      driver.debug("BaseType -> tok_i64");
      $$ = i64_type();
    }
| tok_double
    {
      driver.debug("BaseType -> tok_double");
      $$ = double_type();
    }
| tok_float
    {
      driver.debug("BaseType -> tok_float");
      $$ = float_type();
    }

ContainerType: SimpleContainerType TypeAnnotations
    {
      driver.debug("ContainerType -> SimpleContainerType TypeAnnotations");
      $$ = $1;
      if ($2 != NULL) {
        $$->annotations_ = $2->annotations_;
        $$->annotation_objects_ = $2->annotation_objects_;
        delete $2;
      }
    }

SimpleContainerType:
  MapType
    {
      driver.debug("SimpleContainerType -> MapType");
      $$ = $1;
    }
| SetType
    {
      driver.debug("SimpleContainerType -> SetType");
      $$ = $1;
    }
| ListType
    {
      driver.debug("SimpleContainerType -> ListType");
      $$ = $1;
    }

MapType:
  tok_map "<" FieldType "," FieldType ">"
    {
      driver.debug("MapType -> tok_map<FieldType, FieldType>");
      $$ = new t_map($3, $5);
    }

SetType:
  tok_set "<" FieldType ">"
    {
      driver.debug("SetType -> tok_set<FieldType>");
      $$ = new t_set($3);
    }

ListType:
  tok_list "<" FieldType ">"
    {
      driver.debug("ListType -> tok_list<FieldType>");
      $$ = new t_list($3);
    }

TypeAnnotations:
  "(" TypeAnnotationList ")"
    {
      driver.debug("TypeAnnotations -> ( TypeAnnotationList )");
      $$ = $2;
    }
|
    {
      driver.debug("TypeAnnotations -> nil");
      $$ = NULL;
    }

TypeAnnotationList:
  TypeAnnotationList TypeAnnotation
    {
      driver.debug("TypeAnnotationList -> TypeAnnotationList , TypeAnnotation");
      $$ = $1;
      if ($2->object_val == nullptr) {
        $$->annotations_[$2->key] = $2->val;
      } else {
        $$->annotation_objects_[$2->key] = $2->object_val;     
      }
      delete $2;
    }
|
    {
      /* Just use a dummy structure to hold the annotations. */
      $$ = new t_struct(driver.program);
    }

TypeAnnotation:
  tok_identifier "=" FieldType ConstMap CommaOrSemicolonOptional
    {
      driver.debug("TypeAnnotation TypeAnnotationValueObject");
      $$ = new t_annotation;
      $$->key = $1;
      $$->object_val = std::make_shared<t_const>(
        driver.program,
        $3,
        $1,
        std::unique_ptr<t_const_value>($4)
      );
      if (driver.mode == parsing_mode::PROGRAM) {
        driver.validate_const_type($$->object_val.get());
      }
    }
|
  tok_identifier TypeAnnotationValue CommaOrSemicolonOptional
    {
      driver.debug("TypeAnnotation TypeAnnotationValue");
      $$ = new t_annotation;
      $$->key = $1;
      $$->val = $2;
    }

TypeAnnotationValue:
  "=" IntOrLiteral
    {
      driver.debug("TypeAnnotationValue -> = IntOrLiteral");
      $$ = $2;
    }
|
    {
      driver.debug("TypeAnnotationValue ->");
      $$ = "1";
    }

FunctionAnnotations:
  TypeAnnotations
    {
      driver.debug("FunctionAnnotations -> TypeAnnotations");
      $$ = $1;
      if ($$ == nullptr) {
        break;
      }
      auto prio_iter = $$->annotations_.find("priority");
      if (prio_iter == $$->annotations_.end()) {
       break;
      }
      const std::string& prio = prio_iter->second;
      const std::string prio_list[] = {"HIGH_IMPORTANT", "HIGH", "IMPORTANT",
                                       "NORMAL", "BEST_EFFORT"};
      const auto end = prio_list + sizeof(prio_list)/sizeof(prio_list[0]);
      if (std::find(prio_list, end, prio) == end) {
        std::string s;
        for (const auto& prio : prio_list) {
          s += prio + "','";
        }
        s.erase(s.length() - 3);
        driver.failure("Bad priority '%s'. Choose one of '%s'.",
                       prio.c_str(), s.c_str());
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
void apache::thrift::compiler::yy::parser::error(std::string const& message) {
  driver.yyerror("%s", message.c_str());
}
