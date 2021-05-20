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
 * service_method_name_uniqueness_validator
 */

bool service_method_name_uniqueness_validator::visit(t_service* const service) {
  validate_service_method_names_unique(service);
  return true;
}

void service_method_name_uniqueness_validator::add_error_service_method_names(
    int const lineno,
    std::string const& service_name_new,
    std::string const& service_name_old,
    std::string const& function_name) {
  //[FALIURE:{}:{}] Function {}.{} redefines {}.{}
  add_error(
      lineno,
      "Function `" + service_name_new + "." + function_name + "` " +
          "redefines `" + service_name_old + "." + function_name + "`.");
}

void service_method_name_uniqueness_validator::
    validate_service_method_names_unique(t_service const* const service) {
  // Check for a redefinition of a function in a base service.
  std::unordered_map<std::string, t_service const*> base_function_names;
  for (auto e_s = service->get_extends(); e_s; e_s = e_s->get_extends()) {
    for (auto const ex_func : e_s->get_functions()) {
      base_function_names[ex_func->get_name()] = e_s;
    }
  }
  for (auto const fnc : service->get_functions()) {
    auto const s_pos = base_function_names.find(fnc->get_name());
    auto const e_s =
        s_pos != base_function_names.end() ? s_pos->second : nullptr;
    if (e_s) {
      add_error_service_method_names(
          fnc->get_lineno(),
          service->get_name(),
          e_s->get_full_name(),
          fnc->get_name());
    }
  }

  // Check for a redefinition of a function in the same service.
  std::unordered_set<std::string> function_names;
  for (auto const fnc : service->get_functions()) {
    if (function_names.count(fnc->get_name())) {
      add_error_service_method_names(
          fnc->get_lineno(),
          service->get_name(),
          service->get_name(),
          fnc->get_name());
    }
    function_names.insert(fnc->get_name());
  }
}

/**
 * fill_validators - the validator registry
 *
 * This is where all concrete validator types must be registered.
 */

static void fill_validators(validator_list& vs) {
  vs.add<service_method_name_uniqueness_validator>();
  vs.add<enum_values_set_validator>();
  vs.add<exception_list_is_all_exceptions_validator>();
  vs.add<union_no_qualified_fields_validator>();
  vs.add<mixin_type_correctness_validator>();
  vs.add<field_names_uniqueness_validator>();
  vs.add<struct_names_uniqueness_validator>();
  vs.add<structured_annotation_uniqueness_validator>();
  vs.add<reserved_field_id_validator>();
  vs.add<recursive_union_validator>();
  vs.add<recursive_ref_validator>();
  vs.add<recursive_optional_validator>();

  // add more validators here ...
}

void enum_values_set_validator::add_validation_error(
    int const lineno,
    std::string const& enum_value_name,
    std::string const& enum_name) {
  add_error(
      lineno,
      "Unset enum value `" + enum_value_name + "` in enum `" + enum_name +
          "`. Add an explicit value to suppress this error.");
}

bool enum_values_set_validator::visit(t_enum* const tenum) {
  validate(tenum);
  return true;
}

void enum_values_set_validator::validate(t_enum const* const tenum) {
  for (auto v : tenum->get_enum_values()) {
    if (!v->has_value()) {
      add_validation_error(v->get_lineno(), v->get_name(), tenum->get_name());
    }
  }
}

bool exception_list_is_all_exceptions_validator::visit(t_service* service) {
  auto check_func = [=](t_function const* func) {
    if (!validate_throws(func->exceptions())) {
      add_error(
          func->get_lineno(),
          "Non-exception type in throws list for method `" + func->get_name() +
              "`.");
    }
    if (!validate_throws(func->stream_exceptions())) {
      add_error(
          func->get_lineno(),
          "Non-exception type in stream throws list for method `" +
              func->get_name() + "`.");
    }
  };

  auto const& funcs = service->get_functions();
  for (auto const& func : funcs) {
    check_func(func);
  }

  return true;
}

/**
 * Check that all the elements of a throws block are actually exceptions.
 */
bool exception_list_is_all_exceptions_validator::validate_throws(
    const t_throws* throws) {
  if (throws == nullptr) {
    return true;
  }
  for (const auto* ex : throws->fields()) {
    if (!ex->get_type()->get_true_type()->is_exception()) {
      return false;
    }
  }
  return true;
}

bool union_no_qualified_fields_validator::visit(t_struct* s) {
  if (!s->is_union()) {
    return true;
  }

  for (const auto* field : s->fields()) {
    if (field->get_req() != t_field::e_req::opt_in_req_out) {
      add_error(
          field->get_lineno(),
          std::string("Unions cannot contain qualified fields. Remove ") +
              (field->get_req() == t_field::e_req::required ? "required"
                                                            : "optional") +
              " qualifier from field `" + field->get_name() + "`.");
    }
  }
  return true;
}

bool mixin_type_correctness_validator::visit(t_struct* s) {
  for (auto* field : s->fields()) {
    if (cpp2::is_mixin(*field)) {
      auto* member_type = field->get_type()->get_true_type();
      const auto& name = field->get_name();
      if (s->is_union()) {
        add_error(
            s->get_lineno(),
            "Union `" + s->get_name() + "` can not have mixin field `" + name +
                "`.");
      } else if (!member_type->is_struct()) {
        add_error(
            field->get_lineno(),
            "Mixin field `" + name + "` is not a struct but `" +
                field->get_type()->get_name() + "`.");
      } else if (field->get_req() == t_field::e_req::optional) {
        // Nothing technically stops us from marking optional field mixin.
        // However, this will bring surprising behavior. e.g. `foo.bar_ref()`
        // might throw `bad_field_access` if `bar` is inside optional mixin
        // field.
        add_error(
            field->get_lineno(),
            "Mixin field `" + name + "` can not be optional.");
      }
    }
  }
  return true;
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

bool structured_annotation_validator::visit(t_service* service) {
  validate_annotations(service, service->get_full_name());
  return true;
}

bool structured_annotation_validator::visit(t_enum* tenum) {
  validate_annotations(tenum, tenum->get_full_name());
  return true;
}

bool structured_annotation_validator::visit(t_struct* tstruct) {
  validate_annotations(tstruct, tstruct->get_full_name());
  return true;
}

bool structured_annotation_validator::visit(t_const* tconst) {
  validate_annotations(tconst, "const " + tconst->get_name());
  return true;
}

bool structured_annotation_validator::visit(t_field* tfield) {
  validate_annotations(tfield, "field " + tfield->get_name());
  // TODO(afuller): Switch to checking typedefs directly.
  // Type references cannot have structured annotations, so this
  // will only catch typedefs, which should be checked independently.
  validate_annotations(
      tfield->get_type(), "type of the field " + tfield->get_name());
  return true;
}

void structured_annotation_uniqueness_validator::validate_annotations(
    const t_named* tnamed, const std::string& full_name) {
  std::unordered_set<std::string> type_names;
  for (const auto& it : tnamed->structured_annotations()) {
    std::string type_name = it->get_type()->get_full_name();
    if (!type_names.insert(type_name).second) {
      add_error(
          it->get_lineno(),
          "Duplicate structured annotation `" + type_name + "` on `" +
              full_name + "`.");
    }
  }
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

static bool has_ref_annotation(const t_field* f) {
  return f->has_annotation(
      {"cpp.ref", "cpp2.ref", "cpp.ref_type", "cpp2.ref_type"});
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
    if (field->has_annotation("cpp.box") && has_ref_annotation(field)) {
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
