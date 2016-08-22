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

class t_mstch_html_generator : public t_mstch_generator {
public:
  t_mstch_html_generator(t_program *program,
                      const std::map<std::string, std::string>& parsed_options,
                      const std::string& option_string);

  void generate_program() override;

};

t_mstch_html_generator::t_mstch_html_generator(
    t_program *program,
    const std::map<std::string, std::string>& parsed_options,
    const std::string& option_string) :
      t_mstch_generator(program, "html", parsed_options) {

  this->out_dir_base_ = "gen-mstch_html";
}

void t_mstch_html_generator::generate_program() {
  mstch::map root = this->dump(*this->get_program());

  // Generate index.html
  auto index_tpl = this->get_template("index.html");
  auto index = mstch::render(index_tpl, root);
  this->write_output("index.html", index);

}

THRIFT_REGISTER_GENERATOR(mstch_html, "HTML", "");
