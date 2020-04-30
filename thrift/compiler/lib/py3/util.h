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

inline const std::unordered_set<std::string>& get_python_reserved_names() {
  static const std::unordered_set<std::string> keywords = {
      "False",  "None",   "True",    "and",      "as",       "assert", "async",
      "await",  "break",  "class",   "continue", "def",      "del",    "elif",
      "else",   "except", "finally", "for",      "from",     "global", "if",
      "import", "in",     "is",      "lambda",   "nonlocal", "not",    "or",
      "pass",   "raise",  "return",  "try",      "while",    "with",   "yield",
  };
  return keywords;
}

namespace py3 {

template <class T>
std::string get_py3_name(const T& elem) {
  // Reserved Cython / Python keywords that are not blocked by thrift grammer
  // TODO: get rid of this list and force users to rename explicitly
  static const std::unordered_set<std::string> cython_keywords = {
      "DEF",
      "ELIF",
      "ELSE",
      "IF",
      "cdef",
      "cimport",
      "cpdef",
      "cppclass",
      "ctypedef",
  };
  const std::map<std::string, std::string>& annotations = elem.annotations_;
  const auto& it = annotations.find("py3.name");
  if (it != annotations.end()) {
    return it->second;
  }
  const auto& name = elem.get_name();
  const auto& python_keywords = get_python_reserved_names();
  if (cython_keywords.find(name) != cython_keywords.end() ||
      python_keywords.find(name) != python_keywords.end()) {
    return name + "_";
  }
  return name;
}

} // namespace py3
} // namespace compiler
} // namespace thrift
} // namespace apache
