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
#include "thrift/compiler/common.h"
#include "thrift/compiler/globals.h"
#include "thrift/compiler/parse/t_program.h"
#include "thrift/compiler/parse/t_scope.h"

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
char* y_enum_name = nullptr;

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

/**
 * This structure is used by the parser to hold the data types associated with
 * various parse nodes.
 */
%union {
  char*          id;
  int64_t        iconst;
  double         dconst;
  bool           tbool;
  t_doc*         tdoc;
  t_type*        ttype;
  t_base_type*   tbase;
  t_typedef*     ttypedef;
  t_enum*        tenum;
  t_enum_value*  tenumv;
  t_const*       tconst;
  t_const_value* tconstv;
  t_struct*      tstruct;
  t_structpair*  tstructpair;
  t_service*     tservice;
  t_function*    tfunction;
  t_field*       tfield;
  char*          dtext;
  t_field::e_req ereq;
  t_annotation*  tannot;
  t_field_id     tfieldid;
}

/**
 * Strings identifier
 */
%token<id>     tok_identifier
%token<id>     tok_client
%token<id>     tok_literal
%token<dtext>  tok_doctext
%token<id>     tok_st_identifier

/**
 * Constant values
 */
%token<iconst> tok_bool_constant
%token<iconst> tok_int_constant
%token<dconst> tok_dub_constant

/**
 * Header keywords
 */
%token tok_include
%token tok_namespace
%token tok_cpp_namespace
%token tok_cpp_include
%token tok_hs_include
%token tok_php_namespace
%token tok_py_module
%token tok_perl_package
%token tok_java_package
%token tok_ruby_namespace
%token tok_smalltalk_category
%token tok_smalltalk_prefix
%token tok_cocoa_prefix
%token tok_csharp_namespace

/**
 * Base datatype keywords
 */
%token tok_void
%token tok_bool
%token tok_byte
%token tok_string
%token tok_binary
%token tok_slist
%token tok_i16
%token tok_i32
%token tok_i64
%token tok_double
%token tok_float

/**
 * Complex type keywords
 */
%token tok_map
%token tok_hash_map
%token tok_list
%token tok_set
%token tok_hash_set
%token tok_stream

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

/**
 * Grammar nodes
 */

%type<ttype>     BaseType
%type<ttype>     SimpleBaseType
%type<ttype>     ContainerType
%type<ttype>     SimpleContainerType
%type<ttype>     MapType
%type<ttype>     HashMapType
%type<ttype>     SetType
%type<ttype>     HashSetType
%type<ttype>     ListType
%type<ttype>     StreamType

%type<tdoc>      Definition
%type<ttype>     TypeDefinition

%type<ttypedef>  Typedef

%type<ttype>     TypeAnnotations
%type<ttype>     TypeAnnotationList
%type<tannot>    TypeAnnotation
%type<id>        TypeAnnotationValue
%type<ttype>     FunctionAnnotations

%type<tfield>    Field
%type<tfieldid>  FieldIdentifier
%type<ereq>      FieldRequiredness
%type<ttype>     FieldType
%type<ttype>     PubsubStreamType
%type<tconstv>   FieldValue
%type<tstruct>   FieldList

%type<tenum>     Enum
%type<tenum>     EnumDefList
%type<tenumv>    EnumDef
%type<tenumv>    EnumValue

%type<tconst>    Const
%type<tconstv>   ConstValue
%type<tconstv>   ConstList
%type<tconstv>   ConstListContents
%type<tconstv>   ConstMap
%type<tconstv>   ConstMapContents

%type<iconst>    StructHead
%type<tstruct>   Struct
%type<tstruct>   Xception
%type<tservice>  Service

%type<tfunction> Function
%type<ttype>     FunctionType
%type<tservice>  FunctionList

%type<tstruct>   ParamList
%type<tstruct>   EmptyParamList
%type<tstruct>   MaybeStreamAndParamList
%type<tfield>    Param

%type<tstruct>   Throws
%type<tstruct>   ClientThrows
%type<tstructpair>   ThrowsThrows
%type<tservice>  Extends
%type<tbool>     Oneway

%type<dtext>     CaptureDocText
%type<id>        IntOrLiteral

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
      pdebug("Program -> Headers DefinitionList");
      /*
      TODO(dreiss): Decide whether full-program doctext is worth the trouble.
      if ($1 != NULL) {
        g_program->set_doc($1);
      }
      */
      clear_doctext();
    }

