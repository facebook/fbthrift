/*
 * Copyright (c) Facebook, Inc. and its affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
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

#include <boost/algorithm/string.hpp>

#include <thrift/compiler/generate/t_mstch_generator.h>
#include <thrift/compiler/generate/t_mstch_objects.h>
#include <thrift/compiler/util.h>

namespace apache {
namespace thrift {
namespace compiler {
namespace {

class t_json_experimental_generator : public t_mstch_generator {
 public:
  t_json_experimental_generator(
      t_program* program,
      t_generation_context context,
      const std::map<std::string, std::string>& parsed_options,
      const std::string& /*option_string*/);

  void generate_program() override;

 private:
  void set_mstch_generators();
};

t_json_experimental_generator::t_json_experimental_generator(
    t_program* program,
    t_generation_context context,
    const std::map<std::string, std::string>& parsed_options,
    const std::string& /*option_string*/)
    : t_mstch_generator(
          program,
          std::move(context),
          "json",
          parsed_options,
          true) {
  out_dir_base_ = "gen-json_experimental";
}

class json_experimental_program : public mstch_program {
 public:
  json_experimental_program(
      t_program const* program,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos)
      : mstch_program(program, generators, cache, pos) {
    register_methods(
        this,
        {
            {"program:py_namespace",
             &json_experimental_program::get_py_namespace},
            {"program:namespaces", &json_experimental_program::get_namespaces},
            {"program:namespaces?", &json_experimental_program::has_namespaces},
            {"program:docstring?", &json_experimental_program::has_docstring},
            {"program:docstring", &json_experimental_program::get_docstring},
            {"program:normalizedIncludePrefix",
             &json_experimental_program::include_prefix},
        });
  }
  mstch::node get_py_namespace() {
    return program_->get_namespace("py");
  }
  mstch::node get_namespaces() {
    mstch::array result;
    auto last = program_->get_namespaces().size();
    for (auto it : program_->get_namespaces()) {
      result.push_back(mstch::map{
          {"key", it.first}, {"value", it.second}, {"last?", (--last) == 0}});
    }
    return result;
  }
  mstch::node has_namespaces() {
    return !program_->get_namespaces().empty();
  }
  mstch::node has_docstring() {
    return !program_->get_doc().empty();
  }
  mstch::node get_docstring() {
    return json_quote_ascii(program_->get_doc());
  }

  mstch::node include_prefix() {
    auto prefix = program_->get_include_prefix();
    if (!prefix.empty()) {
      if (boost::filesystem::path(prefix).has_root_directory()) {
        return cache_->parsed_options_["include_prefix"];
      }
      return prefix;
    }
    return std::string();
  }
};

class program_json_experimental_generator : public program_generator {
 public:
  program_json_experimental_generator() = default;
  ~program_json_experimental_generator() override = default;
  std::shared_ptr<mstch_base> generate(
      t_program const* program,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t /*index*/ = 0) const override {
    return std::make_shared<json_experimental_program>(
        program, generators, cache, pos);
  }
};

class json_experimental_enum : public mstch_enum {
 public:
  json_experimental_enum(
      t_enum const* enm,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos)
      : mstch_enum(enm, generators, cache, pos) {
    register_methods(
        this,
        {
            {"enum:empty?", &json_experimental_enum::is_empty},
            {"enum:lineno", &json_experimental_enum::get_lineno},
            {"enum:docstring?", &json_experimental_enum::has_docstring},
            {"enum:docstring", &json_experimental_enum::get_docstring},
        });
  }
  mstch::node is_empty() {
    return enm_->get_enum_values().empty();
  }
  mstch::node get_lineno() {
    return enm_->get_lineno();
  }
  mstch::node has_docstring() {
    return enm_->has_doc();
  }
  mstch::node get_docstring() {
    return json_quote_ascii(enm_->get_doc());
  }
};

class enum_json_experimental_generator : public enum_generator {
 public:
  enum_json_experimental_generator() = default;
  ~enum_json_experimental_generator() override = default;
  std::shared_ptr<mstch_base> generate(
      t_enum const* enm,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t /*index*/ = 0) const override {
    return std::make_shared<json_experimental_enum>(
        enm, generators, cache, pos);
  }
};

class json_experimental_enum_value : public mstch_enum_value {
 public:
  json_experimental_enum_value(
      t_enum_value const* enm_value,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos)
      : mstch_enum_value(enm_value, generators, cache, pos) {
    register_methods(
        this,
        {
            {"enumValue:lineno", &json_experimental_enum_value::get_lineno},
            {"enumValue:docstring?",
             &json_experimental_enum_value::has_docstring},
            {"enumValue:docstring",
             &json_experimental_enum_value::get_docstring},
        });
  }
  mstch::node get_lineno() {
    return enm_value_->get_lineno();
  }
  mstch::node has_docstring() {
    return enm_value_->has_doc();
  }
  mstch::node get_docstring() {
    return json_quote_ascii(enm_value_->get_doc());
  }
};

