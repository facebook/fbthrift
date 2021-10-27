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

#include <thrift/compiler/ast/diagnostic.h>

#include <sstream>
#include <string>

namespace apache {
namespace thrift {
namespace compiler {

namespace {
const char* level_to_string(diagnostic_level level) {
  switch (level) {
    case diagnostic_level::parse_error:
      return "ERROR";
    case diagnostic_level::failure:
      return "FAILURE";
    case diagnostic_level::warning:
      return "WARNING";
    case diagnostic_level::info:
      return "INFO";
    case diagnostic_level::debug:
      return "DEBUG";
  }
  return "??";
}
} // namespace

std::string diagnostic::str() const {
  std::ostringstream ss;
  ss << *this;
  return ss.str();
}

std::ostream& operator<<(std::ostream& os, const diagnostic& e) {
  os << "[" << level_to_string(e.level()) << ":" << e.file();
  if (e.lineno() != 0) {
    os << ":" << e.lineno();
  }
  os << "] ";
  if (!e.token().empty()) {
    os << "(last token was '" << e.token() << "') ";
  }
  os << e.message();
  if (!e.name().empty()) {
    return os << " [" << e.name() << "]";
  }
  return os;
}

void diagnostic_results::add(diagnostic diag) {
  increment(diag.level());
  diagnostics_.emplace_back(std::move(diag));
}

diagnostic_results::diagnostic_results(
    std::vector<diagnostic> initial_diagnostics)
    : diagnostics_(std::move(initial_diagnostics)) {
  for (const auto& diag : diagnostics_) {
    increment(diag.level());
  }
}

} // namespace compiler
} // namespace thrift
} // namespace apache