CaptureDocText:
    {
      if (g_parse_mode == PROGRAM) {
        $$ = g_doctext;
        g_doctext = NULL;
      } else {
        $$ = NULL;
      }
    }

/* TODO(dreiss): Try to DestroyDocText in all sorts or random places. */
DestroyDocText:
    {
      if (g_parse_mode == PROGRAM) {
        clear_doctext();
      }
    }

/* We have to DestroyDocText here, otherwise it catches the doctext
   on the first real element. */
HeaderList:
  HeaderList DestroyDocText Header
    {
      pdebug("HeaderList -> HeaderList Header");
    }
|
    {
      pdebug("HeaderList -> ");
    }

Header:
  Include
    {
      pdebug("Header -> Include");
    }
| tok_namespace tok_identifier tok_identifier
    {
      pdebug("Header -> tok_namespace tok_identifier tok_identifier");
      if (g_parse_mode == PROGRAM) {
        g_program->set_namespace($2, $3);
      }
    }
| tok_namespace tok_identifier tok_literal
    {
      pdebug("Header -> tok_namespace tok_identifier tok_literal");
      if (g_parse_mode == PROGRAM) {
        g_program->set_namespace($2, $3);
      }
    }
/* TODO(dreiss): Get rid of this once everyone is using the new hotness. */
| tok_cpp_namespace tok_identifier
    {
      pwarning(1, "'cpp_namespace' is deprecated. Use 'namespace cpp' instead");
      pdebug("Header -> tok_cpp_namespace tok_identifier");
      if (g_parse_mode == PROGRAM) {
        g_program->set_namespace("cpp", $2);
      }
    }
| tok_cpp_include tok_literal
    {
      pdebug("Header -> tok_cpp_include tok_literal");
      if (g_parse_mode == PROGRAM) {
        g_program->add_cpp_include($2);
      }
    }
| tok_hs_include tok_literal
    {
      pdebug("Header -> tok_hs_include tok_literal");
      // Do nothing. This syntax is handled by the hs compiler
    }
| tok_php_namespace tok_identifier
    {
      pwarning(1, "'php_namespace' is deprecated. Use 'namespace php' instead");
      pdebug("Header -> tok_php_namespace tok_identifier");
      if (g_parse_mode == PROGRAM) {
        g_program->set_namespace("php", $2);
      }
    }
/* TODO(dreiss): Get rid of this once everyone is using the new hotness. */
| tok_py_module tok_identifier
    {
      pwarning(1, "'py_module' is deprecated. Use 'namespace py' instead");
      pdebug("Header -> tok_py_module tok_identifier");
      if (g_parse_mode == PROGRAM) {
        g_program->set_namespace("py", $2);
      }
    }
/* TODO(dreiss): Get rid of this once everyone is using the new hotness. */
| tok_perl_package tok_identifier
    {
      pwarning(1, "'perl_package' is deprecated. Use 'namespace perl' instead");
      pdebug("Header -> tok_perl_namespace tok_identifier");
      if (g_parse_mode == PROGRAM) {
        g_program->set_namespace("perl", $2);
      }
    }
/* TODO(dreiss): Get rid of this once everyone is using the new hotness. */
| tok_ruby_namespace tok_identifier
    {
      pwarning(1, "'ruby_namespace' is deprecated. Use 'namespace rb' instead");
      pdebug("Header -> tok_ruby_namespace tok_identifier");
      if (g_parse_mode == PROGRAM) {
        g_program->set_namespace("rb", $2);
      }
    }
/* TODO(dreiss): Get rid of this once everyone is using the new hotness. */
| tok_smalltalk_category tok_st_identifier
    {
      pwarning(1, "'smalltalk_category' is deprecated. Use 'namespace smalltalk.category' instead");
      pdebug("Header -> tok_smalltalk_category tok_st_identifier");
      if (g_parse_mode == PROGRAM) {
        g_program->set_namespace("smalltalk.category", $2);
      }
    }
/* TODO(dreiss): Get rid of this once everyone is using the new hotness. */
| tok_smalltalk_prefix tok_identifier
    {
      pwarning(1, "'smalltalk_prefix' is deprecated. Use 'namespace smalltalk.prefix' instead");
      pdebug("Header -> tok_smalltalk_prefix tok_identifier");
      if (g_parse_mode == PROGRAM) {
        g_program->set_namespace("smalltalk.prefix", $2);
      }
    }