class enum_value_json_experimental_generator : public enum_value_generator {
 public:
  enum_value_json_experimental_generator() = default;
  ~enum_value_json_experimental_generator() override = default;
  std::shared_ptr<mstch_base> generate(
      t_enum_value const* enm_value,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t /*index*/ = 0) const override {
    return std::make_shared<json_experimental_enum_value>(
        enm_value, generators, cache, pos);
  }
};

// trim all white spaces and commas from end (in place)
static inline void rtrim(std::string& s) {
  s.erase(
      std::find_if(
          s.rbegin(),
          s.rend(),
          [](int ch) { return !(ch == ' ' || ch == ','); })
          .base(),
      s.end());
}

static inline std::string to_string(t_const_value const* value) {
  auto stringify_list = [](const auto& value) {
    std::string result;
    for (const auto& v : value) {
      result += to_string(v) + ", ";
    }
    rtrim(result);
    return "[" + result + "]";
  };

  auto stringify_map = [](const auto& value) {
    std::string result;
    for (const auto& v : value) {
      result += to_string(v.first) + ": " + to_string(v.second) + ", ";
    }
    rtrim(result);
    return "{" + result + "}";
  };

  switch (value->get_type()) {
    case mstch_const_value::cv::CV_BOOL:
      return value->get_bool() ? "true" : "false";
    case mstch_const_value::cv::CV_INTEGER:
      return std::to_string(value->get_integer());
    case mstch_const_value::cv::CV_DOUBLE:
      return std::to_string(value->get_double());
    case mstch_const_value::cv::CV_STRING:
      return json_quote_ascii(value->get_string());
    case mstch_const_value::cv::CV_LIST:
      return stringify_list(value->get_list());
    case mstch_const_value::cv::CV_MAP:
      return stringify_map(value->get_map());
  }
  return "";
}

class json_experimental_const_value : public mstch_const_value {
 public:
  json_experimental_const_value(
      t_const_value const* const_value,
      t_const const* current_const,
      t_type const* expected_type,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos,
      int32_t index)
      : mstch_const_value(
            const_value,
            current_const,
            expected_type,
            generators,
            cache,
            pos,
            index) {
    register_methods(
        this,
        {
            {"value:lineno", &json_experimental_const_value::get_lineno},
            {"value:typeName", &json_experimental_const_value::get_typename},
            {"value:stringValueAny",
             &json_experimental_const_value::string_value_any},
            {"value:docstring?", &json_experimental_const_value::has_docstring},
            {"value:docstring", &json_experimental_const_value::get_docstring},
        });
  }
  mstch::node get_lineno() {
    return current_const_->get_lineno();
  }
  mstch::node has_docstring() {
    return current_const_->has_doc();
  }
  mstch::node get_docstring() {
    return json_quote_ascii(current_const_->get_doc());
  }

  mstch::node get_typename() {
    return current_const_->get_type()->get_true_type()->get_full_name();
  }

  mstch::node string_value_any() {
    return to_string(const_value_);
  }
};

class const_value_json_experimental_generator : public const_value_generator {
 public:
  const_value_json_experimental_generator() = default;
  ~const_value_json_experimental_generator() override = default;
  std::shared_ptr<mstch_base> generate(
      t_const_value const* const_value,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t index = 0,
      t_const const* current_const = nullptr,
      t_type const* expected_type = nullptr) const override {
    return std::make_shared<json_experimental_const_value>(
        const_value,
        current_const,
        expected_type,
        generators,
        cache,
        pos,
        index);
  }
};

class json_experimental_struct : public mstch_struct {
 public:
  json_experimental_struct(
      t_struct const* strct,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos)
      : mstch_struct(strct, generators, cache, pos) {
    register_methods(
        this,
        {
            {"struct:lineno", &json_experimental_struct::get_lineno},
            {"struct:docstring?", &json_experimental_struct::has_docstring},
            {"struct:docstring", &json_experimental_struct::get_docstring},
        });
  }
  mstch::node get_lineno() {
    return strct_->get_lineno();
  }
  mstch::node has_docstring() {
    return strct_->has_doc();
  }
  mstch::node get_docstring() {
    return json_quote_ascii(strct_->get_doc());
  }
};

