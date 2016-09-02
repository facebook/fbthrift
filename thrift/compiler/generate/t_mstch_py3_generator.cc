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
    mstch::map extend_struct(const t_struct& strct) const override;

  protected:
    void generate_structs(const vector<t_struct*>& structs);

  private:
    string _to_cpp_namespace(string cpp_namespace) const;
};

mstch::map t_mstch_py3_generator::extend_struct(const t_struct& strct) const {
  auto program = strct.get_program();
  auto cpp_namespace = program->get_namespace("cpp2");
  boost::algorithm::replace_all(cpp_namespace, ".", "::");
  mstch::map result {
    {"includePrefix", program->get_include_prefix()},
    {"programName", program->get_name()},
    {"cppNamespace", cpp_namespace}
  };
  return result;
}

void t_mstch_py3_generator::generate_structs(const vector<t_struct*>& structs) {
  for (const auto strct : structs) {
    auto struct_context = this->dump(*strct);
    auto rendered_struct = this->render("Struct_pxd", struct_context);
    auto filename = "cy_" + strct->get_program()->get_name() + "_types.pxd";
    this->write_output(filename, rendered_struct);
  }
}

void t_mstch_py3_generator::generate_program() {
  mstch::config::escape = [](const std::string& s) { return s; };
  this->generate_structs(this->get_program()->get_objects());
}

THRIFT_REGISTER_GENERATOR(
  mstch_py3,
  "Python 3",
  "    include_prefix:  Use full include paths in generated files.\n"
);
}
