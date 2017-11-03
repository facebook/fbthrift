/*
 * Copyright 2016-present Facebook, Inc.
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
#include <boost/algorithm/string/classification.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>

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
      out_dir_base_ = "gen-py3";
      auto include_prefix = get_option("include_prefix");
      if (include_prefix) {
        program->set_include_prefix(*include_prefix);
      }
    }

    void generate_program() override;
    mstch::map extend_program(const t_program&) override;
    mstch::map extend_field(const t_field&) override;
    mstch::map extend_type(const t_type&) override;
    mstch::map extend_service(const t_service&) override;

   protected:
    bool should_resolve_typedefs() const override {
      return true;
    }

    void generate_init_files(const t_program&);
    void generate_structs(const t_program&);
    void generate_services(const t_program&);
    void generate_clients(const t_program&);
    boost::filesystem::path package_to_path(std::string package);
    mstch::array get_return_types(const t_program&);
    void add_per_type_data(const t_program&, mstch::map&);
    void add_cpp_includes(const t_program&, mstch::map&);
    mstch::array get_cpp2_namespace(const t_program&);
    mstch::array get_py3_namespace(const t_program&, const string& tail = "");
    std::string flatten_type_name(const t_type&);

   private:
    struct type_data {
      vector<const t_type*> containers;
      std::set<string> container_names;
      vector<const t_type*> custom_templates;
      std::set<string> custom_template_names;
      vector<const t_type*> custom_types;
      std::set<string> custom_type_names;
    };
    void visit_type(t_type* type, type_data& data);
    void visit_single_type(const t_type& type, type_data& data);
    string ref_type(const t_field& field) const;
    string get_cpp_template(const t_type& type) const;
    string to_cython_template(const string& cpp_template) const;
    bool is_default_template(const string& cpp_template, const t_type& type)
        const;
    string get_cpp_type(const t_type& type) const;
    string to_cython_type(const string& cpp_template) const;
};

mstch::map t_mstch_py3_generator::extend_program(const t_program& program) {
  const auto& cppNamespaces = get_cpp2_namespace(program);
  const auto& py3Namespaces = get_py3_namespace(program, "");
  mstch::array includeNamespaces;
  for (const auto included_program : program.get_includes()) {
    if (included_program->get_path() == program.get_path()) {
      continue;
    }
    const auto ns =
        get_py3_namespace(*included_program, included_program->get_name());
    auto const hasServices = included_program->get_services().size() > 0;
    auto const hasStructs = included_program->get_structs().size() > 0;
    auto const hasEnums = included_program->get_enums().size() > 0;
    auto const hasTypes = hasStructs || hasEnums;
    const mstch::map include_ns {
      {"includeNamespace", ns},
      {"hasServices?", hasServices},
      {"hasTypes?", hasTypes},
    };
    includeNamespaces.push_back(include_ns);
  }

  mstch::map result{
      {"returnTypes", get_return_types(program)},
      {"cppNamespaces", cppNamespaces},
      {"py3Namespaces", py3Namespaces},
      {"includeNamespaces", includeNamespaces},
  };
  add_cpp_includes(program, result);
  add_per_type_data(program, result);
  return result;
}

mstch::map t_mstch_py3_generator::extend_field(const t_field& field) {
  string ref_type = this->ref_type(field);

  mstch::map result{
      {"reference?", (ref_type != "")},
      {"ref_type", ref_type},
      {"unique_ref?", (ref_type == "unique")},
      {"shared_ref?", (ref_type == "shared")},
      {"shared_const_ref?", (ref_type == "shared_const")},
  };
  return result;
}

// TODO: This needs to mirror the behavior of t_cpp_generator::cpp_ref_type
// but it's not obvious how to get there
string t_mstch_py3_generator::ref_type(const t_field& field) const {
  auto& annotations = field.annotations_;

  // backward compatibility with 'ref' annotation
  if (annotations.count("cpp.ref") != 0 || annotations.count("cpp2.ref") != 0) {
    return "unique";
  }

  auto it = annotations.find("cpp.ref_type");
  if (it == annotations.end()) {
    it = annotations.find("cpp2.ref_type");
  }

  if (it == annotations.end()) {
    return "";
  }

  auto& reftype = it->second;

  if (reftype == "unique" || reftype == "std::unique_ptr") {
    return "unique";
  } else if (reftype == "shared" || reftype == "std::shared_ptr") {
    return "shared";
  } else if (reftype == "shared_const") {
    return "shared_const";
  } else {
    // It is legal to get here but hopefully nobody will in practice, since
    // we're not set up to handle other kinds of refs:
    std::ostringstream err;
    err << "Unhandled ref_type " << reftype;
    throw std::runtime_error{err.str()};
  }
}

mstch::map t_mstch_py3_generator::extend_type(const t_type& type) {
  const auto type_program = type.get_program();
  const auto program = type_program ? type_program : get_program();
  const auto modulePath =
      get_py3_namespace(*program, program->get_name() + ".types");
  const auto& cppNamespaces = get_cpp2_namespace(*program);

  bool externalProgram = false;
  const auto& prog_path = program->get_path();
  if (prog_path != get_program()->get_path()) {
    externalProgram = true;
  }

  string cpp_template = this->get_cpp_template(type);
  string cython_template = this->to_cython_template(cpp_template);
  bool is_default_template = this->is_default_template(cpp_template, type);

  string cpp_type = this->get_cpp_type(type);
  bool has_custom_type = (cpp_type != "");
  string cython_type = this->to_cython_type(cpp_type);

  mstch::map result{
      {"modulePath", modulePath},
      {"externalProgram?", externalProgram},
      {"flat_name", flatten_type_name(type)},
      {"cppNamespaces", cppNamespaces},
      {"cppTemplate", cpp_template},
      {"cythonTemplate", cython_template},
      {"defaultTemplate?", is_default_template},
      {"cppCustomType", cpp_type},
      {"cythonCustomType", cython_type},
      {"hasCustomType?", has_custom_type},
  };
  return result;
}

// This handles is_unordered as a special case
string t_mstch_py3_generator::get_cpp_template(const t_type& type) const {
  auto& annotations = type.annotations_;

  auto it = annotations.find("cpp.template");
  if (it == annotations.end()) {
    it = annotations.find("cpp2.template");
  }

  if (it != annotations.end()) {
    return it->second;
  } else if (type.is_list()) {
    return "std::vector";
  } else if (type.is_set()) {
    bool unordered = dynamic_cast<const t_set&>(type).is_unordered();
    return unordered ? "std::unordered_set" : "std::set";
  } else if (type.is_map()) {
    bool unordered = dynamic_cast<const t_map&>(type).is_unordered();
    return unordered ? "std::unordered_map" : "std::map";
  } else {
    return "";
  }
}

string t_mstch_py3_generator::to_cython_template(
    const string& cpp_template) const {
  // handle special built-ins first:
  if (cpp_template == "std::vector") {
    return "vector";
  } else if (cpp_template == "std::set") {
    return "cset";
  } else if (cpp_template == "std::map") {
    return "cmap";
  }

  // then default handling:
  string cython_template = cpp_template;
  boost::algorithm::replace_all(cython_template, "::", "_");
  return cython_template;
}

bool t_mstch_py3_generator::is_default_template(
    const string& cpp_template,
    const t_type& type) const {
  return (!type.is_container() && cpp_template == "") ||
      (type.is_list() && cpp_template == "std::vector") ||
      (type.is_set() && cpp_template == "std::set") ||
      (type.is_map() && cpp_template == "std::map");
}

string t_mstch_py3_generator::get_cpp_type(const t_type& type) const {
  auto& annotations = type.annotations_;

  auto it = annotations.find("cpp.type");
  if (it == annotations.end()) {
    it = annotations.find("cpp2.type");
  }

  if (it != annotations.end()) {
    return it->second;
  } else {
    return "";
  }
}

string strip_comments(const string& str) {
  string s = str;
  while (true) {
    size_t comment_idx = s.find("/*");
    if (comment_idx == string::npos) {
      return s;
    }

    size_t end_comment_idx = s.find("*/", comment_idx);
    if (end_comment_idx != string::npos) {
      end_comment_idx += 2;
    }

    s = s.substr(0, comment_idx) + s.substr(end_comment_idx);
  }
}

