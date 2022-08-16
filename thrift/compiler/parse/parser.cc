/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <cassert>
#include <exception>

#include <fmt/core.h>
#include <thrift/compiler/parse/lexer.h>
#include <thrift/compiler/parse/parser.h>

namespace apache {
namespace thrift {
namespace compiler {
namespace {

// A Thrift parser.
class parser {
 private:
  lexer& lexer_;
  parser_actions& actions_;
  diagnostics_engine& diags_;

  struct parse_error : std::exception {};

  struct token_kind {
    yy::parser::token::yytokentype value;

    token_kind(yy::parser::token::yytokentype val = yy::parser::token::tok_eof)
        : value(val) {}
    /* implicit */ token_kind(char c)
        : value(static_cast<yy::parser::token::yytokentype>(c)) {}

    operator yy::parser::token::yytokentype() const { return value; }
  };

  struct token {
    token_kind kind;
    source_range range;
  };
  token token_; // The current unconsumed token.
  boost::optional<yy::parser::symbol_type> bison_token_;

  // End of the last consumed token.
  source_location end_;

  void consume_token() {
    // Convert token kinds that represent individual chars into the chars
    // themselves for readability and diagnostics.
    auto make_token_kind = [](yy::parser::token::yytokentype t) -> token_kind {
      switch (t) {
        case yy::parser::token::tok_char_comma:
          return ',';
        case yy::parser::token::tok_char_semicolon:
          return ';';
        case yy::parser::token::tok_char_bracket_curly_l:
          return '{';
        case yy::parser::token::tok_char_bracket_curly_r:
          return '}';
        case yy::parser::token::tok_char_equal:
          return '=';
        case yy::parser::token::tok_char_bracket_square_l:
          return '[';
        case yy::parser::token::tok_char_bracket_square_r:
          return ']';
        case yy::parser::token::tok_char_colon:
          return ':';
        case yy::parser::token::tok_char_bracket_round_l:
          return '(';
        case yy::parser::token::tok_char_bracket_round_r:
          return ')';
        case yy::parser::token::tok_char_bracket_angle_l:
          return '<';
        case yy::parser::token::tok_char_bracket_angle_r:
          return '>';
        case yy::parser::token::tok_char_at_sign:
          return '@';
        case yy::parser::token::tok_char_plus:
          return '+';
        case yy::parser::token::tok_char_minus:
          return '-';
        default:
          return t;
      }
    };
    end_ = token_.range.end;
    bison_token_.emplace(lexer_.get_next_token());
    token_.kind = make_token_kind(bison_token_->token());
    token_.range = bison_token_->location;
    if (token_.kind == yy::parser::token::tok_error) {
      actions_.on_error();
    }
  }

  bool try_consume_token(token_kind kind) {
    if (token_.kind != kind) {
      return false;
    }
    consume_token();
    return true;
  }

  [[noreturn]] void report_expected(fmt::string_view expected) {
    diags_.error(token_.range.begin, "expected {}", expected);
    throw parse_error();
  }

  source_range expect_and_consume(token_kind expected) {
    auto range = token_.range;
    if (token_.kind != expected) {
      report_expected(to_string(expected));
    }
    consume_token();
    return range;
  }

