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
  mstch::map extend_type(const t_type& t) const override;
  mstch::map extend_enum(const t_enum&) const override;
  mstch::map extend_const(const t_const&) const override;

 private:
  bool get_is_eb(const t_function& fn) const;
  bool get_is_stack_args() const;
  void generate_constants(const t_program& program);
  void generate_structs(const t_program& program);
  void generate_service(t_service* service);

  mstch::array get_namespace(const t_program& program) const;
  std::string get_include_prefix(const t_program& program) const;
  const t_type* resolve_typedef(const t_type* type) const;

  std::unique_ptr<std::string> include_prefix_;
  std::vector<std::array<std::string, 3>> protocols_;
  bool use_proxy_accessors_;
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
  use_proxy_accessors_ = this->get_option("proxy_accessors") != nullptr;
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

  mstch::array cpp_includes{};
  for (auto const& s : program.get_cpp_includes()) {
    mstch::map cpp_include;
    cpp_include.emplace("system?", s.at(0) == '<' ? std::to_string(0) : "");
    cpp_include.emplace("path", std::string(s));
    cpp_includes.push_back(cpp_include);
  }

  m.emplace("namespace_cpp2", this->get_namespace(program)),
  m.emplace("normalizedIncludePrefix", this->get_include_prefix(program));
  m.emplace("enums?", !program.get_enums().empty());
  m.emplace("thrift_includes", this->dump_elems(program.get_includes()));
  m.emplace("cpp_includes", cpp_includes);
  return m;
}

mstch::map t_mstch_cpp2_generator::extend_service(const t_service& svc) const {
  mstch::map m;
  m.emplace("programName", svc.get_program()->get_name());
  m.emplace(
      "programIncludePrefix", this->get_include_prefix(*svc.get_program()));
  m.emplace(
      "separate_processmap", (bool)this->get_option("separate_processmap"));
  m.emplace(
      "thrift_includes", this->dump_elems(svc.get_program()->get_includes()));
  m.emplace("namespace_cpp2", this->get_namespace(*svc.get_program()));

  mstch::array protocol_array{};
  for (auto it = protocols_.begin(); it != protocols_.end(); ++it) {
    mstch::map protocol;
    protocol.emplace("protocol:name", it->at(0));
    protocol.emplace("protocol:longName", it->at(1));
    protocol.emplace("protocol:enum", it->at(2));
    protocol_array.push_back(protocol);
  }
  add_first_last(protocol_array);
  m.emplace("protocols", protocol_array);

  mstch::array oneway_functions_array{};
  for (auto fn : svc.get_functions()) {
    if (fn->is_oneway()) {
      oneway_functions_array.push_back(this->dump(*fn));
    }
  }
  add_first_last(oneway_functions_array);
  m.emplace("onewayfunctions", oneway_functions_array);

  mstch::array cpp_includes{};
  for (auto const& s : svc.get_program()->get_cpp_includes()) {
    mstch::map cpp_include;
    cpp_include.emplace("system?", s.at(0) == '<' ? std::to_string(0) : "");
    cpp_include.emplace("path", std::string(s));
    cpp_includes.push_back(cpp_include);
  }
  m.emplace("cpp_includes", cpp_includes);

  return m;
}

mstch::map t_mstch_cpp2_generator::extend_function(const t_function& fn) const {
  mstch::map m;

  m.emplace("eb?", this->get_is_eb(fn));
  m.emplace("stackArgs?", this->get_is_stack_args());
  return m;
}