string t_mstch_py3_generator::to_cython_type(const string& cpp_type) const {
  if (cpp_type == "") {
    return "";
  }

  string cython_type = cpp_type;
  cython_type = strip_comments(cython_type);
  boost::algorithm::replace_all(cython_type, "::", "_");
  boost::algorithm::replace_all(cython_type, "<", "_");
  boost::algorithm::replace_all(cython_type, ">", "");
  boost::algorithm::replace_all(cython_type, ", ", "_");
  boost::algorithm::replace_all(cython_type, ",", "_");
  return cython_type;
}

mstch::map t_mstch_py3_generator::extend_service(const t_service& service) {
  const auto program = service.get_program();
  const auto& cppNamespaces = get_cpp2_namespace(*program);
  const auto& py3Namespaces = get_py3_namespace(*program);
  string include_prefix = program->get_include_prefix();
  // Used to avoid circular import in ServicesWrapper.h.mustache
  bool externalProgram = false;
  const auto& prog_path = program->get_path();
  if (prog_path != get_program()->get_path()) {
    externalProgram = true;
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
  auto path = package_to_path(program.get_namespace("py3"));
  auto directory = boost::filesystem::path{};
  for (auto path_part : path) {
    directory /= path_part;
    render_to_file(
        program, "common/AutoGeneratedPy", directory / "__init__.py");
  }
}

void t_mstch_py3_generator::generate_structs(const t_program& program) {
  mstch::map extra_context{
      {"program:typeContext?", true},
  };

  auto path = package_to_path(program.get_namespace("py3"));
  auto name = program.get_name();
  std::string module = "types";
  render_to_file(
      program, extra_context, "types.pxd", path / name / (module + ".pxd"));
  render_to_file(
      program, extra_context, "types.pyx", path / name / (module + ".pyx"));
}

void t_mstch_py3_generator::generate_services(const t_program& program) {
  mstch::map extra_context{
      {"program:typeContext?", false},
  };

  auto path = package_to_path(program.get_namespace("py3"));

  auto name = program.get_name();
  render_to_file(
      program, extra_context, "services.pxd", path / name / "services.pxd");
  render_to_file(
      program, extra_context, "services.pyx", path / name / "services.pyx");

  std::string basename = "services_wrapper";
  auto cpp_path = boost::filesystem::path{name};
  render_to_file(
      program,
      extra_context,
      "services_wrapper.h",
      cpp_path / (basename + ".h"));
  render_to_file(
      program,
      extra_context,
      "services_wrapper.cpp",
      cpp_path / (basename + ".cpp"));
  render_to_file(
      program,
      extra_context,
      "services_wrapper.pxd",
      path / name / (basename + ".pxd"));
}

void t_mstch_py3_generator::generate_clients(const t_program& program) {
  mstch::map extra_context{
      {"program:typeContext?", false},
  };

  auto path = package_to_path(program.get_namespace("py3"));

  auto name = program.get_name();
  render_to_file(
      program, extra_context, "clients.pxd", path / name / "clients.pxd");
  render_to_file(
      program, extra_context, "clients.pyx", path / name / "clients.pyx");

  std::string basename = "clients_wrapper";
  auto cpp_path = boost::filesystem::path{name};
  render_to_file(
      program,
      extra_context,
      "clients_wrapper.h",
      cpp_path / (basename + ".h"));
  render_to_file(
      program,
      extra_context,
      "clients_wrapper.cpp",
      cpp_path / (basename + ".cpp"));
  render_to_file(
      program,
      extra_context,
      "clients_wrapper.pxd",
      path / name / (basename + ".pxd"));
}

boost::filesystem::path t_mstch_py3_generator::package_to_path(
  std::string package
) {
  boost::algorithm::replace_all(package, ".", "/");
  return boost::filesystem::path{package};
}

mstch::array t_mstch_py3_generator::get_return_types(const t_program& program) {
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
void t_mstch_py3_generator::add_per_type_data(
    const t_program& program,
    mstch::map& results) {
  type_data data;

  for (const auto service : program.get_services()) {
    for (const auto function : service->get_functions()) {
      for (const auto field : function->get_arglist()->get_members()) {
        auto arg_type = field->get_type();
        visit_type(arg_type, data);
      }
      auto return_type = function->get_returntype();
      visit_type(return_type, data);
    }
  }
  for (const auto object : program.get_objects()) {
    for (const auto field : object->get_members()) {
      auto ref_type = field->get_type();
      visit_type(ref_type, data);
    }
  }
  for (const auto constant : program.get_consts()) {
    const auto const_type = constant->get_type();
    visit_type(const_type, data);
  }
  for (const auto typedef_def : program.get_typedefs()) {
    const auto typedef_type = typedef_def->get_type();
    visit_type(typedef_type, data);
  }

  results.emplace("containerTypes", dump_elems(data.containers));
  results.emplace("customTemplates", dump_elems(data.custom_templates));
  results.emplace("customTypes", dump_elems(data.custom_types));

  // create second set of container types that treats strings and binaries
  // the same
  vector<const t_type*> move_containers;
  std::set<string> visited_names;

  for (const auto type : data.containers) {
    auto flat_name = flatten_type_name(*type);
    boost::algorithm::replace_all(flat_name, "binary", "string");

    if (visited_names.count(flat_name)) {
      continue;
    }
    visited_names.insert(flat_name);
    move_containers.push_back(type);
  }
  results.emplace("moveContainerTypes", dump_elems(move_containers));
}

void t_mstch_py3_generator::add_cpp_includes(
    const t_program& program,
    mstch::map& results) {
  mstch::array a{};
  for (auto const& include : program.get_cpp_includes()) {
    mstch::map cpp_include;
    cpp_include.emplace(
        "system?", include.at(0) == '<' ? std::to_string(0) : "");
    cpp_include.emplace("path", std::string(include));
    a.push_back(cpp_include);
  }
  results.emplace("cppIncludes", a);
}

void t_mstch_py3_generator::visit_type(t_type* orig_type, type_data& data) {
  auto type = &resolve_typedef(*orig_type);

  if (type->is_list()) {
    const auto elem_type = dynamic_cast<const t_list*>(type)->get_elem_type();
    visit_type(elem_type, data);
  } else if (type->is_set()) {
    const auto elem_type = dynamic_cast<const t_set*>(type)->get_elem_type();
    visit_type(elem_type, data);
  } else if (type->is_map()) {
    const auto map_type = dynamic_cast<const t_map*>(type);
    const auto key_type = map_type->get_key_type();
    const auto value_type = map_type->get_val_type();
    visit_type(key_type, data);
    visit_type(value_type, data);
  }

  visit_single_type(*type, data);
}

void t_mstch_py3_generator::visit_single_type(
    const t_type& type,
    type_data& data) {
  if (type.is_container()) {
    string flat_name = flatten_type_name(type);
    if (!data.container_names.count(flat_name)) {
      data.container_names.insert(flat_name);
      data.containers.push_back(&type);
    }
  }

  string cpp_template = this->get_cpp_template(type);
  if (!this->is_default_template(cpp_template, type) &&
      !data.custom_template_names.count(cpp_template)) {
    data.custom_template_names.insert(cpp_template);
    data.custom_templates.push_back(&type);
  }

  string cpp_type = this->get_cpp_type(type);
  if (cpp_type != "" && !data.custom_type_names.count(cpp_type)) {
    data.custom_type_names.insert(cpp_type);
    data.custom_types.push_back(&type);
  }
}

std::string t_mstch_py3_generator::flatten_type_name(const t_type& orig_type) {
  auto& type = resolve_typedef(orig_type);

  string cpp_template = this->get_cpp_template(type);
  string custom_prefix = "";
  if (!this->is_default_template(cpp_template, type)) {
    custom_prefix = this->to_cython_template(cpp_template) + "__";
  }

  if (type.is_list()) {
    return custom_prefix + "List__" +
        flatten_type_name(*dynamic_cast<const t_list&>(type).get_elem_type());
  } else if (type.is_set()) {
    return custom_prefix + "Set__" +
        flatten_type_name(*dynamic_cast<const t_set&>(type).get_elem_type());
  } else if (type.is_map()) {
    return (
        custom_prefix + "Map__" +
        flatten_type_name(*dynamic_cast<const t_map&>(type).get_key_type()) +
        "_" +
        flatten_type_name(*dynamic_cast<const t_map&>(type).get_val_type()));
  } else if (type.is_binary()) {
    return custom_prefix + "binary";
  } else {
    return custom_prefix + type.get_name();
  }
}

mstch::array t_mstch_py3_generator::get_cpp2_namespace(
    const t_program& program) {
  auto cpp_namespace = program.get_namespace("cpp2");
  if (cpp_namespace == "") {
    cpp_namespace = program.get_namespace("cpp");
    if (cpp_namespace == "") {
      cpp_namespace = "cpp2";
    } else {
      cpp_namespace = cpp_namespace + ".cpp2";
    }
  }
  vector<string> ns = split_namespace(cpp_namespace);
  return dump_elems(ns);
}

mstch::array t_mstch_py3_generator::get_py3_namespace(
    const t_program& program,
    const string& tail) {
  const auto& py3_namespace = program.get_namespace("py3");
  vector<string> ns = split_namespace(py3_namespace);
  if (tail != "") {
    ns.push_back(tail);
  }
  return dump_elems(ns);
}

void t_mstch_py3_generator::generate_program() {
  mstch::config::escape = [](const std::string& s) { return s; };
  generate_init_files(*get_program());
  generate_structs(*get_program());
  generate_services(*get_program());
  generate_clients(*get_program());
}

THRIFT_REGISTER_GENERATOR(
  mstch_py3,
  "Python 3",
  "    include_prefix:  Use full include paths in generated files.\n"
);
}