  static std::string to_string(token_kind kind) {
    if (kind < 256) {
      return fmt::format("'{}'", static_cast<char>(kind));
    }
    switch (kind) {
      case yy::parser::token::tok_identifier:
        return "identifier";
      case yy::parser::token::tok_literal:
        return "string literal";
      case yy::parser::token::tok_doctext:
        return "doctext";
      case yy::parser::token::tok_inline_doc:
        return "inline doc";
      case yy::parser::token::tok_bool_constant:
        return "bool constant";
      case yy::parser::token::tok_int_constant:
        return "integral constant";
      case yy::parser::token::tok_dub_constant:
        return "floating-point constant";
      case yy::parser::token::tok_include:
        return "include";
      case yy::parser::token::tok_cpp_include:
        return "cpp_include";
      case yy::parser::token::tok_hs_include:
        return "hs_include";
      case yy::parser::token::tok_package:
        return "package";
      case yy::parser::token::tok_namespace:
        return "namespace";
      case yy::parser::token::tok_void:
        return "void";
      case yy::parser::token::tok_bool:
        return "bool";
      case yy::parser::token::tok_byte:
        return "byte";
      case yy::parser::token::tok_string:
        return "string";
      case yy::parser::token::tok_binary:
        return "binary";
      case yy::parser::token::tok_i16:
        return "i16";
      case yy::parser::token::tok_i32:
        return "i32";
      case yy::parser::token::tok_i64:
        return "i64";
      case yy::parser::token::tok_double:
        return "double";
      case yy::parser::token::tok_float:
        return "float";
      case yy::parser::token::tok_map:
        return "map";
      case yy::parser::token::tok_list:
        return "list";
      case yy::parser::token::tok_set:
        return "set";
      case yy::parser::token::tok_stream:
        return "stream";
      case yy::parser::token::tok_sink:
        return "sink";
      case yy::parser::token::tok_oneway:
        return "oneway";
      case yy::parser::token::tok_idempotent:
        return "idempotent";
      case yy::parser::token::tok_readonly:
        return "readonly";
      case yy::parser::token::tok_safe:
        return "safe";
      case yy::parser::token::tok_transient:
        return "transient";
      case yy::parser::token::tok_stateful:
        return "stateful";
      case yy::parser::token::tok_permanent:
        return "permanent";
      case yy::parser::token::tok_server:
        return "server";
      case yy::parser::token::tok_client:
        return "client";
      case yy::parser::token::tok_typedef:
        return "typedef";
      case yy::parser::token::tok_struct:
        return "struct";
      case yy::parser::token::tok_exception:
        return "exception";
      case yy::parser::token::tok_throws:
        return "throws";
      case yy::parser::token::tok_extends:
        return "extends";
      case yy::parser::token::tok_service:
        return "service";
      case yy::parser::token::tok_enum:
        return "enum";
      case yy::parser::token::tok_const:
        return "const";
      case yy::parser::token::tok_required:
        return "required";
      case yy::parser::token::tok_optional:
        return "optional";
      case yy::parser::token::tok_union:
        return "union";
      case yy::parser::token::tok_interaction:
        return "interaction";
      case yy::parser::token::tok_performs:
        return "performs";
      default:
        return "<unknown token>";
    }
  }

  // The parse methods are ordered top down from the most general to concrete.

  // Program: StatementList
  //
  // StatementList:
  //   StatementList StatementAnnotated CommaOrSemicolonOptional
  // | /* empty */
  bool parse_program() {
    consume_token();
    try {
      while (token_.kind != yy::parser::token::tok_eof) {
        auto stmt = parse_statement();
        if (stmt) {
          actions_.on_statement(std::move(stmt));
        }
      }
      actions_.on_program();
    } catch (const parse_error&) {
      return false; // The error has already been reported.
    }
    return true;
  }

  // StatementAnnotated:
  //   StatementAttrs Statement Annotations
  //
  // Statement:
  //     /* ProgramDocText (empty) */ Header
  //   | Definition
  //
  // Definition:
  //     Typedef
  //   | Enum
  //   | Const
  //   | Struct
  //   | Union
  //   | Exception
  //   | Service
  //   | Interaction
  std::unique_ptr<t_named> parse_statement() {
    auto loc = token_.range.begin;
    auto attrs = parse_statement_attrs();
    auto stmt = parse_header_or_definition();
    auto annotations = parse_annotations();
    auto end = end_;
    parse_comma_or_semicolon_optional();
    switch (stmt.type) {
      case t_statement_type::standard_header:
        actions_.on_standard_header(std::move(attrs), std::move(annotations));
        break;
      case t_statement_type::program_header:
        actions_.on_program_header(
            {loc, end}, std::move(attrs), std::move(annotations));
        break;
      case t_statement_type::definition:
        actions_.on_definition(
            {loc, end}, *stmt.def, std::move(attrs), std::move(annotations));
        break;
    }
    return std::move(stmt.def);
  }

  struct statement {
    t_statement_type type = t_statement_type::definition;
    std::unique_ptr<t_named> def;

    statement(t_statement_type t = t_statement_type::definition) : type(t) {}

    template <typename Named>
    /* implicit */ statement(std::unique_ptr<Named> n) : def(std::move(n)) {}
  };

  statement parse_header_or_definition() {
    switch (token_.kind) {
      case yy::parser::token::tok_include:
      case yy::parser::token::tok_cpp_include:
      case yy::parser::token::tok_hs_include:
      case yy::parser::token::tok_package:
        return parse_include_or_package();
      case yy::parser::token::tok_namespace:
        parse_namespace();
        return t_statement_type::standard_header;
      case yy::parser::token::tok_typedef:
        return parse_typedef();
      case yy::parser::token::tok_enum:
        return parse_enum();
      case yy::parser::token::tok_const:
        return parse_const();
      case yy::parser::token::tok_struct:
        return parse_struct();
      case yy::parser::token::tok_union:
        return parse_union();
      case yy::parser::token::tok_safe:
      case yy::parser::token::tok_transient:
      case yy::parser::token::tok_stateful:
      case yy::parser::token::tok_permanent:
      case yy::parser::token::tok_client:
      case yy::parser::token::tok_server:
      case yy::parser::token::tok_exception:
        return parse_exception();
      case yy::parser::token::tok_service:
        return parse_service();
      case yy::parser::token::tok_interaction:
        return parse_interaction();
      default:
        report_expected("header or definition");
    }
  }

