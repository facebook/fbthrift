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


#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/classification.hpp>

#include <memory>

#include <thrift/compiler/generate/common.h>
#include <thrift/compiler/generate/t_mstch_generator.h>

namespace {

class t_mstch_py3_generator : public t_mstch_generator {
  public:
    t_mstch_py3_generator(
        t_program* program,
        const std::map<std::string, std::string>& parsed_options,
        const std::string& /* option_string unused */)
        : t_mstch_generator(program, "py3", parsed_options) {

      this->out_dir_base_ = "gen-py3";
      auto include_prefix = this->get_option("include_prefix");
      if (include_prefix) {
        program->set_include_prefix(*include_prefix);
      }
    }

    void generate_program() override;
    mstch::map extend_program(const t_program&) const override;
    mstch::map extend_type(const t_type&) const override;
    mstch::map extend_service(const t_service&) const override;

  protected:
    void generate_init_files(const t_program&);
    void generate_structs(const t_program&);
    void generate_services(const t_program&);
    void generate_clients(const t_program&);
    boost::filesystem::path package_to_path(std::string package);
    mstch::array get_return_types(const t_program&) const;
    mstch::array get_container_types(const t_program&) const;
    mstch::array get_cpp2_namespace(const t_program&) const;
    mstch::array get_py3_namespace(
      const t_program&,
      const string& tail = ""
    ) const;
    std::string flatten_type_name(const t_type&) const;

