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

#include <algorithm>
#include <sstream>
#include <stdexcept>

#include "thrift/compiler/ast/t_const_value.h"

namespace apache {
namespace thrift {
namespace compiler {

std::unique_ptr<t_const_value> t_const_value::clone() const {
  auto clone = std::make_unique<t_const_value>();

  switch (get_type()) {
    case CV_BOOL:
      clone->set_bool(get_bool());
      break;
    case CV_INTEGER:
      clone->set_integer(get_integer());
      break;
    case CV_DOUBLE:
      clone->set_double(get_double());
      break;
    case CV_STRING:
      clone->set_string(get_string());
      break;
    case CV_MAP:
      clone->set_map();
      for (auto const& map_elem : get_map()) {
        clone->add_map(map_elem.first->clone(), map_elem.second->clone());
      }
      break;
    case CV_LIST:
      clone->set_list();
      for (auto const& list_elem : get_list()) {
        clone->add_list(list_elem->clone());
      }
      break;
  }

  clone->set_owner(get_owner());
  clone->set_ttype(get_ttype());
  clone->set_is_enum(is_enum());
  clone->set_enum(get_enum());
  clone->set_enum_value(get_enum_value());

  return clone;
}

void t_const_value::check_val_type(
    std::initializer_list<t_const_value_type> types) const {
  if (std::find(types.begin(), types.end(), valType_) == types.end()) {
    std::ostringstream os;
    os << "t_const_value type mismatch: valType_=" << valType_
       << " is not any of {";
    auto delim = "";
    for (auto t : types) {
      os << delim << t;
      delim = ", ";
    }
    os << "}";
    throw std::runtime_error(os.str());
  }
}

bool t_const_value::is_empty() const {
  switch (valType_) {
    case CV_MAP:
      return mapVal_.empty();
    case CV_LIST:
      return listVal_.empty();
    case CV_STRING:
      return stringVal_.empty();
    default:
      return false;
  }
}

} // namespace compiler
} // namespace thrift
} // namespace apache