mstch::map t_mstch_cpp2_generator::extend_struct(const t_struct& s) const {
  mstch::map m;
  m.emplace("namespaces", this->get_namespace(*s.get_program()));
  m.emplace("proxy_accessors?", this->use_proxy_accessors_);

  std::vector<t_field*> s_members = s.get_members();

  // Check if the struct contains any base field
  auto const has_base_field = [&] {
    for (auto const& field : s.get_members()) {
      if (resolve_typedef(field->get_type())->is_base_type()) {
        return std::to_string(0);
      }
    }
    return std::string();
  }();
  m.emplace("base_fields?", has_base_field);

  // Filter fields according to the following criteria:
  // Get all base_types but strings (empty and non-empty)
  // Get all non empty strings
  // Get all non empty containers
  // Get all enums
  std::vector<t_field*> filtered_fields;
  std::copy_if(
      s_members.begin(),
      s_members.end(),
      std::back_inserter(filtered_fields),
      [&](t_field* f) {
        const t_type* t = resolve_typedef(f->get_type());
        return (t->is_base_type() && !t->is_string()) ||
            (t->is_string() && f->get_value() != nullptr) ||
            (t->is_container() && f->get_value() != nullptr) || t->is_enum();
      });
  m.emplace("filtered_fields", this->dump_elems(filtered_fields));

  // Check if all the struct elements:
  // Are only containers (list, map, set) that recursively end up in
  // base types or enums
  std::function<bool(t_type const*)> is_orderable = [&](t_type const* type) {
    if (type->is_base_type()) {
      return true;
    }
    if (type->is_enum()) {
      return true;
    }
    if (type->is_struct()) {
      return true;
    }
    if (type->is_list()) {
      return is_orderable(dynamic_cast<t_list const*>(type)->get_elem_type());
    }
    if (type->is_set()) {
      return is_orderable(dynamic_cast<t_set const*>(type)->get_elem_type());
    }
    if (type->is_map()) {
      return is_orderable(dynamic_cast<t_map const*>(type)->get_key_type()) &&
          is_orderable(dynamic_cast<t_map const*>(type)->get_val_type());
    }
    return false;
  };
  auto const is_struct_orderable =
      std::all_of(s.get_members().begin(), s.get_members().end(), [&](auto m) {
        return is_orderable(m->get_type());
      });
  if (is_struct_orderable) {
    m.emplace("is_struct_orderable?", std::to_string(0));
  }

  return m;
}

mstch::map t_mstch_cpp2_generator::extend_type(const t_type& t) const {
  mstch::map m;

  m.emplace("resolves_to_base?", resolve_typedef(&t)->is_base_type());
  m.emplace(
      "resolves_to_base_or_enum?",
      resolve_typedef(&t)->is_base_type() || resolve_typedef(&t)->is_enum());
  m.emplace("resolves_to_container?", resolve_typedef(&t)->is_container());
  m.emplace(
      "resolves_to_container_or_struct?",
      resolve_typedef(&t)->is_container() || resolve_typedef(&t)->is_struct());
  m.emplace(
      "resolves_to_container_or_enum?",
      resolve_typedef(&t)->is_container() || resolve_typedef(&t)->is_enum());
  m.emplace(
      "resolves_to_complex_return?",
      resolve_typedef(&t)->is_container() || resolve_typedef(&t)->is_string() ||
          resolve_typedef(&t)->is_struct() || resolve_typedef(&t)->is_stream());

  return m;
}

mstch::map t_mstch_cpp2_generator::extend_enum(const t_enum& e) const {
  mstch::map m;

  m.emplace("empty?", e.get_constants().empty());
  m.emplace("size", std::to_string(e.get_constants().size()));

  // Obtain the name of the minimum and maximum enum
  const std::vector<t_enum_value*> e_members = e.get_constants();
  if (!e_members.empty()) {
    auto e_minmax = std::minmax_element(
        e_members.begin(),
        e_members.end(),
        [](t_enum_value* a, t_enum_value* b) {
          return a->get_value() < b->get_value();
        });
    m.emplace("min", std::string((*e_minmax.first)->get_name()));
    m.emplace("max", std::string((*e_minmax.second)->get_name()));
  }
  return m;
}

mstch::map t_mstch_cpp2_generator::extend_const(const t_const& c) const {
  mstch::map m;

  if (c.get_type()->is_enum()) {
    auto e = static_cast<const t_enum&>(*c.get_type());
    auto e_val = e.find_value(c.get_value()->get_integer());
    m.emplace("enum_value?", e_val != nullptr);
    m.emplace(
        "enum_value",
        e_val != nullptr ? std::string(e_val->get_name())
                         : std::to_string(c.get_value()->get_integer()));
  }

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
  render_to_file(*service, "service.cpp", name + ".cpp");
  render_to_file(*service, "service.h", name + ".h");
  render_to_file(*service, "service.tcc", name + ".tcc");
  render_to_file(*service, "service_client.cpp", name + "_client.cpp");
  render_to_file(
      *service, "types_custom_protocol.h", name + "_custom_protocol.h");

  for (const auto& protocol : protocols_) {
    auto m = dump(*service);
    m.emplace("protocol:name", protocol.at(0));
    m.emplace("protocol:longName", protocol.at(1));
    m.emplace("protocol:enum", protocol.at(2));
    render_to_file(
        m,
        "service_processmap_protocol.cpp",
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

const t_type* t_mstch_cpp2_generator::resolve_typedef(const t_type* t) const {
  while (t->is_typedef()) {
    t = dynamic_cast<const t_typedef*>(t)->get_type();
  }
  return t;
}
}

THRIFT_REGISTER_GENERATOR(mstch_cpp2, "cpp2", "");