  // Header:
  //     tok_include tok_literal
  //   | tok_package tok_literal
  //   | tok_namespace Identifier Identifier
  //   | tok_namespace Identifier tok_literal
  //   | tok_cpp_include tok_literal
  //   | tok_hs_include tok_literal
  t_statement_type parse_include_or_package() {
    auto kind = token_.kind;
    source_location loc = token_.range.begin;
    actions_.on_program_doctext();
    consume_token();
    if (token_.kind != yy::parser::token::tok_literal) {
      report_expected("string literal");
    }
    auto literal = bison_token_->value.as<std::string>();
    auto range = source_range{loc, token_.range.end};
    consume_token();
    switch (kind) {
      case yy::parser::token::tok_package:
        actions_.on_package(range, std::move(literal));
        return t_statement_type::program_header;
      case yy::parser::token::tok_include:
        actions_.on_include(range, std::move(literal));
        break;
      case yy::parser::token::tok_cpp_include:
        actions_.on_cpp_include(range, std::move(literal));
        break;
      case yy::parser::token::tok_hs_include:
        actions_.on_hs_include(range, std::move(literal));
        break;
      default:
        assert(false);
    }
    return t_statement_type::standard_header;
  }

  void parse_namespace() {
    assert(token_.kind == yy::parser::token::tok_namespace);
    actions_.on_program_doctext();
    consume_token();
    auto language = parse_identifier();
    std::string ns;
    if (token_.kind == yy::parser::token::tok_literal) {
      ns = bison_token_->value.as<std::string>();
      consume_token();
    } else {
      ns = parse_identifier();
    }
    return actions_.on_namespace(language, ns);
  }

  // StatementAttrs: /* CaptureDocText (empty) */ StructuredAnnotations
  //
  // StructuredAnnotations:
  //     NonEmptyStructuredAnnotationList
  //   | /* empty */
  //
  // NonEmptyStructuredAnnotationList:
  //     NonEmptyStructuredAnnotationList StructuredAnnotation
  //   | StructuredAnnotation
  std::unique_ptr<t_def_attrs> parse_statement_attrs() {
    auto doc = actions_.on_doctext();
    auto annotations = std::unique_ptr<t_struct_annotations>();
    while (auto annotation = parse_structured_annotation()) {
      if (!annotations) {
        annotations = std::make_unique<t_struct_annotations>();
      }
      annotations->emplace_back(std::move(annotation));
    }
    return actions_.on_statement_attrs(std::move(doc), std::move(annotations));
  }

  // InlineDocOptional: tok_inline_doc | /* empty */
  t_docstring parse_inline_doc_optional() {
    if (token_.kind != yy::parser::token::tok_inline_doc) {
      return {};
    }
    auto doc = bison_token_->value.as<t_docstring>();
    consume_token();
    return actions_.on_inline_doc(std::move(doc));
  }

  // StructuredAnnotation:
  //     "@" ConstStruct
  //   | "@" ConstStructType
  std::unique_ptr<t_const> parse_structured_annotation() {
    auto loc = token_.range.begin;
    if (!try_consume_token('@')) {
      return {};
    }
    auto name_range = token_.range;
    auto name = parse_identifier();
    if (token_.kind != '{') {
      return actions_.on_structured_annotation({loc, name_range.end}, name);
    }
    auto const_value = parse_const_struct_body(name_range, std::move(name));
    return actions_.on_structured_annotation(
        {loc, name_range.end}, std::move(const_value));
  }

  // Annotations:
  //     "(" AnnotationList CommaOrSemicolonOptional ")"
  //   | "(" ")"
  //   | /* empty */
  //
  // AnnotationList:
  //     AnnotationList CommaOrSemicolon Annotation
  //   | Annotation
  //
  // Annotation: Identifier "=" IntOrLiteral | Identifier
  //
  // IntOrLiteral: tok_literal | tok_bool_constant | Integer
  std::unique_ptr<t_annotations> parse_annotations() {
    if (!try_consume_token('(')) {
      return {};
    }
    auto annotations = std::unique_ptr<t_annotations>();
    while (token_.kind != ')') {
      if (!annotations) {
        annotations = std::make_unique<t_annotations>();
      }
      auto range = token_.range;
      auto key = parse_identifier();
      auto value = std::string("1");
      if (try_consume_token('=')) {
        range.end = token_.range.end;
        if (token_.kind == yy::parser::token::tok_literal) {
          value = bison_token_->value.as<std::string>();
          consume_token();
        } else if (token_.kind == yy::parser::token::tok_bool_constant) {
          value = fmt::format("{:d}", bison_token_->value.as<bool>());
          consume_token();
        } else if (auto integer = try_parse_integer()) {
          value = fmt::format("{}", *integer);
        } else {
          report_expected("integer, bool or string");
        }
      }
      annotations->strings[std::move(key)] = {range, std::move(value)};
      if (!parse_comma_or_semicolon_optional()) {
        break;
      }
    }
    expect_and_consume(')');
    return annotations;
  }

