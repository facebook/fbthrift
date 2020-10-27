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
} // namespace compiler
} // namespace thrift
} // namespace apache
