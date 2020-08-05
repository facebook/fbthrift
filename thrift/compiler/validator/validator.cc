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
    boost::optional<int> const lineno,
    std::string const& message) {
  diagnostics_->emplace_back(
      diagnostic::type::failure, program_->get_path(), lineno, message);
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
  std::ostringstream err;
  err << "Function " << service_name_new << "." << function_name << " "
      << "redefines " << service_name_old << "." << function_name;
  add_error(lineno, err.str());
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
  vs.add<enum_value_names_uniqueness_validator>();
  vs.add<enum_values_uniqueness_validator>();
  vs.add<enum_values_set_validator>();
  vs.add<exception_list_is_all_exceptions_validator>();
  vs.add<union_no_qualified_fields_validator>();
  vs.add<mixin_type_correctness_validator>();
  vs.add<field_names_uniqueness_validator>();
  vs.add<struct_names_uniqueness_validator>();
  vs.add<structured_annotations_uniqueness_validator>();

  // add more validators here ...
}

void enum_value_names_uniqueness_validator::add_validation_error(
    int const lineno,
    std::string const& value_name,
    std::string const& enum_name) {
  // [FAILURE:{}] Redefinition of value {} in enum {}
  std::ostringstream err;
  err << "Redefinition of value " << value_name << " in enum " << enum_name;
  add_error(lineno, err.str());
}

bool enum_value_names_uniqueness_validator::visit(t_enum* const tenum) {
  validate(tenum);
  return true;
}

void enum_value_names_uniqueness_validator::validate(
    t_enum const* const tenum) {
  std::unordered_set<std::string> enum_value_names;
  for (auto const v : tenum->get_enum_values()) {
    if (enum_value_names.count(v->get_name())) {
      add_validation_error(v->get_lineno(), v->get_name(), tenum->get_name());
    }
    enum_value_names.insert(v->get_name());
  }
}

void enum_values_uniqueness_validator::add_validation_error(
    int const lineno,
    t_enum_value const& enum_value,
    std::string const& existing_value_name,
    std::string const& enum_name) {
  std::ostringstream err;
  err << "Duplicate value " << enum_value.get_name() << "="
      << enum_value.get_value() << " with value " << existing_value_name
      << " in enum " << enum_name << ".";
  add_error(lineno, err.str());
}

bool enum_values_uniqueness_validator::visit(t_enum* const tenum) {
  validate(tenum);
  return true;
}

void enum_values_uniqueness_validator::validate(t_enum const* const tenum) {
  std::unordered_map<int32_t, t_enum_value const*> enum_values;
  for (auto v : tenum->get_enum_values()) {
    auto it = enum_values.find(v->get_value());
    if (it != enum_values.end()) {
      add_validation_error(
          v->get_lineno(), *v, it->second->get_name(), tenum->get_name());
    } else {
      enum_values[v->get_value()] = v;
    }
  }
}