/* TODO(dreiss): Get rid of this once everyone is using the new hotness. */
| tok_java_package tok_identifier
    {
      pwarning(1, "'java_package' is deprecated. Use 'namespace java' instead");
      pdebug("Header -> tok_java_package tok_identifier");
      if (g_parse_mode == PROGRAM) {
        g_program->set_namespace("java", $2);
      }
    }
/* TODO(dreiss): Get rid of this once everyone is using the new hotness. */
| tok_cocoa_prefix tok_identifier
    {
      pwarning(1, "'cocoa_prefix' is deprecated. Use 'namespace cocoa' instead");
      pdebug("Header -> tok_cocoa_prefix tok_identifier");
      if (g_parse_mode == PROGRAM) {
        g_program->set_namespace("cocoa", $2);
      }
    }
/* TODO(dreiss): Get rid of this once everyone is using the new hotness. */
| tok_csharp_namespace tok_identifier
   {
     pwarning(1, "'csharp_namespace' is deprecated. Use 'namespace csharp' instead");
     pdebug("Header -> tok_csharp_namespace tok_identifier");
     if (g_parse_mode == PROGRAM) {
       g_program->set_namespace("csharp", $2);
     }
   }

Include:
  tok_include tok_literal
    {
      pdebug("Include -> tok_include tok_literal");
      if (g_parse_mode == INCLUDES) {
        std::string path = include_file(std::string($2));
        if (!path.empty()) {
          if (program_cache.find(path) == program_cache.end()) {
            program_cache[path] = g_program->add_include(path, std::string($2));
          } else {
            g_program->add_include(program_cache[path]);
          }
        }
      }
    }

DefinitionList:
  DefinitionList CaptureDocText Definition
    {
      pdebug("DefinitionList -> DefinitionList Definition");
      if ($2 != NULL && $3 != NULL) {
        $3->set_doc($2);
      }
    }
|
    {
      pdebug("DefinitionList -> ");
    }

Definition:
  Const
    {
      pdebug("Definition -> Const");
      if (g_parse_mode == PROGRAM) {
        g_program->add_const($1);
      }
      $$ = $1;
    }
| TypeDefinition
    {
      pdebug("Definition -> TypeDefinition");
      if (g_parse_mode == PROGRAM) {
        g_scope_cache->add_type(g_program->get_name() + "." + $1->get_name(), $1);
      }
      $$ = $1;
    }
| Service
    {
      pdebug("Definition -> Service");
      if (g_parse_mode == PROGRAM) {
        g_scope_cache->add_service(g_program->get_name() + "." + $1->get_name(), $1);
        g_program->add_service($1);
      }
      $$ = $1;
    }

TypeDefinition:
  Typedef
    {
      pdebug("TypeDefinition -> Typedef");
      if (g_parse_mode == PROGRAM) {
        g_program->add_typedef($1);
      }
    }
| Enum
    {
      pdebug("TypeDefinition -> Enum");
      if (g_parse_mode == PROGRAM) {
        g_program->add_enum($1);
      }
    }
| Struct
    {
      pdebug("TypeDefinition -> Struct");
      if (g_parse_mode == PROGRAM) {
        g_program->add_struct($1);
      }
    }
| Xception
    {
      pdebug("TypeDefinition -> Xception");
      if (g_parse_mode == PROGRAM) {
        g_program->add_xception($1);
      }
    }

Typedef:
  tok_typedef
    {
      lineno_stack.push(LineType::kTypedef, yylineno);
    }
  FieldType tok_identifier TypeAnnotations
    {
      pdebug("TypeDef -> tok_typedef FieldType tok_identifier");
      t_typedef *td = new t_typedef(g_program, $3, $4, g_scope_cache);
      $$ = td;
      $$->set_lineno(lineno_stack.pop(LineType::kTypedef));
      if ($5 != NULL) {
        $$->annotations_ = $5->annotations_;
        delete $5;
      }
    }

CommaOrSemicolonOptional:
  ','
    {}
| ';'
    {}
|
    {}

Enum:
  tok_enum
    {
      lineno_stack.push(LineType::kEnum, yylineno);
    }
  tok_identifier
    {
      assert(y_enum_name == nullptr);
      y_enum_name = $3;
    }
  '{' EnumDefList '}' TypeAnnotations
    {
      pdebug("Enum -> tok_enum tok_identifier { EnumDefList }");
      $$ = $6;
      $$->set_name($3);
      $$->set_lineno(lineno_stack.pop(LineType::kEnum));
      if ($8 != NULL) {
        $$->annotations_ = $8->annotations_;
        delete $8;
      }
      y_enum_name = nullptr;
    }

