/*
 * Copyright 2016 Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
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

#include <thrift/compiler/visitor.h>

namespace apache { namespace thrift { namespace compiler {

class validator : virtual public visitor {
 public:
  static std::vector<std::string> validate(t_program const* program);

  using visitor::visit;

  bool visit(t_program const* program) override;

 protected:
  void add_error(int lineno, std::string const& message);

 private:
  std::vector<std::string>* errors_{};
  t_program const* program_{};
};

class service_method_name_uniqueness_validator : virtual public validator {
 public:
  using validator::visit;

  /**
   * Enforces that there are no duplicate method names either within this
   * service or between this service and any of its ancestors.
   */
  bool visit(t_service const* service) override;

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

  // Enforces that there are not duplicated enum values
  bool visit(t_enum const* tenum) override;

 private:
  void validate(t_enum const* tenum);

  void add_validation_error(
      int lineno,
      std::string const& value_name,
      std::string const& enum_name);
};
}}}
