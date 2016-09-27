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

#include <thrift/compiler/generate/t_mstch_generator.h>

#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>

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
        program->set_include_prefix(include_prefix.value());
      }
    }

    void generate_program() override;
    mstch::map extend_program(const t_program&) const override;

  protected:
    void generate_structs(const t_program&);
    void generate_services(const t_program&);
    void generate_promises(const t_program&);
};

mstch::map t_mstch_py3_generator::extend_program(const t_program& program) const {
  auto cpp_namespace = program.get_namespace("cpp2");
  if (cpp_namespace == "") {
    cpp_namespace = program.get_namespace("cpp");
    if (cpp_namespace == "") {
      cpp_namespace = "cpp2";
    }
    else {
      cpp_namespace = cpp_namespace + "cpp2";
    }
  }
  vector<string> ns;
  boost::algorithm::split(ns, cpp_namespace, boost::algorithm::is_any_of("."));

  mstch::map result {
    {"cppNamespaces?", !cpp_namespace.empty()},
    {"cppNamespaces", this->dump_elems(ns)},
  };
  return result;
}

void t_mstch_py3_generator::generate_structs(const t_program& program) {
  auto basename = program.get_name() + "_types";
  this->render_to_file(program, "Struct.pxd", basename + ".pxd");
  this->render_to_file(program, "Struct.pxi", basename + ".pxi");
}

void t_mstch_py3_generator::generate_services(const t_program& program) {
  auto name = this->get_program()->get_name();
  this->render_to_file(program, "Services.pxd", name + "_services.pxd");
  this->render_to_file(
    program, "ServiceCallbacks.pxi", name + "_callbacks.pxi");
  auto basename = name + "_services_wrapper";
  this->render_to_file(program, "ServicesWrapper.h", basename + ".h");
  this->render_to_file(program, "ServicesWrapper.cpp", basename + ".cpp");
  this->render_to_file(program, "ServicesWrapper.pxd", basename + ".pxd");
  this->render_to_file(program, "ServiceCallbacks.pxi", basename + ".pxi");
  this->render_to_file(program, "CythonServer.pyx", name + "_server.pyx");

}

void t_mstch_py3_generator::generate_promises(const t_program& program) {
  mstch::array distinct_return_types;
  std::set<string> visited_names;

  for (const auto service : program.get_services()) {
    for (const auto function : service->get_functions()) {
      const auto returntype = function->get_returntype();
      if (!visited_names.count(returntype->get_name())) {
        distinct_return_types.push_back(this->dump(*returntype));
        visited_names.insert(returntype->get_name());
      }
    }
  }

  mstch::map context {
    {"returnTypes", distinct_return_types},
  };

  auto rendered_context = this->render("CythonPromises", context);
  this->write_output(this->get_program()->get_name() + "_promises.pxi", rendered_context);
}

void t_mstch_py3_generator::generate_program() {
  mstch::config::escape = [](const std::string& s) { return s; };
  this->generate_structs(*this->get_program());
  this->generate_services(*this->get_program());
  this->generate_promises(*this->get_program());
}

THRIFT_REGISTER_GENERATOR(
  mstch_py3,
  "Python 3",
  "    include_prefix:  Use full include paths in generated files.\n"
);
}
