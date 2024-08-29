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

#include <thrift/compiler/whisker/object.h>

#include <cstdint>
#include <deque>
#include <optional>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>

namespace whisker {

/**
 * An eval_context is responsible for name resolution within Whisker templates
 * and maintains necessary state to achieve when a template is rendered (aka
 * evaluated).
 *
 * The eval_context is responsible for:
 *   - Maintaining the "stack" of lexical scopes required for name resolution.
 *   - Resolving names (array of identifier strings, e.g. variable-lookup) to
 *     whisker::object instances.
 *
 * The top of the stack is the "current" scope — this is where name resolution
 * begins before falling back to parent scopes. See eval_context::lexical_scope
 * for more details.
 *
 * The eval_context is also responsible for binding local names to objects in
 * the current scope (analogous to local variables in programming languages).
 * See eval_context::bind_local().
 */
class eval_context {
 public:
  explicit eval_context(const object& root_scope);
  /* implicit */ eval_context(object&&) = delete;
  ~eval_context() noexcept;

  /**
   * Returns the number of frames (lexical_scopes) currently in the stack. This
   * is guaranteed to be greater than 0.
   *
   * The stack depth changes with push_scope() and pop_scope() calls.
   */
  std::size_t stack_depth() const;

  /**
   * Pushes a new lexical scope onto the top of the stack backed by the provided
   * object. This scope becomes the current scope.
   *
   * Calling push_scope() increases the stack depth by 1.
   */
  void push_scope(const object& object);

  /**
   * Pops the top-most lexical scope from the stack. The previous scope becomes
   * the current scope.
   *
   * Preconditions:
   *   - The stack depth is greater than 1.
   */
  void pop_scope();

  /**
   * Binds a name to an object. This binding is local to the current scope,
   * which means that pop_scope() will unbound this name. Locals shadow names in
   * the current scope as well as parent scopes.
   *
   * Throws:
   *   - eval_name_already_bound_error if the name already exists as a local in
   *     the current scope.
   */
  void bind_local(std::string name, object value);

  /**
   * Performs a lexical search for an object by name (chain of properties)
   * within the current scope (top of the stack).
   *
   * If the provided path is empty, the object backing the current scope is
   * returned. That is, the "{{.}}" object. This is infallible.
   *
   * Preconditions:
   *   - The provided path is a series of valid Whisker identifier
   *
   * Throws:
   *   - eval_scope_lookup_error if the first identifier of the path is not
   *     found in the current scope.
   *   - eval_property_lookup_error if any subsequent identifier is not found in
   *     the chain of resolved whisker::objects.
   */
  const object& lookup_object(const std::vector<std::string>& path);

 private:
  /**
   * A lexical scope which determines how name lookups are performed within the
   * Whisker templating language.
   *   https://en.wikipedia.org/wiki/Scope_(computer_science)#Lexical_scope
   *
   * In Whisker, the program has an initial lexical scope (called the root
   * scope). Block elements (such as section blocks) can introduce child scopes.
   *
   * Child scopes implicitly inherit the parent scope's bindings, but can shadow
   * these inherited names in case of collision. This class, however, represents
   * a single lexical scope with no reference to parent scopes. The actual
   * lexical lookup algorithm is implemented in eval_context (which stores a
   * stack of lexical_scope).
   *
   * In Whisker, all lexical scopes are backed by whisker::object. However, only
   * whisker::map and whisker::native_object have enumerable property names.
   * Other types only allow the self-referential {{.}} lookup.
   *
   * Lexical scopes can also have "locals" — these are names bound to the
   * current scope independent of the backing whisker::object. These bindings
   * are analogous to local variables in programming languages. Local bindings
   * shadow names from the backing whisker::object.
   */
  class lexical_scope {
   public:
    explicit lexical_scope(const object& this_ref) : this_ref_(this_ref) {}

    // The {{.}} object is always available in the current scope (or else the
    // scope couldn't exist).
    const object& this_ref() const { return this_ref_; }

    /**
     * Looks up a properties in the following order:
     *   1. Locals
     *   2. Backing object (this_ref())
     */
    const object* lookup_property(std::string_view identifier);

    // Before C++20, std::unordered_map does not support heterogenous lookups
    using locals_map = std::map<std::string, object, std::less<>>;
    locals_map& locals() noexcept { return locals_; }
    const locals_map& locals() const noexcept { return locals_; }

   private:
    object this_ref_;
    locals_map locals_;
  };

  // We're using a deque because we want to maintain reference stability when
  // push_scope() / pop_scope() are called. This is because there may be
  // manaed_object's passed around with references into those scope objects.
  std::deque<lexical_scope> stack_;
};

/**
 * Base class for all exceptions thrown by the Whisker evaluation engine.
 */
class eval_error : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

/**
 * Error thrown when a name spelled in a template is not found in the current
 * eval_context's lexical scope.
 *
 * This error only applies to top-level identifiers. That is, if there is a
 * lookup for `foo.bar.abc`, then this error is only thrown if `foo` is not
 * found in the eval_context. The reason is that after the initial lookup in the
 * language scope, name resolution no longer pertains to the templating language
 * scope — it's a matter of the Whisker object model afterwards
 * (whisker::object).
 */
class eval_scope_lookup_error : public eval_error {
 public:
  explicit eval_scope_lookup_error(
      std::string property_name, std::vector<object> searched_scopes);

  /**
   * The name of the property that was missing.
   */
  const std::string& property_name() const { return property_name_; }
  /**
   * The stack of objects that were searched for the property from the current
   * eval_context.
   *
   * The order of objects on the stack is the order of search (most to least
   * local).
   */
  const std::vector<object>& searched_scopes() const {
    return searched_scopes_;
  }

 private:
  std::string property_name_;
  std::vector<object> searched_scopes_;
};

/**
 * Error thrown when a property name on a whisker::object is missing.
 *
 * This error is thrown when a non-top-level property is accessed. That is, if
 * there is a lookup for `foo.bar.abc`, then this error is only thrown if `bar`
 * is not found on the object `foo`, or `abc` is not found on the object
 * `foo.bar`.
 */
class eval_property_lookup_error : public eval_error {
 public:
  explicit eval_property_lookup_error(
      object missing_from,
      std::vector<std::string> success_path,
      std::string property_name);

  /**
   * The object on which the property named by property_name() was missing.
   */
  const object& missing_from() const { return missing_from_; }
  /**
   * The path of property lookups that succeeded before the failure.
   */
  const std::vector<std::string>& success_path() const { return success_path_; }
  /**
   * The name of the property that was missing.
   */
  const std::string& property_name() const { return property_name_; }

 private:
  object missing_from_;
  std::vector<std::string> success_path_;
  std::string property_name_;
};

/**
 * Error thrown when a name is bound more than once in the same scope.
 *
 * While names can shadow each other in the same eval_context, the shadowed
 * names must be in a different lexical scope.
 */
class eval_name_already_bound_error : public eval_error {
 public:
  explicit eval_name_already_bound_error(std::string name);

  const std::string& name() const { return name_; }

 private:
  std::string name_;
};

} // namespace whisker