class struct_json_experimental_generator : public struct_generator {
 public:
  struct_json_experimental_generator() = default;
  ~struct_json_experimental_generator() override = default;
  std::shared_ptr<mstch_base> generate(
      t_struct const* strct,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t /*index*/ = 0) const override {
    return std::make_shared<json_experimental_struct>(
        strct, generators, cache, pos);
  }
};

class json_experimental_service : public mstch_service {
 public:
  json_experimental_service(
      t_service const* srvc,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos)
      : mstch_service(srvc, generators, cache, pos) {
    register_methods(
        this,
        {
            {"service:lineno", &json_experimental_service::get_lineno},
            {"service:docstring?", &json_experimental_service::has_docstring},
            {"service:docstring", &json_experimental_service::get_docstring},
        });
  }
  mstch::node get_lineno() {
    return service_->get_lineno();
  }
  mstch::node has_docstring() {
    return service_->has_doc();
  }
  mstch::node get_docstring() {
    return json_quote_ascii(service_->get_doc());
  }
};

class service_json_experimental_generator : public service_generator {
 public:
  service_json_experimental_generator() = default;
  ~service_json_experimental_generator() override = default;
  std::shared_ptr<mstch_base> generate(
      t_service const* srvc,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t /*index*/ = 0) const override {
    return std::make_shared<json_experimental_service>(
        srvc, generators, cache, pos);
  }
};

class json_experimental_function : public mstch_function {
 public:
  json_experimental_function(
      t_function const* func,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos)
      : mstch_function(func, generators, cache, pos) {
    register_methods(
        this,
        {
            {"function:lineno", &json_experimental_function::get_lineno},
            {"function:docstring?", &json_experimental_function::has_docstring},
            {"function:docstring", &json_experimental_function::get_docstring},
            {"function:args?", &json_experimental_function::has_args},
        });
  }
  mstch::node get_lineno() {
    return function_->get_lineno();
  }
  mstch::node has_docstring() {
    return function_->has_doc();
  }
  mstch::node get_docstring() {
    return json_quote_ascii(function_->get_doc());
  }
  mstch::node has_args() {
    return !function_->get_arglist()->get_members().empty();
  }
};

class function_json_experimental_generator : public function_generator {
 public:
  function_json_experimental_generator() = default;
  ~function_json_experimental_generator() override = default;
  std::shared_ptr<mstch_base> generate(
      t_function const* func,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t /*index*/ = 0) const override {
    return std::make_shared<json_experimental_function>(
        func, generators, cache, pos);
  }
};

class json_experimental_field : public mstch_field {
 public:
  json_experimental_field(
      t_field const* field,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION const pos,
      int32_t index)
      : mstch_field(field, generators, cache, pos, index) {
    register_methods(
        this,
        {
            {"field:lineno", &json_experimental_field::get_lineno},
            {"field:docstring?", &json_experimental_field::has_docstring},
            {"field:docstring", &json_experimental_field::get_docstring},
        });
  }
  mstch::node get_lineno() {
    return field_->get_lineno();
  }
  mstch::node has_docstring() {
    return field_->has_doc();
  }
  mstch::node get_docstring() {
    return json_quote_ascii(field_->get_doc());
  }
};

class field_json_experimental_generator : public field_generator {
 public:
  field_json_experimental_generator() = default;
  ~field_json_experimental_generator() override = default;
  std::shared_ptr<mstch_base> generate(
      t_field const* field,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t index = 0) const override {
    return std::make_shared<json_experimental_field>(
        field, generators, cache, pos, index);
  }
};

void t_json_experimental_generator::generate_program() {
  auto const* program = get_program();
  set_mstch_generators();
  std::string fname = program->get_name();
  fname += ".json";
  auto output =
      generators_->program_generator_->generate(program, generators_, cache_);
  render_to_file(output, "thrift_ast", fname);
}

void t_json_experimental_generator::set_mstch_generators() {
  generators_->set_program_generator(
      std::make_unique<program_json_experimental_generator>());
  generators_->set_enum_generator(
      std::make_unique<enum_json_experimental_generator>());
  generators_->set_enum_value_generator(
      std::make_unique<enum_value_json_experimental_generator>());
  generators_->set_const_value_generator(
      std::make_unique<const_value_json_experimental_generator>());
  generators_->set_struct_generator(
      std::make_unique<struct_json_experimental_generator>());
  generators_->set_field_generator(
      std::make_unique<field_json_experimental_generator>());
  generators_->set_service_generator(
      std::make_unique<service_json_experimental_generator>());
  generators_->set_function_generator(
      std::make_unique<function_json_experimental_generator>());
}

} // namespace

THRIFT_REGISTER_GENERATOR(json_experimental, "JSON_EXPERIMENTAL", "");

} // namespace compiler
} // namespace thrift
} // namespace apache
