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
#include <stdio.h>
#include <inttypes.h>
#include <limits.h>
#include "folly/String.h"
#include "thrift/compiler/main.h"
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
%token<id>     tok_literal
%token<dtext>  tok_doctext
%token<id>     tok_st_identifier

/**
 * Constant values
 */
%token<iconst> tok_int_constant
%token<dconst> tok_dub_constant

/**
 * Header keywords
 */
%token tok_include
%token tok_namespace
%token tok_cpp_namespace
%token tok_cpp_include
%token tok_php_namespace
%token tok_py_module
%token tok_perl_package
%token tok_java_package
%token tok_xsd_all
%token tok_xsd_optional
%token tok_xsd_nillable
%token tok_xsd_namespace
%token tok_xsd_attrs
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
%token tok_senum
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
%token tok_view

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
%type<tconstv>   FieldValue
%type<tstruct>   FieldList

%type<tfield>    ViewField
%type<tstruct>   ViewFieldList
%type<tstruct>   View

%type<tenum>     Enum
%type<tenum>     EnumDefList
%type<tenumv>    EnumDef
%type<tenumv>    EnumValue

%type<ttypedef>  Senum
%type<tbase>     SenumDefList
%type<id>        SenumDef

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
%type<tfield>    Param
%type<tfield>    StreamParam

%type<tstruct>   Throws
%type<tservice>  Extends
%type<tbool>     Oneway
%type<tbool>     XsdAll
%type<tbool>     XsdOptional
%type<tbool>     XsdNillable
%type<tstruct>   XsdAttributes

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
| tok_xsd_namespace tok_literal
    {
      pwarning(1, "'xsd_namespace' is deprecated. Use 'namespace xsd' instead");
      pdebug("Header -> tok_xsd_namespace tok_literal");
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
          g_program->add_include(path, std::string($2));
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
        g_scope->add_type($1->get_name(), $1);
        if (g_parent_scope != NULL) {
          g_parent_scope->add_type(g_parent_prefix + $1->get_name(), $1);
        }
      }
      $$ = $1;
    }