  private:
    void load_container_type(
      vector<t_type*>& container_types,
      std::set<string>& visited_names,
      t_type* type
    ) const;
};

mstch::map t_mstch_py3_generator::extend_program(
  const t_program& program
) const {
  const auto& cppNamespaces = this->get_cpp2_namespace(program);
  const auto& py3Namespaces = this->get_py3_namespace(program, "");
  mstch::array includeNamespaces;
  for (const auto included_program : program.get_includes()) {
    if (included_program->get_path() == program.get_path()) {
      continue;
    }
    const auto ns = this->get_py3_namespace(
      *included_program,
      included_program->get_name()
    );
    const mstch::map include_ns {{"includeNamespace", ns}};
    includeNamespaces.push_back(include_ns);
  }

  mstch::map result {
    {"returnTypes", this->get_return_types(program)},
    {"containerTypes", this->get_container_types(program)},
    {"cppNamespaces", cppNamespaces},
    {"py3Namespaces", py3Namespaces},
    {"includeNamespaces", includeNamespaces},
  };
  return result;
}

mstch::map t_mstch_py3_generator::extend_type(const t_type& type) const {
  const auto type_program = type.get_program();
  const auto program = type_program? type_program: this->get_program();
  const auto modulePath = this->get_py3_namespace(
    *program,
    program->get_name() + ".types"
  );
  const auto& cppNamespaces = this->get_cpp2_namespace(*program);

  bool externalProgram = false;
  const auto& prog_path = program->get_path();
  if (prog_path != this->get_program()->get_path()) {
    externalProgram = true;
  }

  mstch::map result {
    {"modulePath", modulePath},
    {"externalProgram?", externalProgram},
    {"flat_name", this->flatten_type_name(type)},
    {"cppNamespaces", cppNamespaces},
  };
  return result;
}

mstch::map t_mstch_py3_generator::extend_service(
  const t_service& service
) const {
  const auto program = service.get_program();
  const auto& cppNamespaces = this->get_cpp2_namespace(*program);
  const auto& py3Namespaces = this->get_py3_namespace(*program);
  string include_prefix = program->get_include_prefix();
  // Used to avoid circular import in ServicesWrapper.h.mustache
  bool externalProgram = false;
  if (service.get_extends()) {
    const auto ext_program = service.get_extends()->get_program();
    const auto& ext_path = ext_program->get_path();
    const auto& prog_path = program->get_path();
    if (ext_path != prog_path) {
      externalProgram = true;
    }
  }
  mstch::map result {
    {"externalProgram?", externalProgram},
    {"cppNamespaces", cppNamespaces},
    {"py3Namespaces", py3Namespaces},
    {"programName", program->get_name()},
    {"includePrefix", include_prefix}
  };
  return result;
}

void t_mstch_py3_generator::generate_init_files(const t_program& program) {
  auto path = this->package_to_path(program.get_namespace("py3"));
  auto directory = boost::filesystem::path{};
  for (auto path_part : path) {
    directory /= path_part;
    this->write_output(directory / "__init__.py", "");
  }
}

void t_mstch_py3_generator::generate_structs(const t_program& program) {
  auto path = this->package_to_path(program.get_namespace("py3"));
  auto name = program.get_name();
  std::string module = "types";
  this->render_to_file(program, "Struct.pxd", path / name / (module + ".pxd"));
  this->render_to_file(program, "Struct.pyx", path / name / (module + ".pyx"));
}

void t_mstch_py3_generator::generate_services(const t_program& program) {
  auto path = this->package_to_path(program.get_namespace("py3"));

  auto name = program.get_name();
  this->render_to_file(
    program, "Services.pxd", path / name / "services.pxd");
  this->render_to_file(
    program, "CythonServices.pyx", path / name / "services.pyx");

  std::string basename = "services_wrapper";
  this->render_to_file(
    program, "ServicesWrapper.h", path / name / (basename + ".h"));
  this->render_to_file(
    program, "ServicesWrapper.cpp", path / name / (basename + ".cpp"));
  this->render_to_file(
    program, "ServicesWrapper.pxd", path / name / (basename + ".pxd"));
}

void t_mstch_py3_generator::generate_clients(const t_program& program) {
  auto path = this->package_to_path(program.get_namespace("py3"));

  auto name = program.get_name();
  this->render_to_file(
    program, "Clients.pxd", path / name / "clients.pxd");
  this->render_to_file(
    program, "CythonClients.pyx", path / name / "clients.pyx");

  std::string basename = "clients_wrapper";
  this->render_to_file(
    program, "ClientsWrapper.h", path / name / (basename + ".h"));
  this->render_to_file(
    program, "ClientsWrapper.cpp", path / name / (basename + ".cpp"));
  this->render_to_file(
    program, "ClientsWrapper.pxd", path / name / (basename + ".pxd"));
}

boost::filesystem::path t_mstch_py3_generator::package_to_path(
  std::string package
) {
  boost::algorithm::replace_all(package, ".", "/");
  return boost::filesystem::path{package};
}

mstch::array t_mstch_py3_generator::get_return_types(
  const t_program& program
) const {
  mstch::array distinct_return_types;
  std::set<string> visited_names;

  for (const auto service : program.get_services()) {
    for (const auto function : service->get_functions()) {
      const auto returntype = function->get_returntype();
      string flat_name = this->flatten_type_name(*returntype);
      if (!visited_names.count(flat_name)) {
        distinct_return_types.push_back(this->dump(*returntype));
        visited_names.insert(flat_name);
      }
    }
  }
  return distinct_return_types;
}

mstch::array t_mstch_py3_generator::get_container_types(
  const t_program& program
) const {
  vector<t_type*> container_types;
  std::set<string> visited_names;

  for (const auto service : program.get_services()) {
    for (const auto function : service->get_functions()) {
      for (const auto field : function->get_arglist()->get_members()) {
        auto arg_type = field->get_type();
        this->load_container_type(
          container_types,
          visited_names,
          arg_type
        );
      }
      auto return_type = function->get_returntype();
      this->load_container_type(container_types, visited_names, return_type);
    }
  }
  for (const auto object :program.get_objects()) {
    for (const auto field : object->get_members()) {
      auto ref_type = field->get_type();
      this->load_container_type(container_types, visited_names, ref_type);
    }
  }
  for (const auto constant: program.get_consts()) {
    const auto const_type = constant->get_type();
    this->load_container_type(container_types, visited_names, const_type);

  }
  return this->dump_elems(container_types);
}

void t_mstch_py3_generator::load_container_type(
  vector<t_type*>& container_types,
  std::set<string>& visited_names,
  t_type* type
) const {
  if (!type->is_container()) return;

  string flat_name = this->flatten_type_name(*type);
  if (visited_names.count(flat_name)) return;

  if (type->is_list()) {
    const auto elem_type = dynamic_cast<const t_list*>(type)->get_elem_type();
    this->load_container_type(container_types, visited_names, elem_type);
  }
  else if (type->is_set()) {
    const auto elem_type = dynamic_cast<const t_set*>(type)->get_elem_type();
    this->load_container_type(container_types, visited_names, elem_type);
  }
  else if (type->is_map()) {
    const auto map_type = dynamic_cast<const t_map*>(type);
    const auto key_type = map_type->get_key_type();
    const auto value_type = map_type->get_val_type();
    this->load_container_type(container_types, visited_names, key_type);
    this->load_container_type(container_types, visited_names, value_type);
  }

  visited_names.insert(flat_name);
  container_types.push_back(type);
}

std::string t_mstch_py3_generator::flatten_type_name(const t_type& type) const {
    if (type.is_list()) {
      return "List__" + this->flatten_type_name(
        *dynamic_cast<const t_list&>(type).get_elem_type()
      );
  } else if (type.is_set()) {
    return "Set__" + this->flatten_type_name(
      *dynamic_cast<const t_set&>(type).get_elem_type()
    );
  } else if (type.is_map()) {
      return ("Map__" +
        this->flatten_type_name(
          *dynamic_cast<const t_map&>(type).get_key_type()
        ) + "_" +
        this->flatten_type_name(
          *dynamic_cast<const t_map&>(type).get_val_type()
        )
      );
  } else if (type.is_binary()) {
      return "binary";
  } else {
    return type.get_name();
  }
}

mstch::array t_mstch_py3_generator::get_cpp2_namespace(
  const t_program& program
) const {
  auto cpp_namespace = program.get_namespace("cpp2");
  if (cpp_namespace == "") {
    cpp_namespace = program.get_namespace("cpp");
    if (cpp_namespace == "") {
      cpp_namespace = "cpp2";
    }
    else {
      cpp_namespace = cpp_namespace + ".cpp2";
    }
  }
  vector<string> ns = split_namespace(cpp_namespace);
  return this->dump_elems(ns);
}

mstch::array t_mstch_py3_generator::get_py3_namespace(
  const t_program& program,
  const string& tail
) const {
  const auto& py3_namespace = program.get_namespace("py3");
  vector<string> ns = split_namespace(py3_namespace);
  if (tail != "") {
    ns.push_back(tail);
  }
  return this->dump_elems(ns);
}

void t_mstch_py3_generator::generate_program() {
  mstch::config::escape = [](const std::string& s) { return s; };
  this->generate_init_files(*this->get_program());
  this->generate_structs(*this->get_program());
  this->generate_services(*this->get_program());
  this->generate_clients(*this->get_program());
}

THRIFT_REGISTER_GENERATOR(
  mstch_py3,
  "Python 3",
  "    include_prefix:  Use full include paths in generated files.\n"
);
}
