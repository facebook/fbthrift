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
#include <thrift/compiler/generate/t_mstch_objects.h>

namespace {

class t_mstch_cpp2_generator : public t_mstch_generator {
 public:
  t_mstch_cpp2_generator(
      t_program* program,
      const std::map<std::string, std::string>& parsed_options,
      const std::string& /*option_string*/);

  void generate_program() override;

 protected:
  mstch::map extend_function(const t_function&) override;
  mstch::map extend_program(const t_program&) override;
  mstch::map extend_service(const t_service&) override;
  mstch::map extend_struct(const t_struct&) override;
  mstch::map extend_type(const t_type& t) override;
  mstch::map extend_enum(const t_enum&) override;
  mstch::map extend_const(const t_const&) override;

 private:
  bool get_is_eb(const t_function& fn);
  bool get_is_stack_args();
  void generate_constants(const t_program& program);
  void generate_structs(const t_program& program);
  void generate_service(t_service* service);

  mstch::array get_namespace(const t_program& program);
  std::string get_include_prefix(const t_program& program);
  bool has_annotation(const t_field* f, const std::string& name);

  std::unique_ptr<std::string> include_prefix_;
  std::vector<std::array<std::string, 3>> protocols_;
  bool use_proxy_accessors_;
  bool use_getters_setters_;
};

class mstch_cpp2_enum : public mstch_enum {
 public:
  mstch_cpp2_enum(
      t_enum const* enm,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos)
      : mstch_enum(enm, generators, cache, pos) {
    register_methods(
        this,
        {
            {"enum:empty?", &mstch_cpp2_enum::is_empty},
            {"enum:size", &mstch_cpp2_enum::size},
            {"enum:min", &mstch_cpp2_enum::min},
            {"enum:max", &mstch_cpp2_enum::max},
        });
  }
  mstch::node is_empty() {
    return enm_->get_constants().empty();
  }
  mstch::node size() {
    return std::to_string(enm_->get_constants().size());
  }
  mstch::node min() {
    if (!enm_->get_constants().empty()) {
      auto e_min = std::min_element(
          enm_->get_constants().begin(),
          enm_->get_constants().end(),
          [](t_enum_value* a, t_enum_value* b) {
            return a->get_value() < b->get_value();
          });
      return (*e_min)->get_name();
    }
    return mstch::node();
  }
  mstch::node max() {
    if (!enm_->get_constants().empty()) {
      auto e_max = std::max_element(
          enm_->get_constants().begin(),
          enm_->get_constants().end(),
          [](t_enum_value* a, t_enum_value* b) {
            return a->get_value() < b->get_value();
          });
      return (*e_max)->get_name();
    }
    return mstch::node();
  }
};

class mstch_cpp2_type : public mstch_type {
 public:
  mstch_cpp2_type(
      t_type const* type,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos)
      : mstch_type(type, generators, cache, pos),
        resolved_type_(resolve_typedef(type)) {
    register_methods(
        this,
        {
            {"type:resolves_to_base?", &mstch_cpp2_type::resolves_to_base},
            {"type:resolves_to_base_or_enum?",
             &mstch_cpp2_type::resolves_to_base_or_enum},
            {"type:resolves_to_container?",
             &mstch_cpp2_type::resolves_to_container},
            {"type:resolves_to_container_or_struct?",
             &mstch_cpp2_type::resolves_to_container_or_struct},
            {"type:resolves_to_container_or_enum?",
             &mstch_cpp2_type::resolves_to_container_or_enum},
            {"type:resolves_to_complex_return?",
             &mstch_cpp2_type::resolves_to_complex_return},
        });
  }
  virtual std::string get_type_namespace(t_program const* program) override {
    auto ns = program->get_namespace("cpp2");
    if (ns.empty()) {
      ns = program->get_namespace("cpp");
      if (ns.empty()) {
        ns = "cpp2";
      }
    }
    return ns;
  }
  mstch::node resolves_to_base() {
    return resolved_type_->is_base_type();
  }
  mstch::node resolves_to_base_or_enum() {
    return resolved_type_->is_base_type() || resolved_type_->is_enum();
  }
  mstch::node resolves_to_container() {
    return resolved_type_->is_container();
  }
  mstch::node resolves_to_container_or_struct() {
    return resolved_type_->is_container() || resolved_type_->is_struct();
  }
  mstch::node resolves_to_container_or_enum() {
    return resolved_type_->is_container() || resolved_type_->is_enum();
  }
  mstch::node resolves_to_complex_return() {
    return resolved_type_->is_container() || resolved_type_->is_string() ||
        resolved_type_->is_struct();
  }

