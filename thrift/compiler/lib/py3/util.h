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

#pragma once

#include <map>
#include <string>
#include <unordered_set>

namespace apache {
namespace thrift {
namespace compiler {
namespace py3 {

template <class T>
std::string get_py3_name(const T& elem) {
  // Reserved Cython / Python keywords that are not blocked by thrift grammer
  // TODO: get rid of this list and force users to rename explicitly
  static const std::unordered_set<std::string> keywords = {
      "DEF",   "ELIF",  "ELSE", "False",   "IF",    "None",     "True",
      "async", "await", "cdef", "cimport", "cpdef", "cppclass", "ctypedef",
      "def",   "elif",  "else", "from",    "if",    "nonlocal",
  };
  const std::map<std::string, std::string>& annotations = elem.annotations_;
  const auto& it = annotations.find("py3.name");
  if (it != annotations.end()) {
    return it->second;
  }
  if (keywords.find(elem.get_name()) != keywords.end()) {
    return elem.get_name() + "_";
  }
  return elem.get_name();
}

} // namespace py3
} // namespace compiler
} // namespace thrift
} // namespace apache
