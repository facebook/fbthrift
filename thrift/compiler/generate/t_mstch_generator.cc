/*
 * Copyright 2016 Facebook, Inc.
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
#include <thrift/compiler/common.h>
#include <thrift/compiler/generate/t_generator.h>

#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

#include <mstch/mstch.hpp>

t_mstch_generator::t_mstch_generator(
    t_program* program,
    boost::filesystem::path template_prefix)
    : t_generator(program), template_dir_(g_template_dir) {
  if (this->template_dir_ == "") {
    std::string s = "Must set template directory when using mstch generator";
    throw std::runtime_error{s};
  }

  this->gen_template_map(template_prefix);
}

mstch::map t_mstch_generator::dump(const t_program& program) const {
  return mstch::map{
      {"program:name", program.get_name()},
      {"program:path", program.get_path()},
      {"program:outPath", program.get_out_path()},
      {"program:namespace", program.get_namespace()},
      {"program:includePrefix", program.get_include_prefix()},
      {"program:structs", this->dump_vector(program.get_structs())},
      {"program:exceptions", this->dump_vector(program.get_xceptions())},
      {"program:enums", this->dump_vector(program.get_enums())},
      {"program:services", this->dump_vector(program.get_services())},
      {"program:typedefs", this->dump_vector(program.get_typedefs())},
      {"program:consts", this->dump_vector(program.get_consts())},
  };
}

mstch::map t_mstch_generator::dump(const t_struct& strct) const {
  return mstch::map{
      {"struct:name", strct.get_name()},
      {"struct:fields", this->dump_vector(strct.get_sorted_members())},
  };
}

mstch::map t_mstch_generator::dump(const t_field& field) const {
  mstch::map result{
      {"field:name", field.get_name()},
      {"field:key", std::to_string(field.get_key())},
      {"field:type", this->dump(*field.get_type())},
  };

  switch (field.get_req()) {
    case t_field::e_req::T_REQUIRED:
      result.insert({"field:required?", true});
      break;
    case t_field::e_req::T_OPTIONAL:
      result.insert({"field:optional?", true});
      break;
    case t_field::e_req::T_OPT_IN_REQ_OUT:
      result.insert({"field:optInReqOut?", true});
      break;
    default:
      // Set nothing
      break;
  }

  return result;
}

mstch::map t_mstch_generator::dump(const t_type& type) const {
  return mstch::map{
      {"type:name", type.get_name()},
  };
}

mstch::map t_mstch_generator::dump(const t_enum& enm) const {
  // TODO
  return {};
}

mstch::map t_mstch_generator::dump(const t_service& service) const {
  mstch::map result{
      {"service:name", service.get_name()},
      {"service:functions", this->dump_vector(service.get_functions())},
  };

  t_service* extends = service.get_extends();
  if (extends) {
    result.insert({"service:extends?", true});
    result.insert({"service:extends", this->dump(*extends)});
  }

  return result;
}

mstch::map t_mstch_generator::dump(const t_function& function) const {
  return mstch::map{
      {"function:name", function.get_name()},
      {"function:returnType", this->dump(*function.get_returntype())},
      {"function:args",
       this->dump_vector(function.get_arglist()->get_members())}};
}

mstch::map t_mstch_generator::dump(const t_const& cnst) const {
  return mstch::map{
      {"const:type", this->dump(*cnst.get_type())},
      {"const:name", cnst.get_name()},
      {"const:value", this->dump(*cnst.get_value())},
  };
}

mstch::map t_mstch_generator::dump(const t_const_value& value) const {
  mstch::map result{};

  switch (value.get_type()) {
    case t_const_value::t_const_value_type::CV_DOUBLE:
      result.insert({"value:double?", true});
      result.insert({"value.value", std::to_string(value.get_double())});
      break;
    case t_const_value::t_const_value_type::CV_INTEGER:
      result.insert({"value:integer?", true});
      result.insert({"value:value", std::to_string(value.get_integer())});
      break;
    case t_const_value::t_const_value_type::CV_STRING:
      result.insert({"value:string?", true});
      result.insert({"value:value", value.get_string()});
      break;
    case t_const_value::t_const_value_type::CV_MAP:
      result.insert({"value:map?", true});
      {
        mstch::array elements{};
        for (const auto& pair : value.get_map()) {
          mstch::map elem{
              {"pair:first", this->dump(*pair.first)},
              {"pair:second", this->dump(*pair.second)},
          };
          elements.push_back(elem);
        }
        result.insert({"value:elements", elements});
      }
      break;
    case t_const_value::t_const_value_type::CV_LIST:
      result.insert({"value:list?", true});
      {
        mstch::array elements{};
        for (const t_const_value* innerValue : value.get_list()) {
          elements.push_back(this->dump(*innerValue));
        }
        result.insert({"value:elements", elements});
      }
      break;
    default:
      failure("Unhandled t_const_value_type %d\n", value.get_type());
  }

  return result;
}

mstch::map t_mstch_generator::dump(const t_typedef& typdef) const {
  // TODO
  return {};
}

void t_mstch_generator::gen_template_map(boost::filesystem::path prefix) {
  auto template_dir = this->template_dir_ / prefix;
  this->template_map_ = {};
  for (auto& elem : boost::filesystem::directory_iterator(template_dir)) {
    if (boost::filesystem::is_regular_file(elem.path())) {
      std::ifstream ifs{elem.path().string()};
      auto tpl = std::string{std::istreambuf_iterator<char>(ifs),
                             std::istreambuf_iterator<char>()};

      this->template_map_.insert({elem.path().filename().string(), tpl});
    }
  }
}
