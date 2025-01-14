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

#include <thrift/compiler/generate/t_mstch_generator.h>

#include <cassert>
#include <cstdio>
#include <fstream>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include <fmt/format.h>

#include <thrift/compiler/detail/mustache/mstch.h>
#include <thrift/compiler/whisker/ast.h>
#include <thrift/compiler/whisker/mstch_compat.h>
#include <thrift/compiler/whisker/parser.h>
#include <thrift/compiler/whisker/source_location.h>
#include <thrift/compiler/whisker/standard_library.h>

#include <thrift/compiler/detail/system.h>
#include <thrift/compiler/generate/t_generator.h>
#include <thrift/compiler/generate/templates.h>

#include <boost/algorithm/string/split.hpp>

using namespace std;

namespace fs = std::filesystem;

namespace apache::thrift::compiler {

namespace {

bool is_last_char(const string& data, char c) {
  return !data.empty() && data.back() == c;
}

void chomp_last_char(string* data, char c) {
  if (is_last_char(*data, c)) {
    data->pop_back();
  }
}

} // namespace

mstch::map t_mstch_generator::dump(const t_program& program) {
  mstch::map result{
      {"name", program.name()},
      {"path", program.path()},
      {"includePrefix", program.include_prefix()},
      {"structs", dump_elems(program.structured_definitions())},
      {"enums", dump_elems(program.enums())},
      {"services", dump_elems(program.services())},
      {"typedefs", dump_elems(program.typedefs())},
      {"constants", dump_elems(program.consts())},
  };

  mstch::map extension = extend_program(program);
  extension.insert(result.begin(), result.end());
  return prepend_prefix("program", std::move(extension));
}

mstch::map t_mstch_generator::dump(const t_structured& strct, bool shallow) {
  mstch::map result{
      {"name", strct.get_name()},
      {"fields?", strct.has_fields()},
      {"fields",
       shallow ? static_cast<mstch::node>(false) : dump_elems(strct.fields())},
      {"exception?", strct.is_exception()},
      {"union?", strct.is_union()},
      {"plain?", !strct.is_exception() && !strct.is_union()},
      {"annotations", dump_elems(strct.annotations())},
  };

  mstch::map extension = extend_struct(strct);
  extension.insert(result.begin(), result.end());
  return prepend_prefix("struct", std::move(extension));
}

mstch::map t_mstch_generator::dump(const t_field& field, int32_t index) {
  auto req = field.get_req();
  mstch::map result{
      {"name", field.get_name()},
      {"key", std::to_string(field.get_key())},
      {"type", dump(*field.get_type())},
      {"index", std::to_string(index)},
      {"index_plus_one", std::to_string(index + 1)},
      {"required?", req == t_field::e_req::required},
      {"optional?", req == t_field::e_req::optional},
      {"opt_in_req_out?", req == t_field::e_req::opt_in_req_out},
      {"terse?", req == t_field::e_req::terse},
      {"annotations", dump_elems(field.annotations())},
  };

  if (field.get_value() != nullptr) {
    result.emplace("value", dump(*field.get_value()));
  }

  mstch::map extension = extend_field(field);
  extension.insert(result.begin(), result.end());
  return prepend_prefix("field", std::move(extension));
}

mstch::map t_mstch_generator::dump(const t_type& orig_type) {
  const t_type& type =
      should_resolve_typedefs() ? *orig_type.get_true_type() : orig_type;

  mstch::map result{
      {"name", type.get_name()},
      {"annotations", dump_elems(type.annotations())},

      {"void?", type.is_void()},
      {"string?", type.is_string()},
      {"binary?", type.is_binary()},
      {"bool?", type.is_bool()},
      {"byte?", type.is_byte()},
      {"i16?", type.is_i16()},
      {"i32?", type.is_i32()},
      {"i64?", type.is_i64()},
      {"double?", type.is_double()},
      {"float?", type.is_float()},
      {"floating_point?", type.is_floating_point()},
      {"struct?", type.is_struct() || type.is_exception()},
      {"union?", type.is_union()},
      {"enum?", type.is_enum()},
      {"service?", type.is_service()},
      {"base?", type.is_primitive_type()},
      {"container?", type.is_container()},
      {"list?", type.is_list()},
      {"set?", type.is_set()},
      {"map?", type.is_map()},
      {"typedef?", type.is_typedef()},
  };

  if (type.is_struct() || type.is_exception()) {
    // Shallow dump the struct
    result.emplace("struct", dump(dynamic_cast<const t_struct&>(type), true));
  } else if (type.is_enum()) {
    result.emplace("enum", dump(dynamic_cast<const t_enum&>(type)));
  } else if (type.is_service()) {
    result.emplace("service", dump(dynamic_cast<const t_service&>(type)));
  } else if (type.is_list()) {
    result.emplace(
        "list_elem_type",
        dump(*dynamic_cast<const t_list&>(type).get_elem_type()));
  } else if (type.is_set()) {
    result.emplace(
        "set_elem_type",
        dump(*dynamic_cast<const t_set&>(type).get_elem_type()));
  } else if (type.is_map()) {
    result.emplace(
        "key_type", dump(*dynamic_cast<const t_map&>(type).get_key_type()));
    result.emplace(
        "value_type", dump(*dynamic_cast<const t_map&>(type).get_val_type()));
  } else if (type.is_typedef()) {
    result.emplace(
        "typedef_type", dump(*dynamic_cast<const t_typedef&>(type).get_type()));
  }

  mstch::map extension = extend_type(type);
  extension.insert(result.begin(), result.end());
  return prepend_prefix("type", std::move(extension));
}

mstch::map t_mstch_generator::dump(const t_enum& enm) {
  mstch::map result{
      {"name", enm.get_name()},
      {"values", dump_elems(enm.get_enum_values())},
      {"annotations", dump_elems(enm.annotations())},
  };

  mstch::map extension = extend_enum(enm);
  extension.insert(result.begin(), result.end());
  return prepend_prefix("enum", std::move(extension));
}

mstch::map t_mstch_generator::dump(const t_enum_value& val) {
  mstch::map result{
      {"name", val.get_name()},
      {"value", std::to_string(val.get_value())},
  };

  mstch::map extension = extend_enum_value(val);
  extension.insert(result.begin(), result.end());
  return prepend_prefix("enum_value", std::move(extension));
}

mstch::map t_mstch_generator::dump(const t_service& service) {
  const t_service* extends = service.get_extends();
  mstch::map result{
      {"name", service.get_name()},
      {"annotations", dump_elems(service.annotations())},
      {"functions", dump_elems(service.get_functions())},
      {"functions?", !service.get_functions().empty()},
      {"extends?", extends != nullptr},
      {"extends", extends ? static_cast<mstch::node>(dump(*extends)) : false},
  };

  mstch::map extension = extend_service(service);
  extension.insert(result.begin(), result.end());
  return prepend_prefix("service", std::move(extension));
}

mstch::map t_mstch_generator::dump(const t_function& function) {
  auto exceptions = get_elems(function.exceptions());
  mstch::map result{
      {"name", function.get_name()},
      {"oneway?", function.qualifier() == t_function_qualifier::oneway},
      {"return_type", dump(*function.return_type())},
      {"exceptions", dump_elems(exceptions)},
      {"exceptions?", !exceptions.empty()},
      {"annotations", dump_elems(function.annotations())},
      {"args", dump_elems(function.params().fields())},
  };

  mstch::map extension = extend_function(function);
  extension.insert(result.begin(), result.end());
  return prepend_prefix("function", std::move(extension));
}

mstch::map t_mstch_generator::dump(const t_const& cnst) {
  mstch::map result{
      {"type", dump(*cnst.type())},
      {"name", cnst.get_name()},
      {"value", dump(*cnst.value())},
  };

  mstch::map extension = extend_const(cnst);
  extension.insert(result.begin(), result.end());
  return prepend_prefix("constant", std::move(extension));
}

mstch::map t_mstch_generator::dump(const t_const_value& value) {
  using cv = t_const_value::t_const_value_kind;
  const cv type = value.kind();
  mstch::map result{
      {"bool?", type == cv::CV_BOOL},
      {"double?", type == cv::CV_DOUBLE},
      {"integer?", type == cv::CV_INTEGER && !value.get_enum_value()},
      {"enum?", type == cv::CV_INTEGER && value.get_enum_value()},
      {"string?", type == cv::CV_STRING},
      {"base?",
       type == cv::CV_BOOL || type == cv::CV_DOUBLE || type == cv::CV_INTEGER ||
           type == cv::CV_STRING},
      {"map?", type == cv::CV_MAP},
      {"list?", type == cv::CV_LIST},
      {"container?", type == cv::CV_MAP || type == cv::CV_LIST},
  };

  switch (type) {
    case cv::CV_DOUBLE:
      result.emplace("value", value.get_double());
      result.emplace("double_value", value.get_double());
      result.emplace("nonzero?", value.get_double() != 0.0);
      break;
    case cv::CV_BOOL:
      result.emplace("value", value.get_bool() ? 1 : 0);
      result.emplace("bool_value", value.get_bool());
      result.emplace("nonzero?", value.get_bool());
      break;
    case cv::CV_INTEGER:
      if (value.get_enum_value()) {
        result.emplace("enum_name", value.get_enum()->get_name());
        result.emplace("enum_value_name", value.get_enum_value()->get_name());
      }
      result.emplace("value", std::to_string(value.get_integer()));
      result.emplace("integer_value", std::to_string(value.get_integer()));
      result.emplace("nonzero?", value.get_integer() != 0);
      break;
    case cv::CV_STRING:
      result.emplace("value", value.get_string());
      result.emplace("string_value", value.get_string());
      break;
    case cv::CV_MAP:
      result.emplace("map_elements", dump_elems(value.get_map()));
      break;
    case cv::CV_LIST:
      result.emplace("list_elements", dump_elems(value.get_list()));
      break;
    default:
      std::ostringstream err;
      err << "Unhandled t_const_value_kind " << value.kind();
      throw std::domain_error{err.str()};
  }

  mstch::map extension = extend_const_value(value);
  extension.insert(result.begin(), result.end());
  return prepend_prefix("value", std::move(extension));
}

mstch::map t_mstch_generator::dump(
    const std::map<t_const_value*, t_const_value*>::value_type& pair) {
  mstch::map result{
      {"key", dump(*pair.first)},
      {"value", dump(*pair.second)},
  };
  mstch::map extension = extend_const_value_map_elem(pair);
  extension.insert(result.begin(), result.end());
  return prepend_prefix("element", std::move(extension));
}

mstch::map t_mstch_generator::dump(const annotation& pair) {
  mstch::map result{
      {"key", pair.first},
      {"value", pair.second.value},
  };
  mstch::map extension = extend_annotation(pair);
  extension.insert(result.begin(), result.end());
  return prepend_prefix("annotation", std::move(extension));
}

mstch::map t_mstch_generator::dump(const t_typedef& typdef) {
  mstch::map result{
      {"type", dump(*typdef.get_type())},
      {"name", typdef.name()},
  };

  mstch::map extension = extend_typedef(typdef);
  extension.insert(result.begin(), result.end());
  return prepend_prefix("typedef", std::move(extension));
}

mstch::map t_mstch_generator::dump(const string& value) {
  mstch::map result{{"value", value}};
  return result;
}

mstch::map t_mstch_generator::dump_options() {
  mstch::map result;
  for (auto& elem : options_) {
    result.emplace(elem.first, elem.second);
  }
  return prepend_prefix("option", std::move(result));
}

// Extenders, by default do no extending

mstch::map t_mstch_generator::extend_program(const t_program&) {
  return {};
}

mstch::map t_mstch_generator::extend_struct(const t_structured&) {
  return {};
}

mstch::map t_mstch_generator::extend_field(const t_field&) {
  return {};
}

mstch::map t_mstch_generator::extend_type(const t_type&) {
  return {};
}

mstch::map t_mstch_generator::extend_enum(const t_enum&) {
  return {};
}

mstch::map t_mstch_generator::extend_enum_value(const t_enum_value&) {
  return {};
}

mstch::map t_mstch_generator::extend_service(const t_service&) {
  return {};
}

mstch::map t_mstch_generator::extend_function(const t_function&) {
  return {};
}

mstch::map t_mstch_generator::extend_typedef(const t_typedef&) {
  return {};
}

mstch::map t_mstch_generator::extend_const(const t_const&) {
  return {};
}

mstch::map t_mstch_generator::extend_const_value(const t_const_value&) {
  return {};
}

mstch::map t_mstch_generator::extend_const_value_map_elem(
    const std::map<t_const_value*, t_const_value*>::value_type&) {
  return {};
}

mstch::map t_mstch_generator::extend_annotation(const annotation&) {
  return {};
}

void t_mstch_generator::gen_template_map() {
  for (size_t i = 0; i < templates_size; ++i) {
    auto name = std::filesystem::path(
        templates_name_datas[i],
        templates_name_datas[i] + templates_name_sizes[i]);
    name = name.parent_path() / name.stem();

    auto tpl = std::string(
        templates_content_datas[i],
        templates_content_datas[i] + templates_content_sizes[i]);
    // Remove a single '\n' or '\r\n' or '\r' at end, if present.
    chomp_last_char(&tpl, '\n');
    chomp_last_char(&tpl, '\r');
    template_map_.emplace(name.generic_string(), std::move(tpl));
  }
}

const std::string& t_mstch_generator::get_template(
    const std::string& template_name) {
  auto itr = template_map_.find(template_name);
  if (itr == template_map_.end()) {
    std::ostringstream err;
    err << "Could not find template \"" << template_name << "\"";
    throw std::runtime_error{err.str()};
  }
  return itr->second;
}

void t_mstch_generator::write_output(
    const std::filesystem::path& path, const std::string& data) {
  auto base_path = std::filesystem::path{get_out_dir()};
  auto abs_path = detail::make_abs_path(base_path, path);
  std::filesystem::create_directories(abs_path.parent_path());
  std::ofstream ofs{abs_path.string()};
  if (!ofs) {
    std::ostringstream err;
    err << "Couldn't open \"" << abs_path.string() << "\" for writing.";
    throw std::runtime_error{err.str()};
  }
  ofs << data;
  if (!is_last_char(data, '\n')) {
    // Terminate with newline.
    ofs << '\n';
  }
  record_genfile(abs_path.string());
}

bool t_mstch_generator::has_option(const std::string& option) const {
  return options_.find(option) != options_.end();
}

std::optional<std::string> t_mstch_generator::get_option(
    const std::string& option) const {
  auto itr = options_.find(option);
  return itr != options_.end() ? itr->second : std::optional<std::string>();
}

mstch::map t_mstch_generator::prepend_prefix(
    const std::string& prefix, mstch::map map) {
  mstch::map res;
  for (auto& pair : map) {
    res.emplace(prefix + ":" + pair.first, std::move(pair.second));
  }
  return res;
}

t_mstch_generator::whisker_render_state
t_mstch_generator::gen_whisker_render_state(whisker_options whisker_opts) {
  /**
   * This implementation of whisker's template resolver builds on top of the
   * template_map that already used by mstch. This allows template names to
   * remain the same (albeit exact file names are not recoverable).
   *
   * When a new partial is requested, we lazily parse the corresponding entry in
   * the template_map and cache the parsed AST.
   */
  class whisker_template_parser : public whisker::template_resolver {
   public:
    explicit whisker_template_parser(
        const std::map<std::string, std::string>& template_map,
        std::string template_prefix)
        : template_map_(template_map),
          template_prefix_(std::move(template_prefix)) {}

    std::optional<whisker::ast::root> resolve(
        const std::vector<std::string>& partial_path,
        source_location include_from,
        diagnostics_engine& diags) override {
      auto path = get_path_(partial_path, include_from);

      if (auto cached = cached_asts_.find(path); cached != cached_asts_.end()) {
        return cached->second;
      }

      auto source_code = template_map_.find(path);
      if (source_code == template_map_.end()) {
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
    std::string get_path_(
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
        std::filesystem::path current_file_path =
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

    const std::map<std::string, std::string>& template_map_;
    std::string template_prefix_;
    whisker::source_manager src_manager_;
    std::unordered_map<std::string, std::optional<whisker::ast::root>>
        cached_asts_;
  };

  auto template_parser = std::make_shared<whisker_template_parser>(
      get_template_map(), template_prefix());
  whisker::render_options render_options;
  render_options.partial_resolver = template_parser;
  render_options.strict_boolean_conditional = whisker::diagnostic_level::debug;
  render_options.strict_printable_types = whisker::diagnostic_level::debug;
  render_options.strict_undefined_variables = whisker::diagnostic_level::error;

  whisker::load_standard_library(render_options.globals);
  for (const auto& undefined_name : whisker_opts.allowed_undefined_variables) {
    render_options.globals.insert({undefined_name, whisker::make::null});
  }

  return whisker_render_state{
      whisker::diagnostics_engine(
          template_parser->source_manager(),
          [](const diagnostic& d) { fmt::print(stderr, "{}\n", d); },
          diagnostic_params::only_errors()),
      template_parser,
      std::move(render_options),
  };
}

std::string t_mstch_generator::render(
    const std::string& template_name, const mstch::node& context) {
  if (!whisker_render_state_.has_value()) {
    whisker_render_state_ = gen_whisker_render_state(render_options());
  }

  std::vector<std::string> partial_path;
  boost::algorithm::split(
      partial_path, template_name, [](char c) { return c == '/'; });

  std::optional<whisker::ast::root> ast =
      whisker_render_state_->template_resolver->resolve(
          partial_path, {}, whisker_render_state_->diagnostic_engine);
  if (!ast.has_value()) {
    throw std::runtime_error{
        fmt::format("Could not find template \"{}\"", template_name)};
  }

  std::ostringstream out;
  if (!whisker::render(
          out,
          *ast,
          whisker::from_mstch(context),
          whisker_render_state_->diagnostic_engine,
          whisker_render_state_->render_options)) {
    throw std::runtime_error{
        fmt::format("Failed to render template \"{}\"", template_name)};
  }
  return out.str();
}

void t_mstch_generator::render_to_file(
    const mstch::map& context,
    const std::string& template_name,
    const std::filesystem::path& path) {
  write_output(path, render(template_name, context));
}

const std::shared_ptr<mstch_base>& t_mstch_generator::cached_program(
    const t_program* program) {
  const auto& id = program->path();
  auto itr = mstch_context_.program_cache.find(id);
  if (itr == mstch_context_.program_cache.end()) {
    itr = mstch_context_.program_cache
              .emplace(
                  id,
                  mstch_context_.program_factory->make_mstch_object(
                      program, mstch_context_))
              .first;
  }
  return itr->second;
}

} // namespace apache::thrift::compiler