void enum_values_set_validator::add_validation_error(
    int const lineno,
    std::string const& enum_value_name,
    std::string const& enum_name) {
  std::ostringstream err;
  err << "Unset enum value " << enum_value_name << " in enum " << enum_name
      << ". Add an explicit value to suppress this error";
  add_error(lineno, err.str());
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
    if (!validate_throws(func->get_xceptions())) {
      std::ostringstream ss;
      ss << "Non-exception type in throws list for method" << func->get_name();
      add_error(func->get_lineno(), ss.str());
    }
    if (!validate_throws(func->get_stream_xceptions())) {
      std::ostringstream ss;
      ss << "Non-exception type in stream throws list for method"
         << func->get_name();
      add_error(func->get_lineno(), ss.str());
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
/* static */ bool exception_list_is_all_exceptions_validator::validate_throws(
    t_struct* throws) {
  if (!throws) {
    return true;
  }

  const std::vector<t_field*>& members = throws->get_members();
  std::vector<t_field*>::const_iterator m_iter;
  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    if (!(*m_iter)->get_type()->get_true_type()->is_xception()) {
      return false;
    }
  }
  return true;
}

bool union_no_qualified_fields_validator::visit(t_struct* s) {
  if (!s->is_union()) {
    return true;
  }

  for (const auto* field : s->get_members()) {
    if (field->get_req() != t_field::T_OPT_IN_REQ_OUT) {
      std::ostringstream ss;
      ss << "Unions cannot contain qualified fields. Remove "
         << (field->get_req() == t_field::T_REQUIRED ? "required" : "optional")
         << " qualifier from field '" << field->get_name() << "'";
      add_error(field->get_lineno(), ss.str());
    }
  }
  return true;
}

bool mixin_type_correctness_validator::visit(t_struct* s) {
  for (auto* member : s->get_members()) {
    if (member->is_mixin()) {
      if (s->is_union()) {
        add_error(
            s->get_lineno(),
            "Union `" + s->get_name() + "` can not have mixin field `" +
                member->get_name() + '`');
      } else if (!member->get_type()->get_true_type()->is_struct()) {
        add_error(
            member->get_lineno(),
            "Mixin field `" + member->get_name() + "` is not a struct but " +
                member->get_type()->get_name());
      } else if (member->get_type()->get_true_type()->is_union()) {
        add_error(
            member->get_lineno(),
            "Mixin field `" + member->get_name() +
                "` is not a struct but union");
      } else if (member->get_req() == t_field::T_OPTIONAL) {
        // Nothing technically stops us from marking optional field mixin.
        // However, this will bring surprising behavior. e.g. `foo.bar_ref()`
        // might throw `bad_field_access` if `bar` is inside optional mixin
        // field.
        add_error(
            member->get_lineno(),
            "Mixin field `" + member->get_name() + "` can not be optional");
      }
    }
  }
  return true;
}

bool field_names_uniqueness_validator::visit(t_struct* s) {
  // If member is not struct, we couldn't extract field from mixin
  for (auto* member : s->get_members()) {
    if (member->is_mixin() &&
        !member->get_type()->get_true_type()->is_struct()) {
      return true;
    }
  }

  std::map<std::string, std::string> memberToParent;
  for (auto* member : s->get_members()) {
    if (!memberToParent.emplace(member->get_name(), s->get_name()).second) {
      add_error(
          member->get_lineno(),
          "Field `" + member->get_name() + "` is not unique in struct `" +
              s->get_name() + '`');
    }
  }

  for (auto i : s->get_mixins_and_members()) {
    auto res =
        memberToParent.emplace(i.member->get_name(), i.mixin->get_name());
    if (!res.second) {
      add_error(
          i.mixin->get_lineno(),
          "Field `" + res.first->second + "." + i.member->get_name() +
              "` and `" + i.mixin->get_name() + "." + i.member->get_name() +
              "` can not have same name in struct `" + s->get_name() + '`');
    }
  }
  return true;
}

bool struct_names_uniqueness_validator::visit(t_program* p) {
  set_program(p);
  std::set<std::string> seen;
  for (auto* object : p->get_objects()) {
    if (!seen.emplace(object->get_name()).second) {
      add_error(
          object->get_lineno(),
          "Redefinition of type `" + object->get_name() + "`");
    }
  }
  return true;
}

bool structured_annotations_validator::visit(t_service* service) {
  validate_annotations(service, service->get_full_name());
  return true;
}

bool structured_annotations_validator::visit(t_enum* tenum) {
  validate_annotations(tenum, tenum->get_full_name());
  return true;
}

bool structured_annotations_validator::visit(t_struct* tstruct) {
  validate_annotations(tstruct, tstruct->get_full_name());
  return true;
}

bool structured_annotations_validator::visit(t_field* tfield) {
  validate_annotations(tfield, "field " + tfield->get_name());
  validate_annotations(
      tfield->get_type(), "type of the field " + tfield->get_name());
  return true;
}

void structured_annotations_uniqueness_validator::validate_annotations(
    t_annotated* tannotated,
    const std::string& tannotated_name) {
  std::unordered_set<std::string> full_names;
  std::string name;
  for (const auto& it : tannotated->structured_annotations_) {
    name = it->get_type()->get_full_name();
    if (full_names.count(name) != 0) {
      add_error(
          it->get_lineno(),
          "Duplicate structured annotation `" + name + "` in the `" +
              tannotated_name + "`.");
    } else {
      full_names.insert(name);
    }
  }
}

} // namespace compiler
} // namespace thrift
} // namespace apache
