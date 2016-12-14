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

  validator() : visitor() {}

  std::vector<std::string>&& get_errors() &&;

  bool visit(t_program const* program) override;
  bool visit(t_service const* service) override;

 private:
  void add_error(std::string error);

  /**
   * Error messages formatters
   */
  void add_error_service_method_names(
    const std::string& file_path,
    const int lineno,
    const std::string& service_name_new,
    const std::string& service_name_old,
    const std::string& function_name);

  /**
   * Enforces that there are no duplicate method names either within this
   * service or between this service and any of its ancestors.
   */
  void validate_service_method_names_unique(t_service const* service);

  t_program const* program_;
  std::vector<std::string> errors_;
};

}}}