  // Service: tok_service Identifier Extends "{" FunctionList "}"
  //
  // Extends: tok_extends Identifier | /* empty */
  std::unique_ptr<t_service> parse_service() {
    auto loc = expect_and_consume(yy::parser::token::tok_service).begin;
    auto name = parse_identifier();
    auto base = std::string();
    if (try_consume_token(yy::parser::token::tok_extends)) {
      base = parse_identifier();
    }
    auto functions = parse_braced_function_list();
    return actions_.on_service(
        {loc, end_}, std::move(name), std::move(base), std::move(functions));
  }

  // Interaction: tok_interaction Identifier "{" FunctionList "}"
  std::unique_ptr<t_interaction> parse_interaction() {
    auto loc = expect_and_consume(yy::parser::token::tok_interaction).begin;
    auto name = parse_identifier();
    auto functions = parse_braced_function_list();
    return actions_.on_interaction(
        {loc, end_}, std::move(name), std::move(functions));
  }

  // FunctionList:
  //     FunctionList FunctionAnnotated CommaOrSemicolonOptional
  //   | FunctionList Performs CommaOrSemicolon
  //   | /* empty */
  //
  // FunctionAnnotated: StatementAttrs Function Annotations
  //
  // Performs: tok_performs FieldType
  std::unique_ptr<t_function_list> parse_braced_function_list() {
    expect_and_consume('{');
    auto functions = std::make_unique<t_function_list>();
    while (token_.kind != '}') {
      if (token_.kind != yy::parser::token::tok_performs) {
        functions->emplace_back(parse_function());
        parse_comma_or_semicolon_optional();
        continue;
      }
      // Parse performs.
      auto range = token_.range;
      consume_token();
      range.end = token_.range.end;
      auto type = parse_field_type();
      if (!parse_comma_or_semicolon_optional()) {
        report_expected("`,` or `;`");
      }
      functions->emplace_back(actions_.on_performs(range, type));
    }
    expect_and_consume('}');
    return functions;
  }

  // FunctionAnnotated:
  //     StatementAttrs Function Annotations
  //
  // Function:
  //     FunctionQualifier FunctionType Identifier "(" FieldList ")" MaybeThrows
  //
  // FunctionQualifier: tok_oneway | tok_idempotent | tok_readonly | /* empty */
  std::unique_ptr<t_function> parse_function() {
    auto loc = token_.range.begin;
    auto attrs = parse_statement_attrs();

    // Parse a function qualifier.
    auto qual = t_function_qualifier();
    switch (token_.kind) {
      case yy::parser::token::tok_oneway:
        qual = t_function_qualifier::one_way;
        consume_token();
        break;
      case yy::parser::token::tok_idempotent:
        qual = t_function_qualifier::idempotent;
        consume_token();
        break;
      case yy::parser::token::tok_readonly:
        qual = t_function_qualifier::read_only;
        consume_token();
        break;
      default:
        break;
    }

    auto return_type = parse_return_type();
    auto name_loc = token_.range.begin;
    auto name = parse_identifier();

    // Parse arguments.
    expect_and_consume('(');
    auto params = parse_field_list(')');
    expect_and_consume(')');

    auto throws = parse_throws();
    auto annotations = parse_annotations();
    auto end = end_;
    return actions_.on_function(
        {loc, end},
        std::move(attrs),
        qual,
        std::move(return_type),
        name_loc,
        std::move(name),
        std::move(params),
        std::move(throws),
        std::move(annotations));
  }

