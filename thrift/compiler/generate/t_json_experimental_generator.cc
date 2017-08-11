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

#include <boost/algorithm/string.hpp>

#include <thrift/compiler/generate/common.h>
#include <thrift/compiler/generate/t_mstch_generator.h>
#include <thrift/compiler/generate/t_mstch_objects.h>

namespace {

class t_json_experimental_generator : public t_mstch_generator {
 public:
  t_json_experimental_generator(
      t_program* program,
      const std::map<std::string, std::string>& parsed_options,
      const std::string& /*option_string*/);

  void generate_program() override;

 private:
  void set_mstch_generators();
};

t_json_experimental_generator::t_json_experimental_generator(
    t_program* program,
    const std::map<std::string, std::string>& parsed_options,
    const std::string& /*option_string*/)
    : t_mstch_generator(program, "json", parsed_options, true) {
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
            {"program:wiki_address",
             &json_experimental_program::get_wiki_address},
            {"program:normalizedIncludePrefix",
             &json_experimental_program::include_prefix},
            {"program:enums?", &json_experimental_program::has_enums},
        });
  }
  mstch::node get_py_namespace() {
    return program_->get_namespace("py");
  }
  mstch::node get_wiki_address() {
    return program_->get_namespace("wiki");
  }
  mstch::node include_prefix() {
    auto prefix = program_->get_include_prefix();
    if (!prefix.empty()) {
      if (prefix[0] == '/') {
        return cache_->parsed_options_["include_prefix"];
      }
      return prefix;
    }
    return std::string();
  }
  mstch::node has_enums() {
    if (!program_->get_enums().empty()) {
      return true;
    }
    return false;
  }
};

class program_json_experimental_generator : public program_generator {
 public:
  program_json_experimental_generator() = default;
  virtual ~program_json_experimental_generator() = default;
  virtual std::shared_ptr<mstch_base> generate(
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
            {"enum:docstring", &json_experimental_enum::get_docstring},
        });
  }
  mstch::node is_empty() {
    return enm_->get_constants().empty();
  }
  mstch::node get_lineno() {
    return enm_->get_lineno();
  }
  mstch::node get_docstring() {
    return enm_->get_doc();
  }
};

class enum_json_experimental_generator : public enum_generator {
 public:
  enum_json_experimental_generator() = default;
  virtual ~enum_json_experimental_generator() = default;
  virtual std::shared_ptr<mstch_base> generate(
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
            {"enumValue:docstring",
             &json_experimental_enum_value::get_docstring},
        });
  }
  mstch::node get_lineno() {
    return enm_value_->get_lineno();
  }
  mstch::node get_docstring() {
    return enm_value_->get_doc();
  }
};

class enum_value_json_experimental_generator : public enum_value_generator {
 public:
  enum_value_json_experimental_generator() = default;
  virtual ~enum_value_json_experimental_generator() = default;
  virtual std::shared_ptr<mstch_base> generate(
      t_enum_value const* enm_value,
      std::shared_ptr<mstch_generators const> generators,
      std::shared_ptr<mstch_cache> cache,
      ELEMENT_POSITION pos = ELEMENT_POSITION::NONE,
      int32_t /*index*/ = 0) const override {
    return std::make_shared<json_experimental_enum_value>(
        enm_value, generators, cache, pos);
  }
};

void t_json_experimental_generator::generate_program() {
  // disable mstch escaping
  // TODO(T20509094): proper JSON escaping routine
  mstch::config::escape = [](const std::string& str) -> std::string {
    std::string str_copy = str;
    boost::replace_all(str_copy, "\n", "\\n");
    return str_copy;
  };

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
}
} // anonymous namespace

THRIFT_REGISTER_GENERATOR(json_experimental, "JSON_EXPERIMENTAL", "");
