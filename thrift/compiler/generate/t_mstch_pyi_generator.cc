/*
 * Copyright 2017-present Facebook, Inc.
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
#include <memory>

#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/replace.hpp>

#include <thrift/compiler/generate/common.h>
#include <thrift/compiler/generate/t_mstch_generator.h>

namespace {

class t_mstch_pyi_generator : public t_mstch_generator {
 public:
  t_mstch_pyi_generator(
      t_program* program,
      const std::map<std::string, std::string>& parsed_options,
      const std::string& /* option_string unused */)
      : t_mstch_generator(program, "pyi", parsed_options) {
    out_dir_base_ = "gen-py";
  }

  void generate_program() override;
  mstch::map extend_program(const t_program&) override;
  mstch::map extend_field(const t_field&) override;
  mstch::map extend_type(const t_type&) override;
  mstch::map extend_service(const t_service&) override;

 protected:
  void generate_init_files(const t_program&);
  void generate_constants(const t_program&);
  void generate_ttypes(const t_program&);
  void generate_services(const t_program&);
  boost::filesystem::path package_to_path(const t_program&);
  mstch::array get_return_types(const t_program&);
  void add_container_types(const t_program&, mstch::map&);
  vector<std::string> get_py_namespace_raw(
      const t_program&,
      const string& tail = "");
  mstch::array get_py_namespace(const t_program&, const string& tail = "");
  std::string flatten_type_name(const t_type&) const;

 private:
  void load_container_type(
      vector<t_type*>& container_types,
      std::set<string>& visited_names,
      t_type* type) const;
};

mstch::map t_mstch_pyi_generator::extend_program(const t_program& program) {
  const auto pyNamespaces = get_py_namespace(program, "");
  mstch::array includeNamespaces;
  for (const auto included_program : program.get_includes()) {
    if (included_program->get_path() == program.get_path()) {
      continue;
    }
    const auto ns = get_py_namespace(*included_program);
    auto const hasServices = included_program->get_services().size() > 0;
    auto const hasStructs = included_program->get_objects().size() > 0;
    auto const hasEnums = included_program->get_enums().size() > 0;
    auto const hasTypes = hasStructs || hasEnums;
    const mstch::map include_ns{
        {"includeNamespace", ns},
        {"hasServices?", hasServices},
        {"hasTypes?", hasTypes},
    };
    includeNamespaces.push_back(include_ns);
  }
  auto const asyncio = cache_->parsed_options_.count("asyncio") != 0;
  mstch::map result{
      {"returnTypes", get_return_types(program)},
      {"pyNamespaces", pyNamespaces},
      {"includeNamespaces", includeNamespaces},
      {"asyncio?", asyncio},
  };
  add_container_types(program, result);
  return result;
}

mstch::map t_mstch_pyi_generator::extend_field(const t_field& field) {
  auto req = field.get_req();
  const auto required = req == t_field::e_req::T_REQUIRED;
  const auto optional = req == t_field::e_req::T_OPTIONAL;
  const auto unqualified = !required && !optional;
  const auto hasValue = field.get_value() != nullptr;
  const auto hasDefaultValue = hasValue || unqualified;
  const auto requireValue = required && !hasDefaultValue;
  // For typing, can a property getter return None, if so it needs to Optional[]
  const auto isPEP484Optional = (optional || (!hasDefaultValue && !required));

  mstch::map result{
      {"requireValue?", requireValue},
      {"PEP484Optional?", isPEP484Optional},
  };
  return result;
}

mstch::map t_mstch_pyi_generator::extend_type(const t_type& type) {
  const auto type_program = type.get_program();
  const auto program = type_program ? type_program : get_program();
  const auto modulePath = get_py_namespace(*program, "ttypes");
  bool externalProgram = false;
  const auto& prog_path = program->get_path();
  if (prog_path != get_program()->get_path()) {
    externalProgram = true;
  }

  mstch::map result{
      {"modulePath", modulePath},
      {"externalProgram?", externalProgram},
      {"flat_name", flatten_type_name(type)},
  };
  return result;
}

mstch::map t_mstch_pyi_generator::extend_service(const t_service& service) {
  const auto program = service.get_program();
  const auto& pyNamespaces = get_py_namespace(*program);
  bool externalProgram = false;
  const auto& prog_path = program->get_path();
  if (prog_path != get_program()->get_path()) {
    externalProgram = true;
  }
  mstch::map result{
      {"externalProgram?", externalProgram},
      {"pyNamespaces", pyNamespaces},
      {"programName", program->get_name()},
  };

  return result;
}

void t_mstch_pyi_generator::generate_init_files(const t_program& program) {
  auto path = package_to_path(program);
  auto directory = boost::filesystem::path{};
  for (auto path_part : path) {
    directory /= path_part;
    render_to_file(
        program, "common/AutoGeneratedPy", directory / "__init__.pyi");
  }
}

void t_mstch_pyi_generator::generate_constants(const t_program& program) {
  auto path = package_to_path(program);
  std::string module = "constants.pyi";
  render_to_file(program, module, path / module);
}

void t_mstch_pyi_generator::generate_ttypes(const t_program& program) {
  auto path = package_to_path(program);
  std::string module = "ttypes.pyi";
  render_to_file(program, module, path / module);
}

void t_mstch_pyi_generator::generate_services(const t_program& program) {
  auto path = package_to_path(program);
  std::string template_ = "service.pyi";
  for (const auto service : program.get_services()) {
    std::string module = service->get_name() + ".pyi";
    mstch::map extra{
        {"service", dump(*service)},
    };
    render_to_file(program, extra, template_, path / module);
  }
}

