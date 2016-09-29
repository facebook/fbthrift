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

#include <boost/algorithm/string.hpp>

namespace {

class t_mstch_cpp2_generator : public t_mstch_generator {
 public:
  t_mstch_cpp2_generator(
      t_program* program,
      const std::map<std::string, std::string>& parsed_options,
      const std::string& /*option_string*/);

  void generate_program() override;

 protected:
  mstch::map extend_service(const t_service&) const override;
  mstch::map extend_function(const t_function&) const override;
  mstch::map extend_struct(const t_struct&) const override;
  mstch::map extend_enum(const t_enum&) const override;

 private:
  bool get_is_eb(const t_function& fn) const;
  bool get_is_complex_return_type(const t_function& fn) const;
  bool get_is_stack_args() const;
  void generate_service(t_service* service);

  mstch::array get_namespace(const t_program& program) const;
  std::string get_include_prefix(const t_program& program) const;

  bool use_include_prefix_ = false;
};

t_mstch_cpp2_generator::t_mstch_cpp2_generator(
    t_program* program,
    const std::map<std::string, std::string>& parsed_options,
    const std::string& /*option_string*/)
    : t_mstch_generator(program, "cpp2", parsed_options, true) {
  // TODO: use gen-cpp2 when this implementation is ready to replace the
  // old python implementation.
  this->out_dir_base_ = "gen-mstch_cpp2";

  auto include_prefix = this->get_option("include_prefix");
  if (include_prefix) {
    use_include_prefix_ = true;
    if (*include_prefix != "") {
      program->set_include_prefix(*include_prefix);
    }
  }
}

void t_mstch_cpp2_generator::generate_program() {
  // disable mstch escaping
  mstch::config::escape = [](const std::string& s) { return s; };

  auto services = this->get_program()->get_services();
  auto root = this->dump(*this->get_program());

  // Generate client_interface_tpl
  for (const auto& service : services ) {
    this->generate_service(service);
  }
}

mstch::map t_mstch_cpp2_generator::extend_service(const t_service& svc) const {
  const std::vector<std::pair<std::string, std::string>> protocols = {
    {"binary", "BinaryProtocol"},
    {"compact", "CompactProtocol"},
  };

  mstch::array v{};
  for (auto it = protocols.begin(); it != protocols.end(); ++it) {
    mstch::map m;
    m.emplace("protocol:name", it->first);
    m.emplace("protocol:longName", it->second);
    v.push_back(m);
  }
  add_first_last(v);
  return mstch::map {
    {"protocols", v},
    {"programName", svc.get_program()->get_name()},
    {"programIncludePrefix", this->get_include_prefix(*svc.get_program())},
    {"namespaces", this->get_namespace(*svc.get_program())},
  };
}

mstch::map t_mstch_cpp2_generator::extend_function(const t_function& fn) const {
  return mstch::map {
    {"eb?", this->get_is_eb(fn)},
    {"complexReturnType?", this->get_is_complex_return_type(fn)},
    {"stackArgs?", this->get_is_stack_args()},
  };
}

mstch::map t_mstch_cpp2_generator::extend_struct(const t_struct& s) const {
  return mstch::map {
    {"namespaces", this->get_namespace(*s.get_program())},
  };
}

mstch::map t_mstch_cpp2_generator::extend_enum(const t_enum& e) const {
  return mstch::map {
    {"namespaces", this->get_namespace(*e.get_program())},
  };
}

bool t_mstch_cpp2_generator::get_is_eb(const t_function& fn) const {
  auto annotations = fn.get_annotations();
  if (annotations) {
    auto it = annotations->annotations_.find("thread");
    return it != annotations->annotations_.end() && it->second == "eb";
  }
  return false;
}

bool t_mstch_cpp2_generator::get_is_complex_return_type(
    const t_function& fn) const {
  auto rt = fn.get_returntype();
  return rt->is_string() ||
      rt->is_struct() ||
      rt->is_container() ||
      rt->is_stream();
}

bool t_mstch_cpp2_generator::get_is_stack_args() const {
  return this->get_option("stack_arguments").hasValue();
}

void t_mstch_cpp2_generator::generate_service(t_service* service) {
  auto path = boost::filesystem::path(service->get_name() + ".h");
  render_to_file(*service, "Service.h", path);
}

mstch::array t_mstch_cpp2_generator::get_namespace(
    const t_program& program) const {
  std::vector<std::string> v;

  auto ns = program.get_namespace("cpp2");
  if (ns != "") {
    boost::split(v, ns, boost::is_any_of("."));
  } else {
    ns = program.get_namespace("cpp");
    if (ns != "") {
      boost::split(v, ns, boost::is_any_of("."));
    }
    v.push_back("cpp2");
  }
  mstch::array a;
  for (auto it = v.begin(); it != v.end(); ++it) {
    mstch::map m;
    m.emplace("namespace:name", *it);
    a.push_back(m);
  }
  add_first_last(a);
  return a;
}

std::string t_mstch_cpp2_generator::get_include_prefix(
    const t_program& program) const {
  string include_prefix = program.get_include_prefix();
  auto path = boost::filesystem::path(include_prefix);
  if (!use_include_prefix_ || path.is_absolute()) {
    return "";
  }

  if (!path.has_stem()) {
    return "";
  }
  if (get_program()->is_out_path_absolute()) {
    return path.string();
  }
  return (path / "gen-cpp2").string() + "/";
}

}

THRIFT_REGISTER_GENERATOR(mstch_cpp2, "cpp2", "");
