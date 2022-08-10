/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
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

#include <algorithm>
#include <memory>

#include <thrift/compiler/generate/json.h>
#include <thrift/compiler/generate/mstch_objects.h>
#include <thrift/compiler/generate/t_mstch_generator.h>

namespace apache {
namespace thrift {
namespace compiler {
namespace {

class t_json_experimental_generator : public t_mstch_generator {
 public:
  t_json_experimental_generator(
      t_program* program,
      t_generation_context context,
      const std::map<std::string, std::string>& options)
      : t_mstch_generator(program, std::move(context), "json", options, true) {
    out_dir_base_ = "gen-json_experimental";
  }

  void generate_program() override;

 private:
  void set_mstch_factories();
};

class json_experimental_program : public mstch_program {
 public:
  json_experimental_program(
      const t_program* program,
      const mstch_factories& factories,
      std::shared_ptr<mstch_cache> cache,
      mstch_element_position pos)
      : mstch_program(program, factories, cache, pos) {
    register_methods(
        this,
        {
            {"program:py_namespace",
             &json_experimental_program::get_py_namespace},
            {"program:namespaces?", &json_experimental_program::has_namespaces},
            {"program:namespaces", &json_experimental_program::namespaces},
            {"program:docstring?", &json_experimental_program::has_docstring},
            {"program:docstring", &json_experimental_program::get_docstring},
            {"program:normalizedIncludePrefix",
             &json_experimental_program::include_prefix},
        });
  }
  mstch::node get_py_namespace() { return program_->get_namespace("py"); }
  mstch::node has_namespaces() { return !program_->namespaces().empty(); }
  mstch::node namespaces() {
    mstch::array result;
    auto last = program_->namespaces().size();
    for (auto it : program_->namespaces()) {
      result.push_back(mstch::map{
          {"key", it.first}, {"value", it.second}, {"last?", (--last) == 0}});
    }
    return result;
  }
  mstch::node has_docstring() { return !program_->get_doc().empty(); }
  mstch::node get_docstring() { return json_quote_ascii(program_->get_doc()); }