  // FunctionType:
  //     FunctionTypeElement
  //   | FunctionType "," FunctionTypeElement
  //
  // FunctionTypeElement:
  //     FieldType | StreamReturnType | SinkReturnType | tok_void
  //
  // StreamReturnType: tok_stream "<" FieldType MaybeThrows ">"
  //
  // SinkReturnType: tok_sink "<" SinkFieldType "," SinkFieldType ">"
  std::vector<t_type_ref> parse_return_type() {
    auto return_type = std::vector<t_type_ref>();
    auto parse_type_throws = [this]() -> type_throws_spec {
      auto type = parse_field_type();
      auto throws = parse_throws();
      return {std::move(type), std::move(throws)};
    };
    do {
      auto type = t_type_ref();
      switch (token_.kind) {
        case yy::parser::token::tok_void:
          type = t_base_type::t_void();
          consume_token();
          break;
        case yy::parser::token::tok_stream: {
          consume_token();
          expect_and_consume('<');
          auto response = parse_type_throws();
          expect_and_consume('>');
          type = actions_.on_stream_return_type(std::move(response));
          break;
        }
        case yy::parser::token::tok_sink: {
          consume_token();
          expect_and_consume('<');
          auto sink = parse_type_throws();
          expect_and_consume(',');
          auto final_response = parse_type_throws();
          expect_and_consume('>');
          type = actions_.on_sink_return_type(
              std::move(sink), std::move(final_response));
          break;
        }
        default:
          type = parse_field_type();
          break;
      }
      return_type.push_back(std::move(type));
    } while (try_consume_token(','));
    return return_type;
  }

  // MaybeThrows: tok_throws "(" FieldList ")" | /* empty */
  std::unique_ptr<t_throws> parse_throws() {
    if (!try_consume_token(yy::parser::token::tok_throws)) {
      return {};
    }
    expect_and_consume('(');
    auto exceptions = parse_field_list(')');
    expect_and_consume(')');
    return actions_.on_throws(std::move(exceptions));
  }

  // Typedef: tok_typedef FieldType Identifier
  std::unique_ptr<t_typedef> parse_typedef() {
    auto loc = expect_and_consume(yy::parser::token::tok_typedef).begin;
    auto type = parse_field_type();
    auto name = parse_identifier();
    return actions_.on_typedef({loc, end_}, std::move(type), std::move(name));
  }

  // Struct: tok_struct Identifier "{" FieldList "}"
  std::unique_ptr<t_struct> parse_struct() {
    auto loc = expect_and_consume(yy::parser::token::tok_struct).begin;
    auto name = parse_identifier();
    auto fields = parse_braced_field_list();
    return actions_.on_struct({loc, end_}, std::move(name), std::move(fields));
  }

  // Union: tok_union Identifier "{" FieldList "}"
  std::unique_ptr<t_union> parse_union() {
    auto loc = expect_and_consume(yy::parser::token::tok_union).begin;
    auto name = parse_identifier();
    auto fields = parse_braced_field_list();
    return actions_.on_union({loc, end_}, std::move(name), std::move(fields));
  }

  // Exception: ErrorSafety ErrorKind ErrorBlame
  //    case tok_exception Identifier "{" FieldList "}"
  //
  // ErrorSafety: tok_safe | /* empty */
  // ErrorKind: tok_transient | tok_stateful | tok_permanent | /* empty */
  // ErrorBlame: tok_client | tok_server | /* empty */
  std::unique_ptr<t_exception> parse_exception() {
    auto loc = token_.range.begin;
    auto safety = try_consume_token(yy::parser::token::tok_safe)
        ? t_error_safety::safe
        : t_error_safety::unspecified;
    auto kind = t_error_kind::unspecified;
    switch (token_.kind) {
      case yy::parser::token::tok_transient:
        kind = t_error_kind::transient;
        consume_token();
        break;
      case yy::parser::token::tok_stateful:
        kind = t_error_kind::stateful;
        consume_token();
        break;
      case yy::parser::token::tok_permanent:
        kind = t_error_kind::permanent;
        consume_token();
        break;
      default:
        break;
    }
    auto blame = t_error_blame::unspecified;
    if (try_consume_token(yy::parser::token::tok_client)) {
      blame = t_error_blame::client;
    } else if (try_consume_token(yy::parser::token::tok_server)) {
      blame = t_error_blame::server;
    }
    expect_and_consume(yy::parser::token::tok_exception);
    auto name = parse_identifier();
    auto fields = parse_braced_field_list();
    return actions_.on_exception(
        {loc, end_}, safety, kind, blame, std::move(name), std::move(fields));
  }

  t_field_list parse_braced_field_list() {
    expect_and_consume('{');
    auto fields = parse_field_list('}');
    expect_and_consume('}');
    return fields;
  }

  // FieldList:
  //     FieldList FieldAnnotated CommaOrSemicolonOptional InlineDocOptional
  //   | /* empty */
  t_field_list parse_field_list(token_kind delimiter) {
    auto fields = t_field_list();
    while (token_.kind != delimiter) {
      fields.emplace_back(parse_field());
    }
    return fields;
  }

