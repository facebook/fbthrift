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

#include <thrift/compiler/generate/t_generator.h>

#include <utility>

#include <thrift/compiler/common.h>

namespace apache {
namespace thrift {
namespace compiler {

t_generation_context::t_generation_context(
    std::string out_path, bool is_out_path_absolute)
    : out_path_(std::move(out_path)),
      is_out_path_absolute_(is_out_path_absolute) {
  if (!out_path_.empty()) {
    if (!(out_path_.back() == '/' || out_path_.back() == '\\')) {
      out_path_.push_back('/');
    }
  }
}

t_generator::t_generator(t_program* program, t_generation_context context)
    : program_(program), context_(std::move(context)) {
  program_name_ = get_program_name(program_);
}

void t_generator_registry::register_generator(t_generator_factory* factory) {
  gen_map_t& the_map = get_generator_map();
  if (the_map.find(factory->get_short_name()) != the_map.end()) {
    failure(
        "Duplicate generators for language \"%s\"!\n",
        factory->get_short_name().c_str());
  }
  the_map[factory->get_short_name()] = factory;
}

t_generator* t_generator_registry::get_generator(
    t_program* program,
    t_generation_context context,
    const std::string& options) {
  std::string::size_type colon = options.find(':');
  std::string language = options.substr(0, colon);

  std::map<std::string, std::string> parsed_options;
  if (colon != std::string::npos) {
    std::string::size_type pos = colon + 1;
    while (pos != std::string::npos && pos < options.size()) {
      std::string::size_type next_pos = options.find(',', pos);
      std::string option = options.substr(pos, next_pos - pos);
      pos = ((next_pos == std::string::npos) ? next_pos : next_pos + 1);

      std::string::size_type separator = option.find('=');
      std::string key, value;
      if (separator == std::string::npos) {
        key = option;
        value = "";
      } else {
        key = option.substr(0, separator);
        value = option.substr(separator + 1);
      }

      parsed_options[key] = value;
    }
  }

  gen_map_t& the_map = get_generator_map();
  gen_map_t::iterator iter = the_map.find(language);

  if (iter == the_map.end()) {
    return nullptr;
  }

  return iter->second->get_generator(program, context, parsed_options, options);
}

t_generator_registry::gen_map_t& t_generator_registry::get_generator_map() {
  // http://www.parashift.com/c++-faq-lite/ctors.html#faq-10.12
  static gen_map_t* the_map = new gen_map_t();
  return *the_map;
}

t_generator_factory::t_generator_factory(
    std::string short_name, std::string long_name, std::string documentation)
    : short_name_(std::move(short_name)),
      long_name_(std::move(long_name)),
      documentation_(std::move(documentation)) {
  t_generator_registry::register_generator(this);
}

} // namespace compiler
} // namespace thrift
} // namespace apache
