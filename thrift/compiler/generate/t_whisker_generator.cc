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

#include <thrift/compiler/generate/t_whisker_generator.h>

#include <thrift/compiler/detail/system.h>
#include <thrift/compiler/generate/templates.h>
#include <thrift/compiler/whisker/ast.h>
#include <thrift/compiler/whisker/parser.h>
#include <thrift/compiler/whisker/source_location.h>
#include <thrift/compiler/whisker/standard_library.h>

#include <cassert>
#include <cstddef>
#include <fstream>

#include <fmt/ranges.h>

#include <boost/algorithm/string/split.hpp>

using fs_path = std::filesystem::path;

namespace apache::thrift::compiler {

namespace {

bool is_last_char(std::string_view data, char c) {
  return !data.empty() && data.back() == c;
}

void chomp_last_char(std::string* data, char c) {
  if (is_last_char(*data, c)) {
    data->pop_back();
  }
}

/**
 * This implementation of whisker's template resolver builds on top of the
 * template_map that already used by mstch. This allows template names to
 * remain the same (albeit exact file names are not recoverable).
 *
 * When a new partial is requested, we lazily parse the corresponding entry
 * in the template_map and cache the parsed AST.
 */
class whisker_template_parser : public whisker::template_resolver {
 public:
  explicit whisker_template_parser(
      const t_whisker_generator::templates_map& templates_by_path,
      std::string template_prefix)
      : templates_by_path_(templates_by_path),
        template_prefix_(std::move(template_prefix)) {}

  std::optional<whisker::ast::root> resolve(
      const std::vector<std::string>& partial_path,
      source_location include_from,
      diagnostics_engine& diags) override {
    auto path = normalize_path(partial_path, include_from);

    if (auto cached = cached_asts_.find(path); cached != cached_asts_.end()) {
      return cached->second;
    }

    auto source_code = templates_by_path_.find(path);
    if (source_code == templates_by_path_.end()) {
      return std::nullopt;
    }
    auto src = src_manager_.add_virtual_file(path, source_code->second);
    auto ast = whisker::parse(src, diags);
    auto [result, inserted] =
        cached_asts_.insert({std::move(path), std::move(ast)});
    assert(inserted);
    return result->second;
  }

  whisker::source_manager& source_manager() { return src_manager_; }

 private:
  std::string normalize_path(
      const std::vector<std::string>& partial_path,
      source_location include_from) const {
    // The template_prefix will be added to the partial path, e.g.,
    // "field/member" --> "cpp2/field/member"
    std::string template_prefix;

    auto start = partial_path.begin();
    if (include_from == source_location()) {
      // If include_from is empty, we use the stored template_prefix
      template_prefix = template_prefix_;
    } else if (*start != "..") {
      fs_path current_file_path =
          resolved_location(include_from, src_manager_).file_name();
      template_prefix = current_file_path.begin()->generic_string();
    } else {
      // If path starts with "..", the template_prefix will be the second
      // element, and the template_name starts at the 3rd element. e.g.,
      // "../cpp2/field/member": template_prefix = "cpp2"
      ++start;
      template_prefix = *start++;
    }

    // Whisker always breaks down the path into components. However, the
    // template_map stores them as one concatenated string.
    return fmt::format(
        "{}/{}", template_prefix, fmt::join(start, partial_path.end(), "/"));
  }

  const std::map<std::string, std::string, std::less<>>& templates_by_path_;
  std::string template_prefix_;
  whisker::source_manager src_manager_;
  std::unordered_map<std::string, std::optional<whisker::ast::root>>
      cached_asts_;
};

} // namespace

/* static */ const t_whisker_generator::templates_map&
t_whisker_generator::templates_by_path() {
  static const auto cached_result = [] {
    templates_map result;
    for (std::size_t i = 0; i < templates_size; ++i) {
      auto name = fs_path(
          templates_name_datas[i],
          templates_name_datas[i] + templates_name_sizes[i]);
      name = name.parent_path() / name.stem();

      auto tpl = std::string(
          templates_content_datas[i],
          templates_content_datas[i] + templates_content_sizes[i]);
      // Remove a single '\n' or '\r\n' or '\r' at end, if present.
      chomp_last_char(&tpl, '\n');
      chomp_last_char(&tpl, '\r');
      result.emplace(name.generic_string(), std::move(tpl));
    }
    return result;
  }();

  return cached_result;
}

t_whisker_generator::cached_render_state& t_whisker_generator::render_state() {
  if (!cached_render_state_) {
    whisker::render_options options;

    auto template_parser = std::make_shared<whisker_template_parser>(
        templates_by_path(), template_prefix());
    options.partial_resolver = template_parser;

    strictness_options strict = strictness();
    const auto level_for = [](bool strict) {
      return strict ? diagnostic_level::error : diagnostic_level::debug;
    };
    options.strict_boolean_conditional = level_for(strict.boolean_conditional);
    options.strict_printable_types = level_for(strict.printable_types);
    options.strict_undefined_variables = level_for(strict.undefined_variables);

    whisker::load_standard_library(options.globals);
    options.globals.merge(globals());

    cached_render_state_ = cached_render_state{
        whisker::diagnostics_engine(
            template_parser->source_manager(),
            [](const diagnostic& d) { fmt::print(stderr, "{}\n", d); },
            diagnostic_params::only_errors()),
        template_parser,
        std::move(options),
    };
  }

  assert(cached_render_state_.has_value());
  return *cached_render_state_;
}

std::string t_whisker_generator::render(
    std::string_view template_file, const whisker::object& context) {
  std::vector<std::string> partial_path;
  boost::algorithm::split(
      partial_path, template_file, [](char c) { return c == '/'; });

  cached_render_state& state = render_state();
  std::optional<whisker::ast::root> ast = state.template_resolver->resolve(
      partial_path, {}, state.diagnostic_engine);
  if (!ast.has_value()) {
    throw std::runtime_error{fmt::format(
        "Failed to find or correctly parse template '{}'", template_file)};
  }

  std::ostringstream out;
  if (!whisker::render(
          out, *ast, context, state.diagnostic_engine, state.render_options)) {
    throw std::runtime_error{
        fmt::format("Failed to render template '{}'", template_file)};
  }
  return out.str();
}

void t_whisker_generator::write_to_file(
    const std::filesystem::path& output_file, std::string_view data) {
  auto abs_path = detail::make_abs_path(fs_path(get_out_dir()), output_file);
  std::filesystem::create_directories(abs_path.parent_path());

  {
    std::ofstream output{abs_path.string()};
    if (!output) {
      throw std::runtime_error(
          fmt::format("Could not open '{}' for writing.", abs_path.string()));
    }
    output << data;
    if (!is_last_char(data, '\n')) {
      // Terminate with newline.
      output << '\n';
    }
  }
  record_genfile(abs_path.string());
}

void t_whisker_generator::render_to_file(
    const std::filesystem::path& output_file,
    std::string_view template_file,
    const whisker::object& context) {
  write_to_file(output_file, render(template_file, context));
}

} // namespace apache::thrift::compiler
