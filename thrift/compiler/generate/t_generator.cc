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

#include <thrift/compiler/generate/t_generator.h>

#include <stdexcept>
#include <utility>

#include <boost/algorithm/string/split.hpp>
#include <boost/filesystem.hpp>
#include <fmt/core.h>

namespace apache {
namespace thrift {
namespace compiler {

t_generation_context::t_generation_context(
    std::string out_path, bool is_out_path_absolute, source_manager* sm)
    : out_path_(std::move(out_path)),
      is_out_path_absolute_(is_out_path_absolute),
      source_mgr_(sm) {
  boost::filesystem::path path = {out_path_};
  if (!out_path_.empty()) {
    if (!(out_path_.back() == '/' || out_path_.back() == '\\')) {
      path += boost::filesystem::path::preferred_separator;
    }
  }
  out_path_ = path.make_preferred().string();
}

t_generator::t_generator(t_program* program, t_generation_context context)
    : program_(program), context_(std::move(context)) {
  program_name_ = get_program_name(program_);
}

void t_generator_registry::register_generator(t_generator_factory* factory) {
  std::string name = factory->get_short_name();
  if (!get_generator_map().insert(std::make_pair(name, factory)).second) {
    throw std::logic_error(
        fmt::format("duplicate generator for language \"{}\"", name));
  }
}

t_generator* t_generator_registry::get_generator(
    t_program* program,
    t_generation_context context,
    const std::string& option_string) {
  std::string::size_type colon = option_string.find(':');
  std::string language = option_string.substr(0, colon);

  std::map<std::string, std::string> options;
  parse_generator_options(
      option_string.substr(colon + 1), [&](std::string k, std::string v) {
        options[std::move(k)] = std::move(v);
        return CallbackLoopControl::Continue;
      });

  gen_map_t& the_map = get_generator_map();
  gen_map_t::iterator iter = the_map.find(language);

  if (iter == the_map.end()) {
    return nullptr;
  }

  return iter->second->get_generator(program, context, options);
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

void parse_generator_options(
    const std::string& option_string,
    std::function<CallbackLoopControl(std::string, std::string)> callback) {
  std::vector<std::string> parts;
  bool inside_braces = false;
  boost::algorithm::split(parts, option_string, [&inside_braces](char c) {
    if (c == '{' || c == '}') {
      inside_braces = (c == '{');
    }
    return c == ',' && !inside_braces;
  });
  for (const auto& part : parts) {
    auto key = part.substr(0, part.find('='));
    auto value = part.substr(std::min(key.size() + 1, part.size()));
    if (callback(std::move(key), std::move(value)) ==
        CallbackLoopControl::Break) {
      break;
    }
  }
}

} // namespace compiler
} // namespace thrift
} // namespace apache
