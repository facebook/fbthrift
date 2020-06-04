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

#include <thrift/compiler/validator/diagnostic.h>

#include <sstream>
#include <string>

namespace apache {
namespace thrift {
namespace compiler {

std::string diagnostic::str() {
  std::ostringstream ss;
  ss << "[" << getStringFromType(type_) << ":" << file_;
  if (line_) {
    ss << ":" << line_.value();
  }
  ss << "] " << message_;
  return ss.str();
}

std::ostream& operator<<(std::ostream& os, diagnostic e) {
  return os << e.str();
}

std::string diagnostic::getStringFromType(diagnostic::type type) {
  switch (type) {
    case diagnostic::type::failure:
      return "FAILURE";
    case diagnostic::type::warning:
      return "WARNING";
    case diagnostic::type::info:
      return "INFO";
    default:
      return "Undefined diagnostic type";
  }
}

} // namespace compiler
} // namespace thrift
} // namespace apache
