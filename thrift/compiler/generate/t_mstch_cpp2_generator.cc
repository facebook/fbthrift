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
#include <array>
#include <memory>
#include <vector>

#include <thrift/compiler/generate/common.h>
#include <thrift/compiler/generate/t_mstch_generator.h>

namespace {

class t_mstch_cpp2_generator : public t_mstch_generator {
 public:
  t_mstch_cpp2_generator(
      t_program* program,
      const std::map<std::string, std::string>& parsed_options,
      const std::string& /*option_string*/);

  void generate_program() override;

 protected:
  mstch::map extend_function(const t_function&) const override;
  mstch::map extend_program(const t_program&) const override;
  mstch::map extend_service(const t_service&) const override;
  mstch::map extend_struct(const t_struct&) const override;
  mstch::map extend_type(const t_type& t, const int32_t depth) const override;

 private:
  bool get_is_eb(const t_function& fn) const;
  bool get_is_complex_return_type(const t_function& fn) const;
  bool get_is_stack_args() const;
  void generate_constants(const t_program& program);
  void generate_structs(const t_program& program);
  void generate_service(t_service* service);

  mstch::array get_namespace(const t_program& program) const;
  std::string get_include_prefix(const t_program& program) const;

  std::unique_ptr<std::string> include_prefix_;
  std::vector<std::array<std::string, 3>> protocols_;
};

t_mstch_cpp2_generator::t_mstch_cpp2_generator(
    t_program* program,
    const std::map<std::string, std::string>& parsed_options,
    const std::string& /*option_string*/)
    : t_mstch_generator(program, "cpp2", parsed_options, true) {
  // TODO: use gen-cpp2 when this implementation is ready to replace the
  // old python implementation.
  this->out_dir_base_ = "gen-mstch_cpp2";
  this->protocols_ = {
    {{"binary", "BinaryProtocol", "T_BINARY_PROTOCOL"}},
    {{"compact", "CompactProtocol", "T_COMPACT_PROTOCOL"}},
  };

  include_prefix_ = this->get_option("include_prefix");
}

void t_mstch_cpp2_generator::generate_program() {
  // disable mstch escaping
  mstch::config::escape = [](const std::string& s) { return s; };

  auto services = this->get_program()->get_services();
  auto root = this->dump(*this->get_program());

  this->generate_constants(*this->get_program());
  this->generate_structs(*this->get_program());

  // Generate client_interface_tpl
  for (const auto& service : services ) {
    this->generate_service(service);
  }
}

mstch::map t_mstch_cpp2_generator::extend_program(
    const t_program& program) const {
  mstch::map m;
  m.emplace("namespace_cpp2", this->get_namespace(program)),
  m.emplace("normalizedIncludePrefix", this->get_include_prefix(program));
  return m;
}

mstch::map t_mstch_cpp2_generator::extend_service(const t_service& svc) const {
  mstch::array protocol_array{};
  for (auto it = protocols_.begin(); it != protocols_.end(); ++it) {
    mstch::map m;
    m.emplace("protocol:name", it->at(0));
    m.emplace("protocol:longName", it->at(1));
    m.emplace("protocol:enum", it->at(2));
    protocol_array.push_back(m);
  }
  add_first_last(protocol_array);

  mstch::array oneway_functions_array{};
  for (auto fn : svc.get_functions()) {
    if (fn->is_oneway()) {
      oneway_functions_array.push_back(this->dump(*fn));
    }
  }
  add_first_last(oneway_functions_array);

  return mstch::map {
    {"onewayfunctions", oneway_functions_array},
    {"protocols", protocol_array},
    {"programName", svc.get_program()->get_name()},
    {"programIncludePrefix", this->get_include_prefix(*svc.get_program())},
    {"separate_processmap", (bool)this->get_option("separate_processmap")},
    {"thriftIncludes", this->dump_elems(svc.get_program()->get_includes())},
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
  mstch::map m;
  m.emplace("namespaces", this->get_namespace(*s.get_program()));

  std::vector<t_field*> s_members = s.get_members();

  // obtain base types
  std::vector<t_field*> base_members;
  std::copy_if(
    s_members.begin(),
    s_members.end(),
    std::back_inserter(base_members),
    [](t_field* f){ return f->get_type()->is_base_type(); }
  );
  m.emplace("base_fields?", !base_members.empty());
  m.emplace("base_fields", this->dump_elems(base_members));

 // obtain container types
  std::vector<t_field*> container_members;
  std::copy_if(
    s_members.begin(),
    s_members.end(),
    std::back_inserter(container_members),
    [](t_field* f){ return f->get_type()->is_container(); }
  );
  m.emplace("container_fields", this->dump_elems(container_members));

  // Filter fields according to the following criteria:
  // Get all base_types but strings (empty and non-empty)
  // Get all non empty strings
  // Get all non empty containers
  std::vector<t_field*> filtered_fields;
  std::copy_if(
    s_members.begin(),
    s_members.end(),
    std::back_inserter(filtered_fields),
    [](t_field* f){
      return (f->get_type()->is_base_type() && !f->get_type()->is_string()) ||
      (f->get_type()->is_string() && f->get_value() != nullptr) ||
      (f->get_type()->is_container() && f->get_value() != nullptr);
  });
  m.emplace("filtered_fields", this->dump_elems(filtered_fields));

  return m;
}

mstch::map t_mstch_cpp2_generator::extend_type(
    const t_type& t,
    const int32_t depth) const {
  mstch::map m;

  // Indent recursive code generation
  m.emplace("indent2", std::string(depth * 2, ' '));
  m.emplace("indent4", std::string(depth * 4, ' '));

  // Determine if you are inside a container
  if (depth > 0) {
    m.emplace("inner_container?", std::to_string(depth));
  }

  auto const cxx_value_prefix = [&] {
    using TypeValue = t_types::TypeValue;
    switch (t.get_type_value()) {
      case TypeValue::TYPE_BYTE: return std::string("static_cast<int8_t>(");
      case TypeValue::TYPE_I16: return std::string("static_cast<int16_t>(");
      default: return std::string();
    }
  }();
  m.emplace("cxx_value_prefix", cxx_value_prefix);

  auto const cxx_value_suffix = [&] {
    using TypeValue = t_types::TypeValue;
    switch (t.get_type_value()) {
      case TypeValue::TYPE_BYTE: return std::string(")");
      case TypeValue::TYPE_I16: return std::string(")");
      case TypeValue::TYPE_I64: return std::string("LL");
      default: return std::string();
    }
  }();
  m.emplace("cxx_value_suffix", cxx_value_suffix);

  return m;
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
  return this->get_option("stack_arguments") != nullptr;
}

void t_mstch_cpp2_generator::generate_constants(const t_program& program) {
  auto name = program.get_name();
  this->render_to_file(program, "module_constants.h", name + "_constants.h");
  this->render_to_file(
    program,
    "module_constants.cpp",
    name + "_constants.cpp"
  );
}

void t_mstch_cpp2_generator::generate_structs(const t_program& program) {
  auto name = program.get_name();
  this->render_to_file(program, "module_data.h", name + "_data.h");
  this->render_to_file(program, "module_data.cpp", name + "_data.cpp");
  this->render_to_file(program, "module_types.h", name + "_types.h");
  this->render_to_file(program, "module_types.tcc", name + "_types.tcc");
  this->render_to_file(program, "module_types.cpp", name + "_types.cpp");
  this->render_to_file(
    program,
    "module_types_custom_protocol.h",
    name + "_types_custom_protocol.h"
  );
}

void t_mstch_cpp2_generator::generate_service(t_service* service) {
  auto name = service->get_name();
  render_to_file(*service, "Service.cpp", name + ".cpp");
  render_to_file(*service, "Service.h", name + ".h");
  render_to_file(*service, "Service_client.cpp", name + "_client.cpp");
  render_to_file(*service,
                 "Service_custom_protocol.h",
                 name + "_custom_protocol.h");

  for (const auto& protocol : protocols_) {
    auto m = dump(*service);
    m.emplace("protocol:name", protocol.at(0));
    m.emplace("protocol:longName", protocol.at(1));
    m.emplace("protocol:enum", protocol.at(2));
    render_to_file(
        m,
        "Service_processmap_protocol.cpp",
        name + "_processmap_" + protocol.at(0) + ".cpp");
  }
}

mstch::array t_mstch_cpp2_generator::get_namespace(
    const t_program& program) const {
  std::vector<std::string> v;

  auto ns = program.get_namespace("cpp2");
  if (ns != "") {
    v = split_namespace(ns);
  } else {
    ns = program.get_namespace("cpp");
    if (ns != "") {
      v = split_namespace(ns);
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
  if (&program == this->get_program() &&
      include_prefix_ && *include_prefix_ != "") {
    include_prefix = *include_prefix_;
  }
  auto path = boost::filesystem::path(include_prefix);
  if (!include_prefix_ || path.is_absolute()) {
    return "";
  }

  if (!path.has_stem()) {
    return "";
  }
  if (program.is_out_path_absolute()) {
    return path.string();
  }
  return (path / "gen-cpp2").string() + "/";
}

}

THRIFT_REGISTER_GENERATOR(mstch_cpp2, "cpp2", "");
