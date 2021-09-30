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

#include <thrift/compiler/validator/validator.h>

#include <unordered_map>
#include <unordered_set>

namespace apache {
namespace thrift {
namespace compiler {

static void fill_validators(validator_list& vs);

void validator_list::traverse(t_program* const program) {
  auto pointers = std::vector<visitor*>{};
  for (auto const& v : validators_) {
    pointers.push_back(v.get());
  }
  interleaved_visitor(pointers).traverse(program);
}

validator::diagnostics_t validator::validate(t_program* const program) {
  auto diagnostics = validator::diagnostics_t{};

  auto validators = validator_list(diagnostics);
  fill_validators(validators);

  validators.traverse(program);

  return diagnostics;
}

void validator::add_error(
    boost::optional<int> const lineno, std::string const& message) {
  diagnostics_->emplace_back(
      diagnostic_level::failure, message, program_->path(), lineno.value_or(0));
}

void validator::set_program(t_program* const program) {
  program_ = program;
}

bool validator::visit(t_program* const program) {
  program_ = program;
  return true;
}

void validator::set_ref_diagnostics(diagnostics_t& diagnostics) {
  diagnostics_ = &diagnostics;
}

/**
 * fill_validators - the validator registry
 *
 * This is where all concrete validator types must be registered.
 */

static void fill_validators(validator_list& vs) {
  vs.add<struct_names_uniqueness_validator>();
  vs.add<interactions_validator>();
}

bool struct_names_uniqueness_validator::visit(t_program* p) {
  set_program(p);
  std::unordered_set<std::string> seen;
  for (auto* object : p->objects()) {
    if (!seen.emplace(object->name()).second) {
      add_error(
          object->get_lineno(),
          "Redefinition of type `" + object->name() + "`.");
    }
  }
  for (auto* interaction : p->interactions()) {
    if (!seen.emplace(interaction->name()).second) {
      add_error(
          interaction->get_lineno(),
          "Redefinition of type `" + interaction->name() + "`.");
    }
  }
  return true;
}

bool interactions_validator::visit(t_program* p) {
  set_program(p);
  for (auto* interaction : p->interactions()) {
    for (auto* func : interaction->get_functions()) {
      auto ret = func->get_returntype();
      if (ret->is_service() &&
          static_cast<const t_service*>(ret)->is_interaction()) {
        add_error(func->get_lineno(), "Nested interactions are forbidden.");
      }
    }
  }
  return true;
}

bool interactions_validator::visit(t_service* s) {
  std::unordered_set<std::string> seen;
  for (auto* func : s->get_functions()) {
    auto ret = func->get_returntype();
    if (func->is_interaction_constructor()) {
      if (!ret->is_service() ||
          !static_cast<const t_service*>(ret)->is_interaction()) {
        add_error(func->get_lineno(), "Only interactions can be performed.");
        continue;
      }
    }

    if (!func->is_interaction_constructor()) {
      if (ret->is_service()) {
        add_error(func->get_lineno(), "Functions cannot return interactions.");
      }
      continue;
    }

    if (!seen.emplace(ret->name()).second) {
      add_error(
          func->get_lineno(),
          "Service `" + s->name() +
              "` has multiple methods for creating interaction `" +
              ret->name() + "`.");
    }
  }
  return true;
}

} // namespace compiler
} // namespace thrift
} // namespace apache