EnumDefList:
  EnumDefList EnumDef
    {
      pdebug("EnumDefList -> EnumDefList EnumDef");
      $$ = $1;
      $$->append($2);

      if (g_parse_mode == PROGRAM) {
        t_const_value* const_val = new t_const_value($2->get_value());
        const_val->set_is_enum();
        const_val->set_enum($$);
        const_val->set_enum_value($2);
        t_const* tconst = new t_const(
            g_program, g_type_i32, $2->get_name(), const_val);

        assert(y_enum_name != nullptr);
        string type_prefix = string(y_enum_name) + ".";
        g_scope_cache->add_constant(
            g_program->get_name() + "." + $2->get_name(), tconst);
        g_scope_cache->add_constant(
            g_program->get_name() + "." + type_prefix + $2->get_name(), tconst);
      }
    }
|
    {
      pdebug("EnumDefList -> ");
      $$ = new t_enum(g_program);
      y_enum_val = -1;
    }

EnumDef:
  CaptureDocText EnumValue TypeAnnotations CommaOrSemicolonOptional
    {
      pdebug("EnumDef -> EnumValue");
      $$ = $2;
      if ($1 != NULL) {
        $$->set_doc($1);
      }
      if ($3 != NULL) {
        $$->annotations_ = $3->annotations_;
        delete $3;
      }
    }

EnumValue:
  tok_identifier '=' tok_int_constant
    {
      pdebug("EnumValue -> tok_identifier = tok_int_constant");
      if ($3 < 0 && !g_allow_neg_enum_vals) {
        pwarning(1, "Negative value supplied for enum %s.", $1);
      }
      if ($3 < INT32_MIN || $3 > INT32_MAX) {
        // Note: this used to be just a warning.  However, since thrift always
        // treats enums as i32 values, I'm changing it to a fatal error.
        // I doubt this will affect many people, but users who run into this
        // will have to update their thrift files to manually specify the
        // truncated i32 value that thrift has always been using anyway.
        failure("64-bit value supplied for enum %s will be truncated.", $1);
      }
      y_enum_val = $3;
      $$ = new t_enum_value($1, y_enum_val);
      $$->set_lineno(yylineno);
    }
|
  tok_identifier
    {
      pdebug("EnumValue -> tok_identifier");
      if (y_enum_val == INT32_MAX) {
        failure("enum value overflow at enum %s", $1);
      }
      $$ = new t_enum_value($1);

      ++y_enum_val;
      $$->set_value(y_enum_val);
      $$->set_lineno(yylineno);
    }

Const:
  tok_const
    {
      lineno_stack.push(LineType::kConst, yylineno);
    }
  FieldType tok_identifier '=' ConstValue CommaOrSemicolonOptional
    {
      pdebug("Const -> tok_const FieldType tok_identifier = ConstValue");
      if (g_parse_mode == PROGRAM) {
        $$ = new t_const(g_program, $3, $4, $6);
        $$->set_lineno(lineno_stack.pop(LineType::kConst));
        validate_const_type($$);
        g_scope_cache->add_constant(g_program->get_name() + "." + $4, $$);
      } else {
        $$ = NULL;
      }
    }

ConstValue:
  tok_bool_constant
    {
      pdebug("ConstValue => tok_int_constant");
      $$ = new t_const_value();
      $$->set_bool($1);
    }
|  tok_int_constant
    {
      pdebug("constvalue => tok_int_constant");
      $$ = new t_const_value();
      $$->set_integer($1);
      if (!g_allow_64bit_consts && ($1 < INT32_MIN || $1 > INT32_MAX)) {
        pwarning(1, "64-bit constant \"%" PRIi64 "\" may not work in all languages.", $1);
      }
    }
| tok_dub_constant
    {
      pdebug("ConstValue => tok_dub_constant");
      $$ = new t_const_value();
      $$->set_double($1);
    }
| tok_literal
    {
      pdebug("ConstValue => tok_literal");
      $$ = new t_const_value($1);
    }
| tok_identifier
    {
      pdebug("ConstValue => tok_identifier");
      t_const* constant = g_scope_cache->get_constant($1);
      if (!constant) {
        constant = g_scope_cache->get_constant(g_program->get_name() + "." + $1);
      }
      if (constant != nullptr) {
        $$ = constant->get_value();
      } else {
        if (g_parse_mode == PROGRAM) {
          pwarning(1, "Constant strings should be quoted: %s", $1);
        }
        $$ = new t_const_value($1);
      }
    }