 protected:
  t_type const* resolved_type_;
};

class mstch_cpp2_struct : public mstch_struct {
 public:
  mstch_cpp2_struct(
      t_struct const* strct,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos)
      : mstch_struct(strct, generators, cache, pos) {
    register_methods(
        this,
        {
            {"struct:getters_setters?", &mstch_cpp2_struct::getters_setters},
            {"struct:base_field_or_struct?",
             &mstch_cpp2_struct::has_base_field_or_struct},
            {"struct:filtered_fields", &mstch_cpp2_struct::filtered_fields},
            {"struct:is_struct_orderable?",
             &mstch_cpp2_struct::is_struct_orderable},
        });
  }
  mstch::node getters_setters() {
    for (auto const* field : strct_->get_members()) {
      auto const* resolved_typedef = resolve_typedef(field->get_type());
      if (resolved_typedef->is_base_type() || resolved_typedef->is_struct()) {
        return true;
      }
    }
    return false;
  }
  mstch::node has_base_field_or_struct() {
    for (auto const* field : strct_->get_members()) {
      auto const* resolved_typedef = resolve_typedef(field->get_type());
      if (resolved_typedef->is_base_type() || resolved_typedef->is_struct()) {
        return true;
      }
    }
    return false;
  }
  mstch::node filtered_fields() {
    // Filter fields according to the following criteria:
    // Get all base_types but strings (empty and non-empty)
    // Get all non empty strings
    // Get all non empty containers
    // Get all enums
    std::vector<t_field const*> filtered_fields;
    for (auto const* field : strct_->get_members()) {
      const t_type* type = resolve_typedef(field->get_type());
      if ((type->is_base_type() && !type->is_string()) ||
          (type->is_string() && field->get_value() != nullptr) ||
          (type->is_container() && field->get_value() != nullptr) ||
          type->is_enum()) {
        filtered_fields.push_back(field);
      }
    }
    return generate_elements(
        filtered_fields,
        generators_->field_generator_.get(),
        generators_,
        cache_);
  }
  bool is_orderable(t_type const* type) {
    if (type->is_base_type()) {
      return true;
    }
    if (type->is_enum()) {
      return true;
    }
    if (type->is_struct()) {
      for (auto const* f : dynamic_cast<t_struct const*>(type)->get_members()) {
        if (f->get_req() == t_field::e_req::T_OPTIONAL ||
            !is_orderable(f->get_type())) {
          return false;
        }
      }
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
  }
  mstch::node is_struct_orderable() {
    return std::all_of(
        strct_->get_members().begin(),
        strct_->get_members().end(),
        [&](auto m) { return is_orderable(m->get_type()); });
  }
};

class enum_cpp2_generator : public enum_generator {
 public:
  enum_cpp2_generator() = default;
  virtual ~enum_cpp2_generator() = default;
  virtual std::shared_ptr<mstch_base> generate(
      t_enum const* enm,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t /*index*/ = 0) const override {
    return std::make_shared<mstch_cpp2_enum>(enm, generators, cache, pos);
  }
};

class type_cpp2_generator : public type_generator {
 public:
  type_cpp2_generator() = default;
  virtual ~type_cpp2_generator() = default;
  virtual std::shared_ptr<mstch_base> generate(
      t_type const* type,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t /*index*/ = 0) const override {
    return std::make_shared<mstch_cpp2_type>(type, generators, cache, pos);
  }
};

t_mstch_cpp2_generator::t_mstch_cpp2_generator(
    t_program* program,
    const std::map<std::string, std::string>& parsed_options,
    const std::string& /*option_string*/)
    : t_mstch_generator(program, "cpp2", parsed_options, true) {
  // TODO: use gen-cpp2 when this implementation is ready to replace the
  // old python implementation.
  out_dir_base_ = "gen-mstch_cpp2";
  protocols_ = {
      {{"binary", "BinaryProtocol", "T_BINARY_PROTOCOL"}},
      {{"compact", "CompactProtocol", "T_COMPACT_PROTOCOL"}},
  };

  include_prefix_ = get_option("include_prefix");
  use_proxy_accessors_ = get_option("proxy_accessors") != nullptr;
  use_getters_setters_ = get_option("disable_getters_setters") == nullptr;
}

void t_mstch_cpp2_generator::generate_program() {
  // disable mstch escaping
  mstch::config::escape = [](const std::string& s) { return s; };

  auto services = get_program()->get_services();
  auto root = dump(*get_program());

  generate_constants(*get_program());
  generate_structs(*get_program());

  // Generate client_interface_tpl
  for (const auto& service : services ) {
    generate_service(service);
  }
}

class struct_cpp2_generator : public struct_generator {
 public:
  struct_cpp2_generator() = default;
  virtual ~struct_cpp2_generator() = default;
  virtual std::shared_ptr<mstch_base> generate(
      t_struct const* strct,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t /*index*/ = 0) const override {
    return std::make_shared<mstch_cpp2_struct>(strct, generators, cache, pos);
  }
};

mstch::map t_mstch_cpp2_generator::extend_program(const t_program& program) {
  mstch::map m;

  mstch::array cpp_includes{};
  for (auto const& s : program.get_cpp_includes()) {
    mstch::map cpp_include;
    cpp_include.emplace("system?", s.at(0) == '<' ? std::to_string(0) : "");
    cpp_include.emplace("path", std::string(s));
    cpp_includes.push_back(cpp_include);
  }

  m.emplace("namespace_cpp2", get_namespace(program)),
      m.emplace("normalizedIncludePrefix", get_include_prefix(program));
  m.emplace("enums?", !program.get_enums().empty());
  m.emplace("thrift_includes", dump_elems(program.get_includes()));
  m.emplace("cpp_includes", cpp_includes);
  return m;
}

mstch::map t_mstch_cpp2_generator::extend_service(const t_service& svc) {
  mstch::map m;
  m.emplace("programName", svc.get_program()->get_name());
  m.emplace("programIncludePrefix", get_include_prefix(*svc.get_program()));
  m.emplace("separate_processmap", (bool)get_option("separate_processmap"));
  m.emplace("thrift_includes", dump_elems(svc.get_program()->get_includes()));
  m.emplace("namespace_cpp2", get_namespace(*svc.get_program()));

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
      oneway_functions_array.push_back(dump(*fn));
    }
  }
  add_first_last(oneway_functions_array);
  m.emplace("oneway_functions", oneway_functions_array);
  m.emplace("oneways?", !oneway_functions_array.empty());

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

mstch::map t_mstch_cpp2_generator::extend_function(const t_function& fn) {
  mstch::map m;

  m.emplace("eb?", get_is_eb(fn));
  m.emplace("stackArgs?", get_is_stack_args());
  return m;
}

mstch::map t_mstch_cpp2_generator::extend_struct(const t_struct& s) {
  mstch::map m;
  m.emplace("namespaces", get_namespace(*s.get_program()));
  m.emplace("proxy_accessors?", use_proxy_accessors_);
  m.emplace("getters_setters?", use_getters_setters_);

  std::vector<t_field*> s_members = s.get_members();

  // Check if the struct contains any base field
  auto const has_base_field_or_struct = [&] {
    for (auto const& field : s.get_members()) {
      auto const& resolved_typedef = resolve_typedef(*field->get_type());
      if (resolved_typedef.is_base_type() || resolved_typedef.is_struct()) {
        return true;
      }
    }
    return false;
  }();
  m.emplace("base_field_or_struct?", has_base_field_or_struct);

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
        const t_type& t = resolve_typedef(*f->get_type());
        return (t.is_base_type() && !t.is_string()) ||
            (t.is_string() && f->get_value() != nullptr) ||
            (t.is_container() && f->get_value() != nullptr) || t.is_enum();
      });
  m.emplace("filtered_fields", dump_elems(filtered_fields));

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
      for (auto const* f : dynamic_cast<t_struct const*>(type)->get_members()) {
        if (f->get_req() == t_field::e_req::T_OPTIONAL ||
            has_annotation(f, "cpp.template") || !is_orderable(f->get_type())) {
          return false;
        }
      }
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

