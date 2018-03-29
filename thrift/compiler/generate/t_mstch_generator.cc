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
#include <thrift/compiler/generate/t_mstch_generator.h>

#include <algorithm>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

#include <mstch/mstch.hpp>

#include <thrift/compiler/common.h>
#include <thrift/compiler/generate/t_generator.h>

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

t_mstch_generator::t_mstch_generator(
    t_program* program,
    boost::filesystem::path template_prefix,
    std::map<std::string, std::string> parsed_options,
    bool convert_delimiter)
    : t_generator(program),
      template_dir_(g_template_dir),
      parsed_options_(std::move(parsed_options)),
      convert_delimiter_(convert_delimiter),
      generators_(std::make_shared<mstch_generators>()),
      cache_(std::make_shared<mstch_cache>()) {
  cache_->parsed_options_ = parsed_options_;
  if (template_dir_ == "") {
    std::string s = "Must set template directory when using mstch generator";
    throw std::runtime_error{s};
  }

  gen_template_map(template_dir_ / template_prefix, "");
}

mstch::map t_mstch_generator::dump(const t_program& program) {
  mstch::map result{
      {"name", program.get_name()},
      {"path", program.get_path()},
      {"outPath", program.get_out_path()},
      {"namespace", program.get_namespace()},
      {"includePrefix", program.get_include_prefix()},
      {"structs", dump_elems(program.get_objects())},
      {"enums", dump_elems(program.get_enums())},
      {"services", dump_elems(program.get_services())},
      {"typedefs", dump_elems(program.get_typedefs())},
      {"constants", dump_elems(program.get_consts())},
  };

  mstch::map extension = extend_program(program);
  result.insert(extension.begin(), extension.end());
  return prepend_prefix("program", std::move(result));
}

mstch::map t_mstch_generator::dump(const t_struct& strct, bool shallow) {
  mstch::map result{
      {"name", strct.get_name()},
      {"fields?", !strct.get_members().empty()},
      {"fields",
       shallow ? static_cast<mstch::node>(false)
               : dump_elems(strct.get_members())},
      {"exception?", strct.is_xception()},
      {"union?", strct.is_union()},
      {"plain?", !strct.is_xception() && !strct.is_union()},
      {"annotations", dump_elems(strct.annotations_)},
  };

  mstch::map extension = extend_struct(strct);
  result.insert(extension.begin(), extension.end());
  return prepend_prefix("struct", std::move(result));
}

mstch::map t_mstch_generator::dump(const t_field& field, int32_t index) {
  auto req = field.get_req();
  mstch::map result{
      {"name", field.get_name()},
      {"key", std::to_string(field.get_key())},
      {"type", dump(*field.get_type())},
      {"index", std::to_string(index)},
      {"index_plus_one", std::to_string(index + 1)},
      {"required?", req == t_field::e_req::T_REQUIRED},
      {"optional?", req == t_field::e_req::T_OPTIONAL},
      {"optInReqOut?", req == t_field::e_req::T_OPT_IN_REQ_OUT},
      {"annotations", dump_elems(field.annotations_)},
  };

  if (field.get_value() != nullptr) {
    result.emplace("value", dump(*field.get_value()));
  }

  mstch::map extension = extend_field(field);
  result.insert(extension.begin(), extension.end());
  return prepend_prefix("field", std::move(result));
}

mstch::map t_mstch_generator::dump(const t_type& orig_type) {
  const t_type& type =
      should_resolve_typedefs() ? resolve_typedef(orig_type) : orig_type;

  mstch::map result{
      {"name", type.get_name()},
      {"annotations", dump_elems(type.annotations_)},

      {"void?", type.is_void()},
      {"string?", type.is_string() && !type.is_binary()},
      {"binary?", type.is_string() && type.is_binary()},
      {"bool?", type.is_bool()},
      {"byte?", type.is_byte()},
      {"i16?", type.is_i16()},
      {"i32?", type.is_i32()},
      {"i64?", type.is_i64()},
      {"double?", type.is_double()},
      {"float?", type.is_float()},
      {"floating_point?", type.is_floating_point()},
      {"struct?", type.is_struct() || type.is_xception()},
      {"enum?", type.is_enum()},
      {"stream?", type.is_stream()},
      {"service?", type.is_service()},
      {"base?", type.is_base_type()},
      {"container?", type.is_container()},
      {"list?", type.is_list()},
      {"set?", type.is_set()},
      {"map?", type.is_map()},
      {"typedef?", type.is_typedef()},
  };

  if (type.is_struct() || type.is_xception()) {
    // Shallow dump the struct
    result.emplace("struct", dump(dynamic_cast<const t_struct&>(type), true));
  } else if (type.is_enum()) {
    result.emplace("enum", dump(dynamic_cast<const t_enum&>(type)));
  } else if (type.is_service()) {
    result.emplace("service", dump(dynamic_cast<const t_service&>(type)));
  } else if (type.is_list()) {
    result.emplace(
        "listElemType",
        dump(*dynamic_cast<const t_list&>(type).get_elem_type()));
  } else if (type.is_stream()) {
    result.emplace(
        "streamElemType",
        dump(*dynamic_cast<const t_stream&>(type).get_elem_type()));
  } else if (type.is_set()) {
    result.emplace(
        "setElemType", dump(*dynamic_cast<const t_set&>(type).get_elem_type()));
    result.emplace(
        "unordered?", dynamic_cast<const t_set&>(type).is_unordered());
  } else if (type.is_map()) {
    result.emplace(
        "unordered?", dynamic_cast<const t_map&>(type).is_unordered());
    result.emplace(
        "keyType", dump(*dynamic_cast<const t_map&>(type).get_key_type()));
    result.emplace(
        "valueType", dump(*dynamic_cast<const t_map&>(type).get_val_type()));
  } else if (type.is_typedef()) {
    result.emplace(
        "typedefType", dump(*dynamic_cast<const t_typedef&>(type).get_type()));
  }

  mstch::map extension = extend_type(type);
  result.insert(extension.begin(), extension.end());
  return prepend_prefix("type", std::move(result));
}

