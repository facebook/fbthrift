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
  vs.add<field_names_uniqueness_validator>();
  vs.add<struct_names_uniqueness_validator>();
  vs.add<reserved_field_id_validator>();
  vs.add<recursive_union_validator>();
  vs.add<recursive_ref_validator>();
  vs.add<recursive_optional_validator>();

  // add more validators here ...
}

bool field_names_uniqueness_validator::visit(t_struct* s) {
  // If field is not struct, we couldn't extract field from mixin
  for (auto* field : s->fields()) {
    if (cpp2::is_mixin(*field) &&
        !field->get_type()->get_true_type()->is_struct()) {
      return true;
    }
  }

  std::map<std::string, std::string> memberToParent;
  for (auto* field : s->fields()) {
    if (!memberToParent.emplace(field->get_name(), s->get_name()).second) {
      add_error(
          field->get_lineno(),
          "Field `" + field->get_name() + "` is not unique in struct `" +
              s->get_name() + "`.");
    }
  }

  for (auto i : cpp2::get_mixins_and_members(*s)) {
    auto res =
        memberToParent.emplace(i.member->get_name(), i.mixin->get_name());
    if (!res.second) {
      add_error(
          i.mixin->get_lineno(),
          "Field `" + res.first->second + "." + i.member->get_name() +
              "` and `" + i.mixin->get_name() + "." + i.member->get_name() +
              "` can not have same name in struct `" + s->get_name() + "`.");
    }
  }
  return true;
}

bool struct_names_uniqueness_validator::visit(t_program* p) {
  set_program(p);
  std::set<std::string> seen;
  for (auto* object : p->objects()) {
    if (!seen.emplace(object->get_name()).second) {
      add_error(
          object->get_lineno(),
          "Redefinition of type `" + object->get_name() + "`.");
    }
  }
  for (auto* interaction : p->interactions()) {
    if (!seen.emplace(interaction->get_name()).second) {
      add_error(
          interaction->get_lineno(),
          "Redefinition of type `" + interaction->get_name() + "`.");
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
  std::set<std::string> seen;
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

    if (!seen.emplace(ret->get_name()).second) {
      add_error(
          func->get_lineno(),
          "Service `" + s->get_name() +
              "` has multiple methods for creating interaction `" +
              ret->get_name() + "`.");
    }
  }
  return true;
}

bool reserved_field_id_validator::visit(t_struct* s) {
  for (const auto* field : s->fields()) {
    if (field->get_key() < kMinimalValidFieldId) {
      add_error(
          field->get_lineno(), "Too many fields in `" + s->get_name() + "`");
    }
  }
  return true;
}

bool recursive_union_validator::visit(t_struct* s) {
  if (!s->is_union()) {
    return true;
  }

  for (const auto* field : s->fields()) {
    if (field->has_annotation("cpp.box")) {
      add_error(
          field->get_lineno(),
          std::string("Unions cannot contain fields with the `cpp.box` "
                      "annotation. Remove the annotation from `") +
              field->get_name() + "`.");
    }
  }

  return true;
}

bool recursive_ref_validator::visit(t_struct* s) {
  for (const auto* field : s->fields()) {
    if (field->has_annotation("cpp.box") && cpp2::has_ref_annotation(*field)) {
      add_error(
          field->get_lineno(),
          std::string(
              "The `cpp.box` annotation cannot be combined with the `ref` or "
              "`ref_type` annotations. Remove one of the annotations from `") +
              field->get_name() + "`.");
    }
  }

  return true;
}

bool recursive_optional_validator::visit(t_struct* s) {
  if (s->is_union()) {
    return true;
  }

  for (const auto* field : s->fields()) {
    if (field->has_annotation("cpp.box") &&
        field->get_req() != t_field::e_req::optional) {
      add_error(
          field->get_lineno(),
          std::string("The `cpp.box` annotation can only be used with optional "
                      "fields. Make sure ") +
              field->get_name() + std::string(" is optional."));
    }
  }

  return true;
}

} // namespace compiler
} // namespace thrift
} // namespace apache