| Service
    {
      pdebug("Definition -> Service");
      if (g_parse_mode == PROGRAM) {
        g_scope->add_service($1->get_name(), $1);
        if (g_parent_scope != NULL) {
          g_parent_scope->add_service(g_parent_prefix + $1->get_name(), $1);
        }
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
| Senum
    {
      pdebug("TypeDefinition -> Senum");
      if (g_parse_mode == PROGRAM) {
        g_program->add_typedef($1);
      }
    }
| Struct
    {
      pdebug("TypeDefinition -> Struct");
      if (g_parse_mode == PROGRAM) {
        g_program->add_struct($1);
      }
    }
| View
    {
      pdebug("TypeDefinition -> View");
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
  tok_typedef FieldType tok_identifier
    {
      pdebug("TypeDef -> tok_typedef FieldType tok_identifier");
      t_typedef *td = new t_typedef(g_program, $2, $3);
      $$ = td;
    }

CommaOrSemicolonOptional:
  ','
    {}
| ';'
    {}
|
    {}

Enum:
  tok_enum tok_identifier '{' EnumDefList '}'
    {
      pdebug("Enum -> tok_enum tok_identifier { EnumDefList }");
      $$ = $4;
      $$->set_name($2);
    }

EnumDefList:
  EnumDefList EnumDef
    {
      pdebug("EnumDefList -> EnumDefList EnumDef");
      $$ = $1;
      $$->append($2);
    }
|
    {
      pdebug("EnumDefList -> ");
      $$ = new t_enum(g_program);
      y_enum_val = -1;
    }

EnumDef:
  CaptureDocText EnumValue CommaOrSemicolonOptional
    {
      pdebug("EnumDef -> EnumValue");
      $$ = $2;
      if ($1 != NULL) {
        $$->set_doc($1);
      }
      if (g_parse_mode == PROGRAM) {
        // The scope constants never get deleted, so it's okay for us
        // to share a single t_const object between our scope and the parent
        // scope
        t_const* constant = new t_const(g_type_i32, $2->get_name(),
                                        new t_const_value($2->get_value()));
        g_scope->add_constant($2->get_name(), constant);
        if (g_parent_scope != NULL) {
          g_parent_scope->add_constant(g_parent_prefix + $2->get_name(),
                                       constant);
        }
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
    }
|
  tok_identifier
    {
      pdebug("EnumValue -> tok_identifier");
      if (y_enum_val == INT32_MAX) {
        failure("enum value overflow at enum %s", $1);
      }
      ++y_enum_val;
      $$ = new t_enum_value($1, y_enum_val);
    }

Senum:
  tok_senum tok_identifier '{' SenumDefList '}'
    {
      pdebug("Senum -> tok_senum tok_identifier { SenumDefList }");
      $$ = new t_typedef(g_program, $4, $2);
    }

SenumDefList:
  SenumDefList SenumDef
    {
      pdebug("SenumDefList -> SenumDefList SenumDef");
      $$ = $1;
      $$->add_string_enum_val($2);
    }
|
    {
      pdebug("SenumDefList -> ");
      $$ = new t_base_type("string", t_base_type::TYPE_STRING);
      $$->set_string_enum(true);
    }

SenumDef:
  tok_literal CommaOrSemicolonOptional
    {
      pdebug("SenumDef -> tok_literal");
      $$ = $1;
    }

Const:
  tok_const FieldType tok_identifier '=' ConstValue CommaOrSemicolonOptional
    {
      pdebug("Const -> tok_const FieldType tok_identifier = ConstValue");
      if (g_parse_mode == PROGRAM) {
        $$ = new t_const($2, $3, $5);
        validate_const_type($$);

        g_scope->add_constant($3, $$);
        if (g_parent_scope != NULL) {
          g_parent_scope->add_constant(g_parent_prefix + $3, $$);
        }

      } else {
        $$ = NULL;
      }
    }

ConstValue:
  tok_int_constant
    {
      pdebug("ConstValue => tok_int_constant");
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
      t_const* constant = g_scope->get_constant($1);
      if (constant != NULL) {
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
  StructHead tok_identifier XsdAll '{' FieldList '}' TypeAnnotations
    {
      pdebug("Struct -> tok_struct tok_identifier { FieldList }");
      $5->set_xsd_all($3);
      $5->set_union($1 == struct_is_union);
      $$ = $5;
      $$->set_name($2);
      if ($7 != NULL) {
        $$->annotations_ = $7->annotations_;
        delete $7;
      }
      y_field_val = -1;
    }

View:
  tok_view tok_identifier ':' tok_identifier '{' ViewFieldList '}' TypeAnnotations
    {
      pdebug("View -> tok_view tok_identifier { ViewFieldList }");
      if (g_parse_mode == INCLUDES) {
        $$ = $6;
        $$->set_name($2);
      } else {
        // Lookup the identifier in the current scope
        t_type* parent_type = g_scope->get_type($4);
        if (parent_type == NULL || !parent_type->is_struct()) {
          yyerror("Struct \"%s\" has not been defined. %d", $4, g_parse_mode);
          exit(1);
        }
        t_struct* parent_struct = dynamic_cast<t_struct*>(parent_type);
        $$ = new t_struct(g_program);
        $$->set_name($2);
        $$->set_view_parent(parent_struct->get_view_parent());
        $$->annotations_ = parent_struct->annotations_;
        for (const auto& it : $6->get_members()) {
          if (!parent_struct->has_field_named(it->get_name().c_str())) {
            failure("view field '%s.%s' is not found in parent struct '%s'",
                    $2, it->get_name().c_str(), $4);
          }
          const t_field* f = parent_struct->get_field_named(it->get_name().c_str());
          t_field* nf;
          if (it->get_type() != NULL) {
            // Field is fully described, verify that definitions are compatible
            nf = it;
            if (nf->get_key() != f->get_key()) {
              failure("view field '%s.%s' has a different key from parent struct '%s':"
                      " %d vs %d",
                      $2, it->get_name().c_str(), $4, f->get_key(), nf->get_key());
            }
            if (nf->get_req() != f->get_req()) {
              failure("view field '%s.%s' has a different requirement specifier "
                      "from parent struct '%s'",
                      $2, it->get_name().c_str(), $4);
            }
            if (nf->get_type()->get_impl_full_name() != f->get_type()->get_impl_full_name()) {
              failure("view field '%s.%s' has a different type from parent struct '%s':"
                      " '%s' vs '%s'",
                      $2, it->get_name().c_str(), $4,
                      f->get_type()->get_impl_full_name().c_str(),
                      nf->get_type()->get_impl_full_name().c_str());
            }
            if (f->get_value()) {
              nf->set_value(new t_const_value(*f->get_value()));
            }
          } else {
            // It's just a reference to the field, copy the definition from the
            // parent struct
            nf = new t_field(*f);
            override_annotations(nf->annotations_, it->annotations_);
          }
          if (!$$->append(nf)) {
            yyerror("Field identifier %d for \"%s\" has already been used", nf->get_key(), nf->get_name().c_str());
            exit(1);
          }
          if (nf != it) {
            delete it;
          }
        }
        if ($8 != NULL) {
          override_annotations($$->annotations_, $8->annotations_);
          delete $8;
        }
        delete $6;
      }
      y_field_val = -1;
    }

XsdAll:
  tok_xsd_all
    {
      $$ = true;
    }
|
    {
      $$ = false;
    }

XsdOptional:
  tok_xsd_optional
    {
      $$ = true;
    }
|
    {
      $$ = false;
    }

XsdNillable:
  tok_xsd_nillable
    {
      $$ = true;
    }
|
    {
      $$ = false;
    }

XsdAttributes:
  tok_xsd_attrs '{' FieldList '}'
    {
      $$ = $3;
    }
|
    {
      $$ = NULL;
    }

Xception:
  tok_xception tok_identifier '{' FieldList '}' TypeAnnotations
    {
      pdebug("Xception -> tok_xception tok_identifier { FieldList }");
      $4->set_name($2);
      $4->set_xception(true);
      $$ = $4;
      if ($6 != NULL) {
        $$->annotations_ = $6->annotations_;
        delete $6;
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
                  " struct member, '%s' in '%s' is not", v.c_str(), $2);
        }

        auto field = $$->get_field_named(v.c_str());
        if (!field->get_type()->is_string()) {
          failure("member specified as exception 'message' should be of type "
                  "STRING, '%s' in '%s' is not", v.c_str(), $2);
        }
      }

      y_field_val = -1;
    }

Service:
  tok_service tok_identifier Extends '{' FlagArgs FunctionList UnflagArgs '}' FunctionAnnotations
    {
      pdebug("Service -> tok_service tok_identifier { FunctionList }");
      $$ = $6;
      $$->set_name($2);
      $$->set_extends($3);
      if ($9) {
        $$->annotations_ = $9->annotations_;
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
        $$ = g_scope->get_service($2);
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
  CaptureDocText Oneway FunctionType tok_identifier '(' ParamList ')' Throws FunctionAnnotations CommaOrSemicolonOptional
    {
      $6->set_name(std::string($4) + "_args");
      $$ = new t_function($3, $4, $6, $8, $9, $2);
      if ($1 != NULL) {
        $$->set_doc($1);
      }
      y_field_val = -1;
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
|
    {
      pdebug("ParamList -> ");
      $$ = new t_struct(g_program);
    }

Param:
  Field
    {
      pdebug("Param -> Field");
      $$ = $1;
    }
| StreamParam
    {
      pdebug("Param -> StreamParam");
      $$ = $1;
    }

StreamParam:
  CaptureDocText FieldIdentifier FieldRequiredness StreamType tok_identifier XsdOptional XsdNillable XsdAttributes TypeAnnotations CommaOrSemicolonOptional
    {
      pdebug("tok_int_constant : Param -> ParamType tok_identifier");
      if ($2.auto_assigned) {
        pwarning(1, "No field key specified for %s, resulting protocol may have conflicts or not be backwards compatible!", $5);
        if (g_strict >= 192) {
          yyerror("Implicit field keys are deprecated and not allowed with -strict");
          exit(1);
        }
      }

      $$ = new t_field($4, $5, $2.value);
      $$->set_req($3);
      $$->set_xsd_optional($6);
      $$->set_xsd_nillable($7);
      if ($1 != NULL) {
        $$->set_doc($1);
      }
      if ($8 != NULL) {
        $$->set_xsd_attrs($8);
      }
      if ($9 != NULL) {
        $$->annotations_ = $9->annotations_;
        delete $9;
      }
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
  tok_throws '(' FieldList ')'
    {
      pdebug("Throws -> tok_throws ( FieldList )");
      $$ = $3;
      if (g_parse_mode == PROGRAM && !validate_throws($$)) {
        yyerror("Throws clause may not contain non-exception types");
        exit(1);
      }
    }
|
    {
      $$ = new t_struct(g_program);
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
  CaptureDocText FieldIdentifier FieldRequiredness FieldType tok_identifier FieldValue XsdOptional XsdNillable XsdAttributes TypeAnnotations CommaOrSemicolonOptional
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
      if ($6 != NULL) {
        validate_field_value($$, $6);
        $$->set_value($6);
      }
      $$->set_xsd_optional($7);
      $$->set_xsd_nillable($8);
      if ($1 != NULL) {
        $$->set_doc($1);
      }
      if ($9 != NULL) {
        $$->set_xsd_attrs($9);
      }
      if ($10 != NULL) {
        $$->annotations_ = $10->annotations_;
        delete $10;
      }
    }

ViewFieldList:
  ViewFieldList ViewField
    {
      pdebug("ViewFieldList -> ViewFieldList , ViewField");
      $$ = $1;
      // do not bother about field identifiers
      if ($$->has_field_named($2->get_name().c_str())) {
        yyerror("Field name %s has already been listed", $2->get_name().c_str());
        exit(1);
      }
      $$->append($2);
    }
|
    {
      pdebug("ViewFieldList -> ");
      $$ = new t_struct(g_program);
    }

ViewField:
  tok_identifier TypeAnnotations CommaOrSemicolonOptional
    {
      pdebug("tok_int_constant : FieldTypeOptional tok_identifier");
      // used only for tracking names, we'll reuse structs from original struct
      $$ = new t_field(NULL, $1);
      if ($2 != NULL) {
        $$->annotations_ = $2->annotations_;
        delete $2;
      }
    }
| CaptureDocText tok_int_constant ':' FieldRequiredness FieldType tok_identifier TypeAnnotations CommaOrSemicolonOptional
    {
      // In order to make grammar parsable we have to make id specifier required
      // if type is provided.
      pdebug("ViewField -> tok_int_constant : FieldType tok_identifier");
      $$ = new t_field($5, $6, $2);
      $$->set_req($4);
      if ($1 != NULL) {
        $$->set_doc($1);
      }
      if ($7 != NULL) {
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
    }
|
    {
      $$.value = y_field_val--;
      $$.auto_assigned = true;
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
  FieldType
    {
      pdebug("FunctionType -> FieldType");
      $$ = $1;
    }
| tok_void
    {
      pdebug("FunctionType -> tok_void");
      $$ = g_type_void;
    }
| StreamType
    {
      pdebug("FunctionType -> StreamType");
      $$ = $1;
    }

FieldType:
  tok_identifier
    {
      pdebug("FieldType -> tok_identifier");
      if (g_parse_mode == INCLUDES) {
        // Ignore identifiers in include mode
        $$ = NULL;
      } else {
        // Lookup the identifier in the current scope
        $$ = g_scope->get_type($1);
        if ($$ == NULL) {
          /*
           * Either this type isn't yet declared, or it's never
             declared.  Either way allow it and we'll figure it out
             during generation.
           */
          $$ = new t_typedef(g_program, $1);
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
| ListType
    {
      pdebug("SimpleContainerType -> ListType");
      $$ = $1;
    }

MapType:
  tok_map '<' FieldType ',' FieldType '>'
    {
      pdebug("MapType -> tok_map <FieldType, FieldType>");
      $$ = new t_map($3, $5, false);
    }

HashMapType:
  tok_hash_map '<' FieldType ',' FieldType '>'
    {
      pdebug("HashMapType -> tok_hash_map <FieldType, FieldType>");
      $$ = new t_map($3, $5, true);
    }

SetType:
  tok_set '<' FieldType '>'
    {
      pdebug("SetType -> tok_set<FieldType>");
      $$ = new t_set($3);
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
        folly::join(std::string("','"), prio_list, end, s);
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
  tok_int_constant
    {
      char buf[21];  // max len of int64_t as string + null terminator
      pdebug("IntOrLiteral -> tok_int_constant");
      sprintf(buf, "%" PRIi64, $1);
      $$ = strdup(buf);
    }

%%