  // FieldAnnotated: StatementAttrs Field Annotations
  //
  // Field: FieldId FieldQualifier FieldType Identifier FieldValue
  //
  // FieldId: Integer ":" | /* empty */
  //
  // FieldQualifier: tok_required | tok_optional | /* empty */
  //
  // FieldValue: "=" ConstValue | /* empty */
  std::unique_ptr<t_field> parse_field() {
    auto loc = token_.range.begin;
    auto attrs = parse_statement_attrs();

    // Parse the field id.
    auto field_id = boost::optional<int64_t>();
    if (auto integer = try_parse_integer()) {
      field_id = *integer;
      expect_and_consume(':');
    }

    // Parse the field qualifier.
    auto qual = t_field_qualifier();
    if (try_consume_token(yy::parser::token::tok_optional)) {
      qual = t_field_qualifier::optional;
    } else if (try_consume_token(yy::parser::token::tok_required)) {
      qual = t_field_qualifier::required;
    }

    auto type = parse_field_type();
    auto name_loc = token_.range.begin;
    auto name = parse_identifier();

    // Parse the default value.
    auto value = std::unique_ptr<t_const_value>();
    if (try_consume_token('=')) {
      value = parse_const_value();
    }

    auto annotations = parse_annotations();
    auto end = end_;
    parse_comma_or_semicolon_optional();
    auto doc = parse_inline_doc_optional();
    return actions_.on_field(
        {loc, end},
        std::move(attrs),
        field_id,
        qual,
        std::move(type),
        name_loc,
        std::move(name),
        std::move(value),
        std::move(annotations),
        std::move(doc));
  }

  // FieldType:
  //     FieldTypeIdentifier Annotations
  //   | BaseType Annotations
  //   | ContainerType Annotations
  //
  // FieldTypeIdentifier: tok_identifier
  //
  // ContainerType: MapType | SetType | ListType
  //
  // MapType: tok_map "<" FieldType "," FieldType ">"
  // SetType: tok_set "<" FieldType ">"
  // ListType: tok_list "<" FieldType ">"
  //
  // FieldTypeIdentifier is used to disallow context-sensitive keywords as
  // field type identifiers. This avoids an ambuguity in the resolution of the
  // FunctionQualifier FunctionType part of the Function rule, when one of the
  // tok_oneway, tok_idempotent or tok_readonly is encountered. It could either
  // resolve the token as FunctionQualifier or resolve "" as FunctionQualifier
  // and resolve the token as FunctionType.
  t_type_ref parse_field_type() {
    auto range = token_.range;
    if (const t_base_type* type = try_parse_base_type()) {
      return actions_.on_field_type(*type, parse_annotations());
    }
    switch (token_.kind) {
      case yy::parser::token::tok_identifier: {
        auto value = bison_token_->value.as<std::string>();
        consume_token();
        return actions_.on_field_type(
            range, std::move(value), parse_annotations());
      }
      case yy::parser::token::tok_list: {
        consume_token();
        expect_and_consume('<');
        auto element_type = parse_field_type();
        expect_and_consume('>');
        return actions_.on_list_type(
            std::move(element_type), parse_annotations());
      }
      case yy::parser::token::tok_set: {
        consume_token();
        expect_and_consume('<');
        auto key_type = parse_field_type();
        expect_and_consume('>');
        return actions_.on_set_type(std::move(key_type), parse_annotations());
      }
      case yy::parser::token::tok_map: {
        consume_token();
        expect_and_consume('<');
        auto key_type = parse_field_type();
        expect_and_consume(',');
        auto value_type = parse_field_type();
        expect_and_consume('>');
        return actions_.on_map_type(
            std::move(key_type), std::move(value_type), parse_annotations());
      }
      default:
        report_expected("type");
    }
  }

  // BaseType:
  //     tok_string | tok_binary | tok_bool | tok_byte
  //   | tok_i16 | tok_i32| tok_i64 | tok_double | tok_float
  const t_base_type* try_parse_base_type() {
    auto get_base_type = [this]() -> const t_base_type* {
      switch (token_.kind) {
        case yy::parser::token::tok_string:
          return &t_base_type::t_string();
        case yy::parser::token::tok_binary:
          return &t_base_type::t_binary();
        case yy::parser::token::tok_bool:
          return &t_base_type::t_bool();
        case yy::parser::token::tok_byte:
          return &t_base_type::t_byte();
        case yy::parser::token::tok_i16:
          return &t_base_type::t_i16();
        case yy::parser::token::tok_i32:
          return &t_base_type::t_i32();
        case yy::parser::token::tok_i64:
          return &t_base_type::t_i64();
        case yy::parser::token::tok_double:
          return &t_base_type::t_double();
        case yy::parser::token::tok_float:
          return &t_base_type::t_float();
        default:
          return nullptr;
      }
    };
    auto base_type = get_base_type();
    if (base_type) {
      consume_token();
    }
    return base_type;
  }