| ConstList
    {
      pdebug("ConstValue => ConstList");
      $$ = $1;
    }
| ConstMap
    {
      pdebug("ConstValue => ConstMap");
      $$ = $1;
    }

ConstList:
  '[' ConstListContents ']'
    {
      pdebug("ConstList => [ ConstListContents ]");
      $$ = $2;
    }

ConstListContents:
  ConstListContents ConstValue CommaOrSemicolonOptional
    {
      pdebug("ConstListContents => ConstListContents ConstValue CommaOrSemicolonOptional");
      $$ = $1;
      $$->add_list($2);
    }
|
    {
      pdebug("ConstListContents =>");
      $$ = new t_const_value();
      $$->set_list();
    }

ConstMap:
  '{' ConstMapContents '}'
    {
      pdebug("ConstMap => { ConstMapContents }");
      $$ = $2;
    }

ConstMapContents:
  ConstMapContents ConstValue ':' ConstValue CommaOrSemicolonOptional
    {
      pdebug("ConstMapContents => ConstMapContents ConstValue CommaOrSemicolonOptional");
      $$ = $1;
      $$->add_map($2, $4);
    }
|
    {
      pdebug("ConstMapContents =>");
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
        lineno_stack.push(LineType::kStruct, yylineno);
    }
  tok_identifier '{' FieldList '}' TypeAnnotations
    {
      pdebug("Struct -> tok_struct tok_identifier { FieldList }");
      $5->set_union($1 == struct_is_union);
      $$ = $5;
      $$->set_name($3);
      $$->set_lineno(lineno_stack.pop(LineType::kStruct));
      if ($7 != NULL) {
        $$->annotations_ = $7->annotations_;
        delete $7;
      }
      y_field_val = -1;
    }

Xception:
  tok_xception
    {
      lineno_stack.push(LineType::kXception, yylineno);
    }
  tok_identifier '{' FieldList '}' TypeAnnotations
    {
      pdebug("Xception -> tok_xception tok_identifier { FieldList }");
      $5->set_name($3);
      $5->set_xception(true);
      $$ = $5;
      $$->set_lineno(lineno_stack.pop(LineType::kXception));
      if ($7 != NULL) {
        $$->annotations_ = $7->annotations_;
        delete $7;
      }

      const char* annotations[] = {"message", "code"};
      for (auto& annotation: annotations) {
        if (g_parse_mode == PROGRAM
            && $$->has_field_named(annotation)
            && $$->annotations_.find(annotation) != $$->annotations_.end()
            && strcmp(annotation, $$->annotations_.find(annotation)->second.c_str()) != 0) {
          pwarning(1, "Some generators (eg. PHP) will ignore annotation '%s' "
                      "as it is also used as field", annotation);
        }
      }

      // Check that value of "message" annotation is
      // - a valid member of struct
      // - of type STRING
      if (g_parse_mode == PROGRAM
          && $$->annotations_.find("message") != $$->annotations_.end()) {
        const std::string v = $$->annotations_.find("message")->second;

        if (!$$->has_field_named(v.c_str())) {
          failure("member specified as exception 'message' should be a valid"
                  " struct member, '%s' in '%s' is not", v.c_str(), $3);
        }

        auto field = $$->get_field_named(v.c_str());
        if (!field->get_type()->is_string()) {
          failure("member specified as exception 'message' should be of type "
                  "STRING, '%s' in '%s' is not", v.c_str(), $3);
        }
      }

      y_field_val = -1;
    }

