/*
 * Copyright (c) Facebook, Inc. and its affiliates.
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

#include <string>
#include <vector>

#include <thrift/compiler/ast/visitor.h>
#include <thrift/compiler/validator/diagnostic.h>

#include <boost/optional.hpp>

namespace apache {
namespace thrift {
namespace compiler {

class validator : virtual public visitor {
 public:
  using diagnostics_t = std::vector<diagnostic>;
  static diagnostics_t validate(t_program* program);

  using visitor::visit;

  // must call set_program if overriding this overload
  bool visit(t_program* program) override;

  void set_program(t_program* const program);

 protected:
  void add_error(boost::optional<int> const lineno, std::string const& message);

 private:
  template <typename T, typename... Args>
  friend std::unique_ptr<T> make_validator(diagnostics_t&, Args&&...);

  void set_ref_diagnostics(diagnostics_t& diagnostics);

  diagnostics_t* diagnostics_{};
  t_program const* program_{};
};

template <typename T, typename... Args>
std::unique_ptr<T> make_validator(
    validator::diagnostics_t& diagnostics,
    Args&&... args) {
  auto ptr = std::unique_ptr<T>(new T(std::forward<Args>(args)...));
  ptr->set_ref_diagnostics(diagnostics);
  return ptr;
}

template <typename T, typename... Args>
validator::diagnostics_t run_validator(
    t_program* const program,
    Args&&... args) {
  validator::diagnostics_t diagnostics;
  make_validator<T>(diagnostics, std::forward<Args>(args)...)
      ->traverse(program);
  return diagnostics;
}

class validator_list {
 public:
  explicit validator_list(validator::diagnostics_t& diagnostics)
      : diagnostics_(diagnostics) {}

  template <typename T, typename... Args>
  void add(Args&&... args) {
    auto ptr = make_validator<T>(diagnostics_, std::forward<Args>(args)...);
    validators_.push_back(std::move(ptr));
  }

  void traverse(t_program* const program);

 private:
  validator::diagnostics_t& diagnostics_;
  std::vector<std::unique_ptr<validator>> validators_;
};

class service_method_name_uniqueness_validator : virtual public validator {
 public:
  using validator::visit;

  /**
   * Enforces that there are no duplicate method names either within this
   * service or between this service and any of its ancestors.
   */
  bool visit(t_service* service) override;

 private:
  /**
   * Error messages formatters
   */
  void add_error_service_method_names(
      int lineno,
      std::string const& service_name_new,
      std::string const& service_name_old,
      std::string const& function_name);

  void validate_service_method_names_unique(t_service const* service);
};

class enum_value_names_uniqueness_validator : virtual public validator {
 public:
  using validator::visit;

  // Enforces that there are not duplicated enum value names
  bool visit(t_enum* tenum) override;

 private:
  void validate(t_enum const* tenum);

  void add_validation_error(
      int const lineno,
      std::string const& value_name,
      std::string const& enum_name);
};

class enum_values_uniqueness_validator : virtual public validator {
 public:
  using validator::visit;

  // Enforces that there are not duplicated enum values
  bool visit(t_enum* tenum) override;

 private:
  void validate(t_enum const* tenum);

  void add_validation_error(
      int const lineno,
      t_enum_value const& enum_value,
      std::string const& existing_value_name,
      std::string const& enum_name);
};

class enum_values_set_validator : virtual public validator {
 public:
  using validator::visit;

  // Enforces that every enum value has an explicit value
  bool visit(t_enum* tenum) override;

 private:
  void validate(t_enum const* tenum);

  void add_validation_error(
      int const lineno,
      std::string const& enum_value,
      std::string const& enum_name);
};

class exception_list_is_all_exceptions_validator : virtual public validator {
 public:
  using validator::visit;

  bool visit(t_service* service) override;

 private:
  /**
   * Check members of a throws block
   */
  static bool validate_throws(t_struct* throws);
};

class union_no_qualified_fields_validator : virtual public validator {
 public:
  using validator::visit;

  /**
   * Enforces that there are no qualified fields in a union.
   */
  bool visit(t_struct* s) override;
};

class mixin_type_correctness_validator : virtual public validator {
 public:
  using validator::visit;

  /**
   * Enforces that all mixin fields are struct.
   */
  bool visit(t_struct* s) override;
};

class field_names_uniqueness_validator : virtual public validator {
 public:
  using validator::visit;

  /**
   * Enforces that there are no duplicate field names either within this
   * struct or between this struct and any of its mixins.
   */
  bool visit(t_struct* s) override;
};

class struct_names_uniqueness_validator : virtual public validator {
 public:
  using validator::visit;

  /**
   * Enforces that there are no duplicate names between structs and exceptions
   */
  bool visit(t_program* s) override;
};

class structured_annotations_validator : virtual public validator {
 public:
  using validator::visit;

  bool visit(t_service* service) override;
  bool visit(t_enum* tenum) override;
  bool visit(t_struct* tstruct) override;
  bool visit(t_field* tfield) override;

 protected:
  virtual void validate_annotations(
      t_annotated* tannotated,
      const std::string& tannotated_name) = 0;
};

class structured_annotations_uniqueness_validator
    : virtual public structured_annotations_validator {
 protected:
  void validate_annotations(
      t_annotated* tannotated,
      const std::string& tannotated_name) override;
};

} // namespace compiler
} // namespace thrift
} // namespace apache