  // Enum: tok_enum Identifier "{" EnumValueList "}"
  //
  // EnumValueList:
  //     EnumValueList EnumValueAnnotated CommaOrSemicolonOptional
  //       InlineDocOptional
  //   | /* empty */
  std::unique_ptr<t_enum> parse_enum() {
    auto loc = expect_and_consume(yy::parser::token::tok_enum).begin;
    auto name = parse_identifier();
    expect_and_consume('{');
    auto values = t_enum_value_list();
    while (token_.kind != '}') {
      values.emplace_back(parse_enum_value());
    }
    expect_and_consume('}');
    return actions_.on_enum({loc, end_}, std::move(name), std::move(values));
  }

  // EnumValueAnnotated: StatementAttrs EnumValue Annotations
  //
  // EnumValue:
  //     Identifier "=" Integer
  //   | Identifier
  std::unique_ptr<t_enum_value> parse_enum_value() {
    auto range = token_.range;
    auto attrs = parse_statement_attrs();
    auto name_loc = token_.range.begin;
    auto name = parse_identifier();
    auto value = boost::optional<int64_t>();
    if (try_consume_token('=')) {
      value = parse_integer();
      range.end = end_;
    }
    auto annotations = parse_annotations();
    parse_comma_or_semicolon_optional();
    auto doc = parse_inline_doc_optional();
    return actions_.on_enum_value(
        range,
        std::move(attrs),
        name_loc,
        std::move(name),
        value ? &*value : nullptr,
        std::move(annotations),
        std::move(doc));
  }

  // Const: tok_const FieldType Identifier "=" ConstValue
  std::unique_ptr<t_const> parse_const() {
    auto loc = expect_and_consume(yy::parser::token::tok_const).begin;
    auto type = parse_field_type();
    auto name = parse_identifier();
    auto end = end_;
    expect_and_consume('=');
    auto value = parse_const_value();
    return actions_.on_const(
        {loc, end}, type, std::move(name), std::move(value));
  }

  // ConstValue:
  //     tok_bool_constant | Integer | Double | tok_literal
  //   | Identifier | ConstList | ConstMap | ConstStruct
  std::unique_ptr<t_const_value> parse_const_value() {
    auto loc = token_.range.begin;
    auto s = sign::plus;
    switch (static_cast<int>(token_.kind)) {
      case yy::parser::token::tok_bool_constant: {
        auto value = bison_token_->value.as<bool>();
        consume_token();
        return actions_.on_bool_const(value);
      }
      case '-':
        s = sign::minus;
        FMT_FALLTHROUGH;
      case '+':
        consume_token();
        if (token_.kind == yy::parser::token::tok_int_constant) {
          return actions_.on_int_const(loc, parse_integer(s));
        } else if (token_.kind == yy::parser::token::tok_dub_constant) {
          return actions_.on_double_const(parse_double(s));
        }
        report_expected("number");
        break;
      case yy::parser::token::tok_int_constant:
        return actions_.on_int_const(loc, parse_integer());
      case yy::parser::token::tok_dub_constant:
        return actions_.on_double_const(parse_double());
      case yy::parser::token::tok_literal: {
        auto value = bison_token_->value.as<std::string>();
        consume_token();
        return actions_.on_string_literal(value);
      }
      case '[':
        return parse_const_list();
      case '{':
        return parse_const_map();
      default:
        if (auto id = try_parse_identifier()) {
          return token_.kind == '{'
              ? parse_const_struct_body({loc, end_}, *id)
              : actions_.on_reference_const(std::move(*id));
        }
        break;
    }
    report_expected("constant");
  }

  // ConstList:
  //     "[" ConstListContents CommaOrSemicolonOptional "]"
  //   | "[" "]"
  //
  // ConstListContents:
  //     ConstListContents CommaOrSemicolon ConstValue
  //   | ConstValue
  std::unique_ptr<t_const_value> parse_const_list() {
    expect_and_consume('[');
    auto list = actions_.on_const_list();
    while (token_.kind != ']') {
      list->add_list(parse_const_value());
      if (!parse_comma_or_semicolon_optional()) {
        break;
      }
    }
    expect_and_consume(']');
    return list;
  }

  // ConstMap:
  //     "{" ConstMapContents CommaOrSemicolonOptional "}"
  //   | "{" "}"
  //
  // ConstMapContents:
  //     ConstMapContents CommaOrSemicolon ConstValue ":" ConstValue
  //   | ConstValue ":" ConstValue
  std::unique_ptr<t_const_value> parse_const_map() {
    expect_and_consume('{');
    auto map = actions_.on_const_map();
    while (token_.kind != '}') {
      auto key = parse_const_value();
      expect_and_consume(':');
      auto value = parse_const_value();
      map->add_map(std::move(key), std::move(value));
      if (!parse_comma_or_semicolon_optional()) {
        break;
      }
    }
    expect_and_consume('}');
    return map;
  }