Service:
  tok_service
    {
      lineno_stack.push(LineType::kService, yylineno);
    }
  tok_identifier Extends '{' FlagArgs FunctionList UnflagArgs '}' FunctionAnnotations
    {
      pdebug("Service -> tok_service tok_identifier { FunctionList }");
      $$ = $7;
      $$->set_name($3);
      $$->set_extends($4);
      $$->set_lineno(lineno_stack.pop(LineType::kService));
      if ($10) {
        $$->annotations_ = $10->annotations_;
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
      pdebug("Extends -> tok_extends tok_identifier");
      $$ = NULL;
      if (g_parse_mode == PROGRAM) {
        $$ = g_scope_cache->get_service($2);
        if (!$$) {
          $$ = g_scope_cache->get_service(g_program->get_name() + "." + $2);
        }
        if ($$ == NULL) {
          yyerror("Service \"%s\" has not been defined.", $2);
          exit(1);
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
      pdebug("FunctionList -> FunctionList Function");
      $$ = $1;
      $1->add_function($2);
    }
|
    {
      pdebug("FunctionList -> ");
      $$ = new t_service(g_program);
    }

Function:
  CaptureDocText Oneway FunctionType tok_identifier '(' MaybeStreamAndParamList ')' ThrowsThrows FunctionAnnotations CommaOrSemicolonOptional
    {
      $6->set_name(std::string($4) + "_args");
      auto* rettype = $3;
      auto* arglist = $6;
      auto* func = new t_function(rettype, $4, arglist, $8->first, $8->second, $9, $2);
      $$ = func;

      if ($1 != NULL) {
        $$->set_doc($1);
      }
      $$->set_lineno(yylineno);
      y_field_val = -1;
    }


MaybeStreamAndParamList:
  PubsubStreamType tok_identifier ',' ParamList
  {
    pdebug("MaybeStreamAndParamList -> PubsubStreamType tok ParamList");
    t_struct* paramlist = $4;
    t_field* stream_field = new t_field($1, $2, 0);
    paramlist->set_stream_field(stream_field);
    $$ = paramlist;
  }
| PubsubStreamType tok_identifier EmptyParamList
  {
    pdebug("MaybeStreamAndParamList -> PubsubStreamType tok");
    t_struct* paramlist = $3;
    t_field* stream_field = new t_field($1, $2, 0);
    paramlist->set_stream_field(stream_field);
    $$ = paramlist;
  }
| ParamList
  {
    $$ = $1;
  }

ParamList:
  ParamList Param
    {
      pdebug("ParamList -> ParamList , Param");
      $$ = $1;
      if (!($$->append($2))) {
        yyerror("Parameter identifier %d for \"%s\" has already been used", $2->get_key(), $2->get_name().c_str());
        exit(1);
      }
    }
| EmptyParamList
    {
      $$ = $1;
    }

EmptyParamList:
    {
      pdebug("EmptyParamList -> nil");
      t_struct* paramlist = new t_struct(g_program);
      paramlist->set_paramlist(true);
      $$ = paramlist;
    }

Param:
  Field
    {
      pdebug("Param -> Field");
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

ThrowsThrows:
  Throws ClientThrows
		{
			$$ = new t_structpair($1, $2);
		}
| Throws
		{
			$$ = new t_structpair($1, nullptr);
		}
| ClientThrows
		{
			$$ = new t_structpair(new t_struct(g_program), $1);
		}
|   {
			$$ = new t_structpair(new t_struct(g_program), nullptr);
		}

Throws:
  tok_throws '(' FieldList ')'
    {
      pdebug("Throws -> tok_throws ( FieldList )");
      $$ = $3;
    }
ClientThrows:
  tok_client '(' FieldList ')'
    {
      pdebug("ClientThrows -> 'client throws' ( FieldList )");
      $$ = $3;
    }

FieldList:
  FieldList Field
    {
      pdebug("FieldList -> FieldList , Field");
      $$ = $1;
      if (!($$->append($2))) {
        yyerror("Field identifier %d for \"%s\" has already been used", $2->get_key(), $2->get_name().c_str());
        exit(1);
      }
    }
|
    {
      pdebug("FieldList -> ");
      $$ = new t_struct(g_program);
    }

Field:
  CaptureDocText FieldIdentifier FieldRequiredness FieldType tok_identifier FieldValue TypeAnnotations CommaOrSemicolonOptional
    {
      pdebug("tok_int_constant : Field -> FieldType tok_identifier");
      if ($2.auto_assigned) {
        pwarning(1, "No field key specified for %s, resulting protocol may have conflicts or not be backwards compatible!", $5);
        if (g_strict >= 192) {
          yyerror("Implicit field keys are deprecated and not allowed with -strict");
          exit(1);
        }
      }

      $$ = new t_field($4, $5, $2.value);
      $$->set_req($3);
      $$->set_lineno(lineno_stack.pop(LineType::kField));
      if ($6 != NULL) {
        validate_field_value($$, $6);
        $$->set_value($6);
      }
      if ($1 != NULL) {
        $$->set_doc($1);
      }
      if ($7 != NULL) {
        for (const auto& it : $7->annotations_) {
          if (it.first == "cpp.ref" || it.first == "cpp2.ref") {
            if ($3 != t_field::T_OPTIONAL) {
              pwarning(1, "cpp.ref field must be optional if it is recursive");
            }
            break;
          }
        }
        $$->annotations_ = $7->annotations_;
        delete $7;
      }
    }

FieldIdentifier:
  tok_int_constant ':'
    {
      if ($1 <= 0) {
        if (g_allow_neg_field_keys) {
          /*
           * g_allow_neg_field_keys exists to allow users to add explicitly
           * specified key values to old .thrift files without breaking
           * protocol compatibility.
           */
          if ($1 != y_field_val) {
            /*
             * warn if the user-specified negative value isn't what
             * thrift would have auto-assigned.
             */
            pwarning(1, "Negative field key (%d) differs from what would be "
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
          pwarning(1, "Nonpositive value (%d) not allowed as a field key.",
                   $1);
          $$.value = y_field_val--;
          $$.auto_assigned = true;
        }
      } else {
        $$.value = $1;
        $$.auto_assigned = false;
      }
      lineno_stack.push(LineType::kField, yylineno);
    }
|
    {
      $$.value = y_field_val--;
      $$.auto_assigned = true;
      lineno_stack.push(LineType::kField, yylineno);
    }

FieldRequiredness:
  tok_required
    {
      if (g_arglist) {
        if (g_parse_mode == PROGRAM) {
          pwarning(1, "required keyword is ignored in argument lists.");
        }
        $$ = t_field::T_OPT_IN_REQ_OUT;
      } else {
        $$ = t_field::T_REQUIRED;
      }
    }
| tok_optional
    {
      if (g_arglist) {
        if (g_parse_mode == PROGRAM) {
          pwarning(1, "optional keyword is ignored in argument lists.");
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
  '=' ConstValue
    {
      if (g_parse_mode == PROGRAM) {
        $$ = $2;
      } else {
        $$ = NULL;
      }
    }
|
    {
      $$ = NULL;
    }

FunctionType:
  PubsubStreamType
    {
      pdebug("FunctionType -> PubsubStreamType");
      $$ = $1;
    }
| FieldType
    {
      pdebug("FunctionType -> FieldType");
      $$ = $1;
    }
| tok_void
    {
      pdebug("FunctionType -> tok_void");
      $$ = g_type_void;
    }

PubsubStreamType:
  tok_stream FieldType
    {
      pdebug("PubsubStreamType -> tok_stream FieldType");
      $$ = new t_pubsub_stream($2);
    }

FieldType:
  tok_identifier TypeAnnotations
    {
      pdebug("FieldType -> tok_identifier");
      if (g_parse_mode == INCLUDES) {
        // Ignore identifiers in include mode
        $$ = NULL;
      } else {
        // Lookup the identifier in the current scope
        $$ = g_scope_cache->get_type($1);
        if (!$$) {
          $$ = g_scope_cache->get_type(g_program->get_name() + "." + $1);
        }
        if ($$ == NULL || $2 != NULL) {
          /*
           * Either this type isn't yet declared, or it's never
             declared.  Either way allow it and we'll figure it out
             during generation.
           */
          $$ = new t_typedef(g_program, $1, g_scope_cache);
          if ($2 != NULL) {
            $$->annotations_ = $2->annotations_;
            delete $2;
          }
        }
      }
    }
| BaseType
    {
      pdebug("FieldType -> BaseType");
      $$ = $1;
    }
| ContainerType
    {
      pdebug("FieldType -> ContainerType");
      $$ = $1;
    }

BaseType: SimpleBaseType TypeAnnotations
    {
      pdebug("BaseType -> SimpleBaseType TypeAnnotations");
      if ($2 != NULL) {
        $$ = new t_base_type(*static_cast<t_base_type*>($1));
        $$->annotations_ = $2->annotations_;
        delete $2;
      } else {
        $$ = $1;
      }
    }

SimpleBaseType:
  tok_string
    {
      pdebug("BaseType -> tok_string");
      $$ = g_type_string;
    }
| tok_binary
    {
      pdebug("BaseType -> tok_binary");
      $$ = g_type_binary;
    }
| tok_slist
    {
      pdebug("BaseType -> tok_slist");
      $$ = g_type_slist;
    }
| tok_bool
    {
      pdebug("BaseType -> tok_bool");
      $$ = g_type_bool;
    }
| tok_byte
    {
      pdebug("BaseType -> tok_byte");
      $$ = g_type_byte;
    }
| tok_i16
    {
      pdebug("BaseType -> tok_i16");
      $$ = g_type_i16;
    }
| tok_i32
    {
      pdebug("BaseType -> tok_i32");
      $$ = g_type_i32;
    }
| tok_i64
    {
      pdebug("BaseType -> tok_i64");
      $$ = g_type_i64;
    }
| tok_double
    {
      pdebug("BaseType -> tok_double");
      $$ = g_type_double;
    }
| tok_float
    {
      pdebug("BaseType -> tok_float");
      $$ = g_type_float;
    }

ContainerType: SimpleContainerType TypeAnnotations
    {
      pdebug("ContainerType -> SimpleContainerType TypeAnnotations");
      $$ = $1;
      if ($2 != NULL) {
        $$->annotations_ = $2->annotations_;
        delete $2;
      }
    }

SimpleContainerType:
  MapType
    {
      pdebug("SimpleContainerType -> MapType");
      $$ = $1;
    }
|  HashMapType
    {
      pdebug("SimpleContainerType -> HashMapType");
      $$ = $1;
    }
| SetType
    {
      pdebug("SimpleContainerType -> SetType");
      $$ = $1;
    }
| HashSetType
    {
      pdebug("SimpleContainerType -> HashSetType");
      $$ = $1;
    }
| ListType
    {
      pdebug("SimpleContainerType -> ListType");
      $$ = $1;
    }
| StreamType
    {
      pdebug("SimpleContainerType -> StreamType");
      $$ = $1;
    }

MapType:
  tok_map '<' FieldType ',' FieldType '>'
    {
      pdebug("MapType -> tok_map<FieldType, FieldType>");
      $$ = new t_map($3, $5, false);
    }

HashMapType:
  tok_hash_map '<' FieldType ',' FieldType '>'
    {
      pdebug("HashMapType -> tok_hash_map<FieldType, FieldType>");
      $$ = new t_map($3, $5, true);
    }

SetType:
  tok_set '<' FieldType '>'
    {
      pdebug("SetType -> tok_set<FieldType>");
      $$ = new t_set($3, false);
    }

HashSetType:
  tok_hash_set '<' FieldType '>'
    {
      pdebug("HashSetType -> tok_hash_set<FieldType>");
      $$ = new t_set($3, true);
    }

ListType:
  tok_list '<' FieldType '>'
    {
      pdebug("ListType -> tok_list<FieldType>");
      $$ = new t_list($3);
    }

StreamType:
  tok_stream '<' FieldType '>'
    {
      pdebug("StreamType -> tok_stream<FieldType>");
      $$ = new t_stream($3);
    }

TypeAnnotations:
  '(' TypeAnnotationList ')'
    {
      pdebug("TypeAnnotations -> ( TypeAnnotationList )");
      $$ = $2;
    }
|
    {
      pdebug("TypeAnnotations -> nil");
      $$ = NULL;
    }

TypeAnnotationList:
  TypeAnnotationList TypeAnnotation
    {
      pdebug("TypeAnnotationList -> TypeAnnotationList , TypeAnnotation");
      $$ = $1;
      $$->annotations_[$2->key] = $2->val;
      delete $2;
    }
|
    {
      /* Just use a dummy structure to hold the annotations. */
      $$ = new t_struct(g_program);
    }

TypeAnnotation:
  tok_identifier TypeAnnotationValue CommaOrSemicolonOptional
    {
      pdebug("TypeAnnotation TypeAnnotationValue");
      $$ = new t_annotation;
      $$->key = $1;
      $$->val = $2;
    }

TypeAnnotationValue:
  '=' IntOrLiteral
    {
      pdebug("TypeAnnotationValue -> = IntOrLiteral");
      $$ = $2;
    }
|
    {
      pdebug("TypeAnnotationValue ->");
      $$ = strdup("1");
    }

FunctionAnnotations:
  TypeAnnotations
    {
      pdebug("FunctionAnnotations -> TypeAnnotations");
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
        failure("Bad priority '%s'. Choose one of '%s'.",
                prio.c_str(), s.c_str());
      }
    }

IntOrLiteral:
  tok_literal
    {
      pdebug("IntOrLiteral -> tok_literal");
      $$ = $1;
    }
|
  tok_bool_constant
    {
      char buf[21];  // max len of int64_t as string + null terminator
      pdebug("IntOrLiteral -> tok_bool_constant");
      sprintf(buf, "%" PRIi64, $1);
      $$ = strdup(buf);
    }
|
  tok_int_constant
    {
      char buf[21];  // max len of int64_t as string + null terminator
      pdebug("IntOrLiteral -> tok_int_constant");
      sprintf(buf, "%" PRIi64, $1);
      $$ = strdup(buf);
    }

%%