  mstch::node include_prefix() {
    auto prefix = program_->include_prefix();
    if (!prefix.empty()) {
      if (boost::filesystem::path(prefix).has_root_directory()) {
        return cache_->options_["include_prefix"];
      }
      return prefix;
    }
    return std::string();
  }
};

class json_experimental_service : public mstch_service {
 public:
  json_experimental_service(
      const t_service* srvc,
      const mstch_factories& factories,
      std::shared_ptr<mstch_cache> cache,
      mstch_element_position pos)
      : mstch_service(srvc, factories, cache, pos) {
    register_methods(
        this,
        {
            {"service:lineno", &json_experimental_service::get_lineno},
            {"service:docstring?", &json_experimental_service::has_docstring},
            {"service:docstring", &json_experimental_service::get_docstring},
        });
  }
  mstch::node get_lineno() { return service_->get_lineno(); }
  mstch::node has_docstring() { return service_->has_doc(); }
  mstch::node get_docstring() { return json_quote_ascii(service_->get_doc()); }
};

class json_experimental_function : public mstch_function {
 public:
  json_experimental_function(
      const t_function* func,
      const mstch_factories& factories,
      std::shared_ptr<mstch_cache> cache,
      mstch_element_position pos)
      : mstch_function(func, factories, cache, pos) {
    register_methods(
        this,
        {
            {"function:lineno", &json_experimental_function::get_lineno},
            {"function:docstring?", &json_experimental_function::has_docstring},
            {"function:docstring", &json_experimental_function::get_docstring},
            {"function:args?", &json_experimental_function::has_args},
        });
  }
  mstch::node get_lineno() { return function_->get_lineno(); }
  mstch::node has_docstring() { return function_->has_doc(); }
  mstch::node get_docstring() { return json_quote_ascii(function_->get_doc()); }
  mstch::node has_args() {
    return !function_->get_paramlist()->get_members().empty();
  }
};

class json_experimental_struct : public mstch_struct {
 public:
  json_experimental_struct(
      const t_struct* strct,
      const mstch_factories& factories,
      std::shared_ptr<mstch_cache> cache,
      mstch_element_position pos)
      : mstch_struct(strct, factories, cache, pos) {
    register_methods(
        this,
        {
            {"struct:lineno", &json_experimental_struct::get_lineno},
            {"struct:docstring?", &json_experimental_struct::has_docstring},
            {"struct:docstring", &json_experimental_struct::get_docstring},
        });
  }
  mstch::node get_lineno() { return struct_->get_lineno(); }
  mstch::node has_docstring() { return struct_->has_doc(); }
  mstch::node get_docstring() { return json_quote_ascii(struct_->get_doc()); }
};

class json_experimental_field : public mstch_field {
 public:
  json_experimental_field(
      const t_field* field,
      const mstch_factories& factories,
      std::shared_ptr<mstch_cache> cache,
      mstch_element_position pos,
      const field_generator_context* field_context)
      : mstch_field(field, factories, cache, pos, field_context) {
    register_methods(
        this,
        {
            {"field:lineno", &json_experimental_field::get_lineno},
            {"field:docstring?", &json_experimental_field::has_docstring},
            {"field:docstring", &json_experimental_field::get_docstring},
        });
  }
  mstch::node get_lineno() { return field_->get_lineno(); }
  mstch::node has_docstring() { return field_->has_doc(); }
  mstch::node get_docstring() { return json_quote_ascii(field_->get_doc()); }
};

class json_experimental_enum : public mstch_enum {
 public:
  json_experimental_enum(
      const t_enum* enm,
      const mstch_factories& factories,
      std::shared_ptr<mstch_cache> cache,
      mstch_element_position pos)
      : mstch_enum(enm, factories, cache, pos) {
    register_methods(
        this,
        {
            {"enum:empty?", &json_experimental_enum::is_empty},
            {"enum:lineno", &json_experimental_enum::get_lineno},
            {"enum:docstring?", &json_experimental_enum::has_docstring},
            {"enum:docstring", &json_experimental_enum::get_docstring},
        });
  }
  mstch::node is_empty() { return enum_->get_enum_values().empty(); }
  mstch::node get_lineno() { return enum_->get_lineno(); }
  mstch::node has_docstring() { return enum_->has_doc(); }
  mstch::node get_docstring() { return json_quote_ascii(enum_->get_doc()); }
};

class json_experimental_enum_value : public mstch_enum_value {
 public:
  json_experimental_enum_value(
      const t_enum_value* enum_value,
      const mstch_factories& factories,
      std::shared_ptr<mstch_cache> cache,
      mstch_element_position pos)
      : mstch_enum_value(enum_value, factories, cache, pos) {
    register_methods(
        this,
        {
            {"enum_value:lineno", &json_experimental_enum_value::get_lineno},
            {"enum_value:docstring?",
             &json_experimental_enum_value::has_docstring},
            {"enum_value:docstring",
             &json_experimental_enum_value::get_docstring},
        });
  }
  mstch::node get_lineno() { return enum_value_->get_lineno(); }
  mstch::node has_docstring() { return enum_value_->has_doc(); }
  mstch::node get_docstring() {
    return json_quote_ascii(enum_value_->get_doc());
  }
};

// Trim all white spaces and commas from end (in place).
void rtrim(std::string& s) {
  s.erase(
      std::find_if(
          s.rbegin(),
          s.rend(),
          [](int ch) { return !(ch == ' ' || ch == ','); })
          .base(),
      s.end());
}

std::string to_string(const t_const_value* value) {
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
      auto key = to_string(v.first);
      if (v.first->get_type() != mstch_const_value::cv::CV_STRING) {
        // map keys must be strings
        key = json_quote_ascii(key);
      }
      result += key + ": " + to_string(v.second) + ", ";
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
      const t_const_value* const_value,
      const mstch_factories& factories,
      std::shared_ptr<mstch_cache> cache,
      mstch_element_position pos,
      const t_const* current_const,
      const t_type* expected_type)
      : mstch_const_value(
            const_value, factories, cache, pos, current_const, expected_type) {
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
  mstch::node get_lineno() { return current_const_->get_lineno(); }
  mstch::node has_docstring() { return current_const_->has_doc(); }
  mstch::node get_docstring() {
    return json_quote_ascii(current_const_->get_doc());
  }

  mstch::node get_typename() {
    return current_const_->get_type()->get_true_type()->get_full_name();
  }

  mstch::node string_value_any() { return to_string(const_value_); }
};

void t_json_experimental_generator::generate_program() {
  const auto* program = get_program();
  set_mstch_factories();
  auto output = factories_.program_factory->make_mstch_object(
      program, factories_, cache_);
  render_to_file(output, "thrift_ast", program->name() + ".json");
}

void t_json_experimental_generator::set_mstch_factories() {
  factories_.add<json_experimental_program>();
  factories_.add<json_experimental_service>();
  factories_.add<json_experimental_function>();
  factories_.add<json_experimental_struct>();
  factories_.add<json_experimental_field>();
  factories_.add<json_experimental_enum>();
  factories_.add<json_experimental_enum_value>();
  factories_.add<json_experimental_const_value>();
}

THRIFT_REGISTER_GENERATOR(json_experimental, "JSON_EXPERIMENTAL", "");

} // namespace
} // namespace compiler
} // namespace thrift
} // namespace apache