mstch::map t_mstch_generator::dump(const t_enum& enm) {
  mstch::map result{
      {"name", enm.get_name()},
      {"values", dump_elems(enm.get_constants())},
      {"annotations", dump_elems(enm.annotations_)},
  };

  mstch::map extension = extend_enum(enm);
  result.insert(extension.begin(), extension.end());
  return prepend_prefix("enum", std::move(result));
}

mstch::map t_mstch_generator::dump(const t_enum_value& val) {
  mstch::map result{
      {"name", val.get_name()},
      {"value", std::to_string(val.get_value())},
  };

  mstch::map extension = extend_enum_value(val);
  result.insert(extension.begin(), extension.end());
  return prepend_prefix("enumValue", std::move(result));
}

mstch::map t_mstch_generator::dump(const t_service& service) {
  t_service* extends = service.get_extends();
  mstch::map result{
      {"name", service.get_name()},
      {"annotations", dump_elems(service.annotations_)},
      {"functions", dump_elems(service.get_functions())},
      {"functions?", !service.get_functions().empty()},
      {"extends?", extends != nullptr},
      {"extends", extends ? static_cast<mstch::node>(dump(*extends)) : false},
  };

  mstch::map extension = extend_service(service);
  result.insert(extension.begin(), extension.end());
  return prepend_prefix("service", std::move(result));
}

mstch::map t_mstch_generator::dump(const t_function& function) {
  mstch::map result{
      {"name", function.get_name()},
      {"oneway?", function.is_oneway()},
      {"returnType", dump(*function.get_returntype())},
      {"exceptions", dump_elems(function.get_xceptions()->get_members())},
      {"exceptions?", !function.get_xceptions()->get_members().empty()},
      {"annotations",
       function.get_annotations()
           ? dump_elems(function.get_annotations()->annotations_)
           : static_cast<mstch::node>(false)},
      {"args", dump_elems(function.get_arglist()->get_members())},
  };

  mstch::map extension = extend_function(function);
  result.insert(extension.begin(), extension.end());
  return prepend_prefix("function", std::move(result));
}

mstch::map t_mstch_generator::dump(const t_const& cnst) {
  mstch::map result{
      {"type", dump(*cnst.get_type())},
      {"name", cnst.get_name()},
      {"value", dump(*cnst.get_value())},
  };

  mstch::map extension = extend_const(cnst);
  result.insert(extension.begin(), extension.end());
  return prepend_prefix("constant", std::move(result));
}

mstch::map t_mstch_generator::dump(const t_const_value& value) {
  using cv = t_const_value::t_const_value_type;
  const cv type = value.get_type();
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

  auto const format_double_string = [](const double d) {
    std::string d_str = std::to_string(d);
    d_str.erase(d_str.find_last_not_of('0') + 1);
    if (d_str.back() == '.')
      d_str.push_back('0');
    return d_str;
  };

  switch (type) {
    case cv::CV_DOUBLE:
      result.emplace("value", format_double_string(value.get_double()));
      result.emplace("doubleValue", format_double_string(value.get_double()));
      result.emplace("nonzero?", value.get_double() != 0.0);
      break;
    case cv::CV_BOOL:
      result.emplace("value", std::to_string(value.get_bool()));
      result.emplace("boolValue", value.get_bool() == true);
      result.emplace("nonzero?", value.get_bool() == true);
      break;
    case cv::CV_INTEGER:
      if (value.get_enum_value()) {
        result.emplace("enum_name", value.get_enum()->get_name());
        result.emplace("enum_value_name", value.get_enum_value()->get_name());
      }
      result.emplace("value", std::to_string(value.get_integer()));
      result.emplace("integerValue", std::to_string(value.get_integer()));
      result.emplace("nonzero?", value.get_integer() != 0);
      break;
    case cv::CV_STRING:
      result.emplace("value", value.get_string());
      result.emplace("stringValue", value.get_string());
      break;
    case cv::CV_MAP:
      result.emplace("mapElements", dump_elems(value.get_map()));
      break;
    case cv::CV_LIST:
      result.emplace("listElements", dump_elems(value.get_list()));
      break;
    default:
      std::ostringstream err;
      err << "Unhandled t_const_value_type " << value.get_type();
      throw std::domain_error{err.str()};
  }

  mstch::map extension = extend_const_value(value);
  result.insert(extension.begin(), extension.end());
  return prepend_prefix("value", std::move(result));
}