mstch::map t_mstch_cpp2_generator::extend_type(const t_type& t) {
  mstch::map m;

  m.emplace("resolves_to_base?", resolve_typedef(t).is_base_type());
  m.emplace(
      "resolves_to_base_or_enum?",
      resolve_typedef(t).is_base_type() || resolve_typedef(t).is_enum());
  m.emplace("resolves_to_container?", resolve_typedef(t).is_container());
  m.emplace(
      "resolves_to_container_or_struct?",
      resolve_typedef(t).is_container() || resolve_typedef(t).is_struct());
  m.emplace(
      "resolves_to_container_or_enum?",
      resolve_typedef(t).is_container() || resolve_typedef(t).is_enum());
  m.emplace(
      "resolves_to_complex_return?",
      resolve_typedef(t).is_container() || resolve_typedef(t).is_string() ||
          resolve_typedef(t).is_struct() || resolve_typedef(t).is_stream());

  return m;
}

mstch::map t_mstch_cpp2_generator::extend_enum(const t_enum& e) {
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

mstch::map t_mstch_cpp2_generator::extend_const(const t_const& c) {
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

bool t_mstch_cpp2_generator::get_is_eb(const t_function& fn) {
  auto annotations = fn.get_annotations();
  if (annotations) {
    auto it = annotations->annotations_.find("thread");
    return it != annotations->annotations_.end() && it->second == "eb";
  }
  return false;
}

bool t_mstch_cpp2_generator::get_is_stack_args() {
  return get_option("stack_arguments") != nullptr;
}

void t_mstch_cpp2_generator::generate_constants(const t_program& program) {
  auto name = program.get_name();
  render_to_file(program, "module_constants.h", name + "_constants.h");
  render_to_file(program, "module_constants.cpp", name + "_constants.cpp");
}

void t_mstch_cpp2_generator::generate_structs(const t_program& program) {
  auto name = program.get_name();
  render_to_file(program, "module_data.h", name + "_data.h");
  render_to_file(program, "module_data.cpp", name + "_data.cpp");
  render_to_file(program, "module_types.h", name + "_types.h");
  render_to_file(program, "module_types.tcc", name + "_types.tcc");
  render_to_file(program, "module_types.cpp", name + "_types.cpp");
  render_to_file(
      program,
      "module_types_custom_protocol.h",
      name + "_types_custom_protocol.h");
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

mstch::array t_mstch_cpp2_generator::get_namespace(const t_program& program) {
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
    const t_program& program) {
  string include_prefix = program.get_include_prefix();
  if (&program == get_program() && include_prefix_ && *include_prefix_ != "") {
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
bool t_mstch_cpp2_generator::has_annotation(
    const t_field* f,
    const std::string& name) {
  return f->annotations_.count(name);
}
}

THRIFT_REGISTER_GENERATOR(mstch_cpp2, "cpp2", "");
