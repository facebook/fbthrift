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

#pragma once

#include <thrift/compiler/whisker/source_location.h>

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <variant>
#include <vector>

namespace whisker::ast {

struct text;
struct newline;
struct comment;
struct section_block;
struct conditional_block;
struct with_block;
struct partial_apply;
struct interpolation;
struct let_statement;
struct pragma_statement;

/**
 * The top-level types of constructs allowed in a Whisker source file.
 */
using body = std::variant<
    text,
    newline,
    comment,
    interpolation,
    section_block,
    conditional_block,
    with_block,
    let_statement,
    pragma_statement,
    partial_apply>;
using bodies = std::vector<body>;

// Defines operator!= in terms of operator==
// Remove in C++20 which introduces comparison operator synthesis
#define WHISKER_DEFINE_OPERATOR_INEQUALITY(type)             \
  friend bool operator!=(const type& lhs, const type& rhs) { \
    return !(lhs == rhs);                                    \
  }

/**
 * The root node of a Whisker AST representing a source file.
 */
struct root {
  source_location loc;
  bodies body_elements;
};

/**
 * Raw text content that should be emitted unchanged in the rendered output.
 * This is guaranteed to be on one line (i.e. not contain a new line).
 */
struct text {
  source_range loc;
  std::string content;
};

/**
 * Raw newline that should be emitted unchanged in the rendered output. One of:
 *   - "\r\n"
 *   - "\n"
 *   - "\r"
 */
struct newline {
  source_range loc;
  std::string text;
};

/**
 * A comment that should be omitted in the rendered output.
 *   https://mustache.github.io/mustache.5.html#Comments
 * Whisker also supports Handlebars' escaped comments:
 *   https://handlebarsjs.com/guide/#template-comments
 */
struct comment {
  source_range loc;
  std::string text;
};

/**
 * A valid Whisker identifier. See whisker::lexer for its definition.
 */
struct identifier {
  source_range loc;
  std::string name;

  /**
   * Determines if two identifiers are syntactically equivalent, excluding
   * their location in source code.
   */
  friend bool operator==(const identifier& lhs, const identifier& rhs) {
    return lhs.name == rhs.name;
  }
  // Remove in C++20 which introduces comparison operator synthesis
  WHISKER_DEFINE_OPERATOR_INEQUALITY(identifier)
};

/**
 * A "path" of identifiers that represent a lookup of a variable where each
 * chain component is separated by a dot. This is a subset of Mustache's
 * variables:
 *   https://mustache.github.io/mustache.5.html#Variables
 */
struct variable_lookup {
  source_range loc;
  // this_ref is a special case: {{.}} referring to the current object.
  struct this_ref {
    // Remove in C++20 which introduces comparison operator synthesis
    friend bool operator==(const this_ref&, const this_ref&) { return true; }
    WHISKER_DEFINE_OPERATOR_INEQUALITY(this_ref)
  };
  std::variant<this_ref, std::vector<identifier>> chain;

  /**
   * Determines if two variable lookups are syntactically equivalent, excluding
   * their location in source code.
   */
  friend bool operator==(
      const variable_lookup& lhs, const variable_lookup& rhs) {
    return lhs.chain == rhs.chain;
  }
  // Remove in C++20 which introduces comparison operator synthesis
  WHISKER_DEFINE_OPERATOR_INEQUALITY(variable_lookup)

  std::string chain_string() const;
};

/**
 * The base value type in Whisker, which can be used in interpolation or as
 * arguments to blocks such as conditionals.
 */
struct expression {
  source_range loc;
  struct function_call {
    /**
     * Base class for all built-in functions.
     */
    struct builtin {};
    /**
     * Binary functions which are also associative and can be "chained", such as
     * `(and arg1 ... argN)`, `(or arg1 ... argN)` etc.
     */
    struct builtin_binary_associative {};

    /**
     * The `(not arg1)` function.
     */
    struct builtin_not : builtin {
      // Remove in C++20 which introduces comparison operator synthesis
      friend bool operator==(const builtin_not&, const builtin_not&) {
        return true;
      }
      WHISKER_DEFINE_OPERATOR_INEQUALITY(builtin_not)
    };

    /**
     * The `(and arg1 ... argN)` function.
     */
    struct builtin_and : builtin, builtin_binary_associative {
      // Remove in C++20 which introduces comparison operator synthesis
      friend bool operator==(const builtin_and&, const builtin_and&) {
        return true;
      }
      WHISKER_DEFINE_OPERATOR_INEQUALITY(builtin_and)
    };

    /**
     * The `(or arg1 ... argN)` function.
     */
    struct builtin_or : builtin, builtin_binary_associative {
      // Remove in C++20 which introduces comparison operator synthesis
      friend bool operator==(const builtin_or&, const builtin_or&) {
        return true;
      }
      WHISKER_DEFINE_OPERATOR_INEQUALITY(builtin_or)
    };

    /**
     * A user-defined function call whose name is variable (chain of
     * identifiers).
     *
     * Example:
     *   `(my_lib.snake_case "FooBar")` // "foo_bar"
     */
    struct user_defined {
      variable_lookup name;