mstch::map t_mstch_generator::dump(
    const std::map<t_const_value*, t_const_value*>::value_type& pair) {
  mstch::map result{
      {"key", dump(*pair.first)},
      {"value", dump(*pair.second)},
  };
  mstch::map extension = extend_const_value_map_elem(pair);
  result.insert(extension.begin(), extension.end());
  return prepend_prefix("element", std::move(result));
}

mstch::map t_mstch_generator::dump(const annotation& pair) {
  mstch::map result{
      {"key", pair.first},
      {"value", pair.second},
  };
  mstch::map extension = extend_annotation(pair);
  result.insert(extension.begin(), extension.end());
  return prepend_prefix("annotation", std::move(result));
}

mstch::map t_mstch_generator::dump(const t_typedef& typdef) {
  mstch::map result{
      {"type", dump(*typdef.get_type())},
      {"symbolic", typdef.get_symbolic()},
  };

  mstch::map extension = extend_typedef(typdef);
  result.insert(extension.begin(), extension.end());
  return prepend_prefix("typedef", std::move(result));
}

mstch::map t_mstch_generator::dump(const string& value) {
  mstch::map result{{"value", value}};
  return result;
}

mstch::map t_mstch_generator::dump_options() {
  mstch::map result;
  for (auto& elem : parsed_options_) {
    result.emplace(elem.first, elem.second);
  }
  return prepend_prefix("option", std::move(result));
}

// Extenders, by default do no extending

mstch::map t_mstch_generator::extend_program(const t_program&) {
  return {};
}

mstch::map t_mstch_generator::extend_struct(const t_struct&) {
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

void t_mstch_generator::gen_template_map(
    const boost::filesystem::path& root,
    const std::string& sub_directory) {
  for (auto itr = boost::filesystem::directory_iterator{root};
       itr != boost::filesystem::directory_iterator{};
       ++itr) {
    if (boost::filesystem::is_regular_file(itr->path()) &&
        boost::filesystem::extension(itr->path()) == ".mustache") {
      std::ifstream ifs{itr->path().string()};
      auto tpl = std::string{std::istreambuf_iterator<char>(ifs),
                             std::istreambuf_iterator<char>()};
      // Remove a single '\n' or '\r\n' or '\r' at end, if present.
      chomp_last_char(&tpl, '\n');
      chomp_last_char(&tpl, '\r');
      if (convert_delimiter_) {
        tpl = "{{=<% %>=}}\n" + tpl;
      }

      template_map_.emplace(
          sub_directory + itr->path().stem().string(), std::move(tpl));
    } else if (boost::filesystem::is_directory(itr->path())) {
      gen_template_map(itr->path(), itr->path().filename().string() + "/");
    }
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
    const boost::filesystem::path& path,
    const std::string& data) {
  auto abs_path = boost::filesystem::path{get_out_dir()} / path;
  boost::filesystem::create_directories(abs_path.parent_path());
  std::ofstream ofs{abs_path.string()};
  ofs << data;
  if (!is_last_char(data, '\n')) {
    // Terminate with newline.
    ofs << '\n';
  }
  record_genfile(abs_path.string());
}

std::unique_ptr<std::string> t_mstch_generator::get_option(
    const std::string& key) {
  auto itr = parsed_options_.find(key);
  if (itr == parsed_options_.end()) {
    return nullptr;
  }
  return std::unique_ptr<std::string>(new std::string(itr->second));
}

const t_type& t_mstch_generator::resolve_typedef(const t_type& type) const {
  auto t = &type;
  while (t->is_typedef()) {
    t = dynamic_cast<const t_typedef*>(t)->get_type();
  }
  return *t;
}

mstch::map t_mstch_generator::prepend_prefix(
    const std::string& prefix,
    mstch::map map) {
  mstch::map res{};
  for (auto& pair : map) {
    res.emplace(prefix + ":" + pair.first, std::move(pair.second));
  }
  return res;
}

std::string t_mstch_generator::render(
    const std::string& template_name,
    const mstch::node& context) {
  return mstch::render(
      get_template(template_name), context, get_template_map());
}

void t_mstch_generator::render_to_file(
    const mstch::map& context,
    const std::string& template_name,
    const boost::filesystem::path& path) {
  write_output(path, render(template_name, context));
}