  // ConstStruct:
  //     ConstStructType "{" ConstStructContents CommaOrSemicolonOptional "}"
  //   | ConstStructType "{" "}"
  //
  // ConstStructType: Identifier
  //
  // ConstStructContents:
  //     ConstStructContents CommaOrSemicolon Identifier "=" ConstValue
  //   | Identifier "=" ConstValue
  std::unique_ptr<t_const_value> parse_const_struct_body(
      source_range range, std::string id) {
    expect_and_consume('{');
    auto map = actions_.on_const_struct(range, std::move(id));
    while (token_.kind != '}') {
      auto key = actions_.on_string_literal(parse_identifier());
      expect_and_consume('=');
      auto value = parse_const_value();
      map->add_map(std::move(key), std::move(value));
      if (!parse_comma_or_semicolon_optional()) {
        break;
      }
    }
    expect_and_consume('}');
    return map;
  }

  // Integer:
  //     tok_int_constant
  //   | tok_char_plus tok_int_constant
  //   | tok_char_minus tok_int_constant
  boost::optional<int64_t> try_parse_integer(sign s = sign::plus) {
    switch (static_cast<int>(token_.kind)) {
      case '-':
        s = sign::minus;
        FMT_FALLTHROUGH;
      case '+':
        consume_token();
        if (token_.kind != yy::parser::token::tok_int_constant) {
          report_expected("integer");
        }
        FMT_FALLTHROUGH;
      case yy::parser::token::tok_int_constant: {
        auto value = bison_token_->value.as<uint64_t>();
        consume_token();
        return actions_.on_integer(s, value);
      }
    }
    return {};
  }

  int64_t parse_integer(sign s = sign::plus) {
    if (auto result = try_parse_integer(s)) {
      return *result;
    }
    report_expected("integer");
  }

  // Double:
  //     tok_dub_constant
  //   | tok_char_plus tok_dub_constant
  //   | tok_char_minus tok_dub_constant
  double parse_double(sign s = sign::plus) {
    switch (static_cast<int>(token_.kind)) {
      case '-':
        s = sign::minus;
        FMT_FALLTHROUGH;
      case '+':
        consume_token();
        if (token_.kind != yy::parser::token::tok_dub_constant) {
          break;
        }
        FMT_FALLTHROUGH;
      case yy::parser::token::tok_dub_constant: {
        double value = bison_token_->value.as<double>();
        consume_token();
        return s == sign::plus ? value : -value;
      }
    }
    report_expected("double");
  }

  // Identifier:
  //     FieldTypeIdentifier
  //   | tok_package
  //   | tok_sink
  //   | tok_oneway
  //   | tok_readonly
  //   | tok_idempotent
  //   | tok_safe
  //   | tok_transient
  //   | tok_stateful
  //   | tok_permanent
  //   | tok_server
  //   | tok_client
  boost::optional<std::string> try_parse_identifier() {
    auto id = std::string();
    switch (token_.kind) {
      case yy::parser::token::tok_identifier:
        id = bison_token_->value.as<std::string>();
        break;
      // Context-sensitive keywords allowed in identifiers:
      case yy::parser::token::tok_package:
      case yy::parser::token::tok_sink:
      case yy::parser::token::tok_oneway:
      case yy::parser::token::tok_readonly:
      case yy::parser::token::tok_idempotent:
      case yy::parser::token::tok_safe:
      case yy::parser::token::tok_transient:
      case yy::parser::token::tok_stateful:
      case yy::parser::token::tok_permanent:
      case yy::parser::token::tok_server:
      case yy::parser::token::tok_client:
        id = to_string(token_.kind);
        break;
      default:
        return {};
    }
    consume_token();
    return id;
  }

  std::string parse_identifier() {
    if (auto id = try_parse_identifier()) {
      return *id;
    }
    report_expected("identifier");
  }

  // CommaOrSemicolonOptional: CommaOrSemicolon | /* empty */
  // CommaOrSemicolon: ","  | ";"
  bool parse_comma_or_semicolon_optional() {
    return try_consume_token(',') || try_consume_token(';');
  }

 public:
  parser(lexer& lex, parser_actions& actions, diagnostics_engine& diags)
      : lexer_(lex), actions_(actions), diags_(diags) {}

  bool parse() { return parse_program(); }
};

} // namespace

bool parse(lexer& lex, parser_actions& actions, diagnostics_engine& diags) {
  return parser(lex, actions, diags).parse();
}

} // namespace compiler
} // namespace thrift
} // namespace apache