      friend bool operator==(const user_defined& lhs, const user_defined& rhs) {
        return lhs.name == rhs.name;
      }
      // Remove in C++20 which introduces comparison operator synthesis
      WHISKER_DEFINE_OPERATOR_INEQUALITY(user_defined)
    };

    std::variant<builtin_not, builtin_and, builtin_or, user_defined> which;

    /**
     * Unnamed arguments that are identified by their ordering in the function
     * invocation.
     */
    std::vector<expression> positional_arguments;

    struct named_argument {
      identifier name;
      // Using the heap to avoid mutually recursion with `expression`.
      // Using std::shared_ptr so that this struct is copyable.
      std::shared_ptr<expression> value;

      friend bool operator==(
          const named_argument& lhs, const named_argument& rhs) {
        static const auto as_tuple = [](const named_argument& arg) {
          return std::tie(arg.name, *arg.value);
        };
        return as_tuple(lhs) == as_tuple(rhs);
      }
      // Remove in C++20 which introduces comparison operator synthesis
      WHISKER_DEFINE_OPERATOR_INEQUALITY(named_argument)
    };
    /**
     * Named arguments that are identified by their name, with no restrictions
     * on their ordering. Every argument must have a unique identifier. All
     * named arguments must appear after all positional arguments.
     *
     * Using std::map for stable ordering when printing the AST.
     */
    std::map<std::string_view, named_argument> named_arguments;

    /**
     * The name of the function call lookup as seen in the source code.
     */
    std::string name() const;

    friend bool operator==(const function_call& lhs, const function_call& rhs) {
      static const auto as_tuple = [](const function_call& f) {
        // Ignore the source range of the function call.
        return std::tie(f.which, f.positional_arguments, f.named_arguments);
      };
      return as_tuple(lhs) == as_tuple(rhs);
    }
    // Remove in C++20 which introduces comparison operator synthesis
    WHISKER_DEFINE_OPERATOR_INEQUALITY(function_call)
  };
  std::variant<variable_lookup, function_call> which;

  /**
   * Determines if two expressions are syntactically equivalent, excluding their
   * location in source code.
   */
  friend bool operator==(const expression& lhs, const expression& rhs) {
    return lhs.which == rhs.which;
  }
  // Remove in C++20 which introduces comparison operator synthesis
  WHISKER_DEFINE_OPERATOR_INEQUALITY(expression)

  /**
   * Returns a human-readable text representation of the expression.
   */
  std::string to_string() const;
};

/**
 * A top-level use of an expression within a template body. It is similar to
 * expression except its source_range includes the surrounding "{{ }}".
 */
struct interpolation {
  source_range loc;
  expression content;

  std::string to_string() const { return content.to_string(); }
};

/**
 * A Whisker construct for binding an expression to a name. The expression is
 * evaluated exactly once.
 */
struct let_statement {
  source_range loc;

  identifier id;
  expression value;
};

/**
 * A Whisker construct for changing rendering behavior.
 */
struct pragma_statement {
  enum class pragmas {
    single_line,
  };
  source_range loc;

  pragmas pragma;

  std::string_view to_string() const;
};

/**
 * A Whisker construct for conditionals and/or iteration. This matches Mustache:
 *   https://mustache.github.io/mustache.5.html#Sections
 */
struct section_block {
  source_range loc;
  /**
   * {{# ⇒ inverted == false
   * {{^ ⇒ inverted == true
   */
  bool inverted;
  variable_lookup variable;
  bodies body_elements;
};

/**
 * A Whisker construct for conditionals, i.e. the if-block.
 * This matches Handlebars:
 *   https://handlebarsjs.com/guide/builtin-helpers.html#if
 */
struct conditional_block {
  source_range loc;

  expression condition;
  bodies body_elements;

  // The {{#else}} clause, if present.
  struct else_block {
    source_range loc;
    bodies body_elements;
  };
  std::optional<else_block> else_clause;
};

/**
 * A Whisker construct for "de-structuring" a map-like object.
 * This matches Handlebars:
 *   https://handlebarsjs.com/guide/builtin-helpers.html#with
 */
struct with_block {
  source_range loc;

  expression value;
  bodies body_elements;
};

/*
 * A valid Whisker path component for partial application. See whisker::lexer
 * for its definition.
 */
struct path_component {
  source_range loc;
  std::string value;
};

/**
 * A '/' delimited series of path components representing a POSIX portable file
 * path. This is used for partial applications.
 */
struct partial_lookup {
  source_range loc;
  std::vector<path_component> parts;

  std::string as_string() const;
};

/**
 * A Whisker construct for partially applied templates. This matches Mustache:
 *   https://mustache.github.io/mustache.5.html#Partials
 */
struct partial_apply {
  source_range loc;
  partial_lookup path;
  /**
   * Standalone partial applications exhibit different indentation behavior:
   *   https://github.com/mustache/spec/blob/v1.4.2/specs/partials.yml#L13-L15
   *
   * If this is a standalone partial application, the value is the preceeding
   * whitespace necessary before the partial application interpolation.
   * Otherwise, this is std::nullopt.
   *
   * The contained string is guaranteed to be whitespace only.
   */
  std::optional<std::string> standalone_offset_within_line;

  std::string path_string() const;
};

#undef WHISKER_DEFINE_OPERATOR_INEQUALITY

} // namespace whisker::ast
