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

#include <thrift/compiler/validator.h>

#include <unordered_map>
#include <unordered_set>

#include <folly/Format.h>
#include <folly/MapUtil.h>
#include <folly/Range.h>

namespace apache { namespace thrift { namespace compiler {

std::vector<std::string> validator::validate(t_program const* const program) {
  validator _;
  _.traverse(program);
  return std::move(_).get_errors();
}

std::vector<std::string>&& validator::get_errors() && {
  return std::move(errors_);
}

void validator::add_error(std::string error) {
  errors_.push_back(std::move(error));
}

bool validator::visit(t_program const* const program) {
  program_ = program;
  return true;
}

bool validator::visit(t_service const* const service) {
  validate_service_method_names_unique(service);
  return true;
}

void validator::validate_service_method_names_unique(t_service const* const s) {
  // Check for a redefinition of a function in a base service.
  std::unordered_map<
    folly::StringPiece, t_service const*, folly::hasher<folly::StringPiece>>
    base_function_names;
  for (auto b = s->get_extends(); b != nullptr; b = b->get_extends()) {
    for (const auto& f : b->get_functions()) {
      base_function_names[f->get_name()] = b;
    }
  }
  for (const auto f : s->get_functions()) {
    auto b = folly::get_default(base_function_names, f->get_name());
    if (b != nullptr) {
      add_error(folly::sformat(
          "[FAILURE:{}:{}] Function {}.{} redefines {}.{}",
            program_->get_path(),
            f->get_lineno(),
            s->get_name(),
            f->get_name(),
            b->get_full_name(),
            f->get_name()));
    }
  }
  // Check for a redefinition of a function in the same service.
  std::unordered_set<folly::StringPiece, folly::hasher<folly::StringPiece>>
    function_names;
  for (auto f : s->get_functions()) {
    if (function_names.count(f->get_name())) {
      add_error(folly::sformat(
            "[FAILURE:{}:{}] Function {}.{} redefines {}.{}",
            program_->get_path(),
            f->get_lineno(),
            s->get_name(),
            f->get_name(),
            s->get_name(),
            f->get_name()));
    }
    function_names.insert(f->get_name());
  }
}

}}}