boost::filesystem::path t_mstch_pyi_generator::package_to_path(
    const t_program& program) {
  const auto ns = get_py_namespace_raw(program);
  std::stringstream ss;
  for (auto itr = ns.begin(); itr != ns.end(); ++itr) {
    ss << *itr;
    if (itr != ns.end()) {
      ss << "/";
    }
  }
  return boost::filesystem::path{ss.str()};
}

mstch::array t_mstch_pyi_generator::get_return_types(const t_program& program) {
  mstch::array distinct_return_types;
  std::set<string> visited_names;

  for (const auto service : program.get_services()) {
    for (const auto function : service->get_functions()) {
      const auto returntype = function->get_returntype();
      string flat_name = flatten_type_name(*returntype);
      if (!visited_names.count(flat_name)) {
        distinct_return_types.push_back(dump(*returntype));
        visited_names.insert(flat_name);
      }
    }
  }
  return distinct_return_types;
}

/*
 * Add two items to the results map, one "containerTypes" that lists all
 * container types, and one "moveContainerTypes" that treats binary and string
 * as one type. Required because in pxd's we can't have duplicate move(string)
 * definitions */
void t_mstch_pyi_generator::add_container_types(
    const t_program& program,
    mstch::map& results) {
  vector<t_type*> container_types;
  vector<t_type*> move_container_types;
  std::set<string> visited_names;

  for (const auto service : program.get_services()) {
    for (const auto function : service->get_functions()) {
      for (const auto field : function->get_arglist()->get_members()) {
        auto arg_type = field->get_type();
        load_container_type(container_types, visited_names, arg_type);
      }
      auto return_type = function->get_returntype();
      load_container_type(container_types, visited_names, return_type);
    }
  }
  for (const auto object : program.get_objects()) {
    for (const auto field : object->get_members()) {
      auto ref_type = field->get_type();
      load_container_type(container_types, visited_names, ref_type);
    }
  }
  for (const auto constant : program.get_consts()) {
    const auto const_type = constant->get_type();
    load_container_type(container_types, visited_names, const_type);
  }

  results.emplace("containerTypes", dump_elems(container_types));

  // create second set that treats strings and binaries the same
  visited_names.clear();

  for (const auto type : container_types) {
    auto flat_name = flatten_type_name(*type);
    boost::algorithm::replace_all(flat_name, "binary", "string");

    if (visited_names.count(flat_name)) {
      continue;
    }
    visited_names.insert(flat_name);
    move_container_types.push_back(type);
  }
  results.emplace("moveContainerTypes", dump_elems(move_container_types));
}

void t_mstch_pyi_generator::load_container_type(
    vector<t_type*>& container_types,
    std::set<string>& visited_names,
    t_type* type) const {
  if (!type->is_container()) {
    return;
  }

  string flat_name = flatten_type_name(*type);
  if (visited_names.count(flat_name)) {
    return;
  }

  if (type->is_list()) {
    const auto elem_type = dynamic_cast<const t_list*>(type)->get_elem_type();
    load_container_type(container_types, visited_names, elem_type);
  } else if (type->is_set()) {
    const auto elem_type = dynamic_cast<const t_set*>(type)->get_elem_type();
    load_container_type(container_types, visited_names, elem_type);
  } else if (type->is_map()) {
    const auto map_type = dynamic_cast<const t_map*>(type);
    const auto key_type = map_type->get_key_type();
    const auto value_type = map_type->get_val_type();
    load_container_type(container_types, visited_names, key_type);
    load_container_type(container_types, visited_names, value_type);
  }

  visited_names.insert(flat_name);
  container_types.push_back(type);
}

std::string t_mstch_pyi_generator::flatten_type_name(const t_type& type) const {
  if (type.is_list()) {
    return "List__" +
        flatten_type_name(*dynamic_cast<const t_list&>(type).get_elem_type());
  } else if (type.is_set()) {
    return "Set__" +
        flatten_type_name(*dynamic_cast<const t_set&>(type).get_elem_type());
  } else if (type.is_map()) {
    return (
        "Map__" +
        flatten_type_name(*dynamic_cast<const t_map&>(type).get_key_type()) +
        "_" +
        flatten_type_name(*dynamic_cast<const t_map&>(type).get_val_type()));
  } else if (type.is_binary()) {
    return "binary";
  } else {
    return type.get_name();
  }
}

vector<std::string> t_mstch_pyi_generator::get_py_namespace_raw(
    const t_program& program,
    const string& tail) {
  auto const asyncio = cache_->parsed_options_.count("asyncio") != 0;
  auto& py_namespace = program.get_namespace("py");
  auto& py_asyncio_namespace = program.get_namespace("py.asyncio");
  auto _namespace = asyncio && !py_asyncio_namespace.empty()
      ? py_asyncio_namespace
      : py_namespace;

  const auto name = program.get_name();
  vector<string> ns = split_namespace(_namespace);
  if (ns.empty()) {
    ns.push_back(name);
  }
  if (tail != "") {
    ns.push_back(tail);
  }
  return ns;
}

mstch::array t_mstch_pyi_generator::get_py_namespace(
    const t_program& program,
    const string& tail) {
  return dump_elems(get_py_namespace_raw(program, tail));
}

void t_mstch_pyi_generator::generate_program() {
  mstch::config::escape = [](const std::string& s) { return s; };
  generate_init_files(*get_program());
  generate_constants(*get_program());
  generate_ttypes(*get_program());
  generate_services(*get_program());
}

THRIFT_REGISTER_GENERATOR(
    mstch_pyi,
    "Legacy Python type information",
    "    no arguments\n");
} // namespace
