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

#include <thrift/compiler/util.h>

#include <ostream>
#include <sstream>
#include <vector>

#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/split.hpp>

namespace apache {
namespace thrift {
namespace compiler {

std::string strip_left_margin(std::string const& s) {
  constexpr auto const strippable = " \t";

  if (s.empty()) {
    return s;
  }

  // step: split
  std::vector<std::string> lines;
  boost::algorithm::split(lines, s, [](auto const c) { return c == '\n'; });

  // step: preprocess
  if (lines.back().find_first_not_of(strippable) == std::string::npos) {
    lines.back().clear();
  }
  if (lines.front().find_first_not_of(strippable) == std::string::npos) {
    lines.erase(lines.begin());
  }

  // step: find the left margin
  constexpr auto const sentinel = std::numeric_limits<size_t>::max();
  auto indent = sentinel;
  size_t max_length = 0;
  for (auto& line : lines) {
    auto const needle = line.find_first_not_of(strippable);
    if (needle == std::string::npos) {
      max_length = std::max(max_length, line.size());
    } else {
      indent = std::min(indent, needle);
    }
  }
  indent = indent == sentinel ? max_length : indent;

  // step: strip the left margin
  for (auto& line : lines) {
    line = line.substr(std::min(indent, line.size()));
  }

  // step: join
  return boost::algorithm::join(lines, "\n");
}

std::string json_quote_ascii(std::string const& s) {
  std::ostringstream o;
  json_quote_ascii(o, s);
  return o.str();
}

std::ostream& json_quote_ascii(std::ostream& o, std::string const& s) {
  o << "\"";
  for (char const c : s) {
    switch (c) {
      // clang-format off
      case '"':  o << "\\\""; break;
      case '\\': o << "\\\\"; break;
      case '\b': o << "\\b";  break;
      case '\f': o << "\\f";  break;
      case '\n': o << "\\n";  break;
      case '\r': o << "\\r";  break;
      // clang-format on
      default: {
        uint8_t const b = uint8_t(c);
        if (!(b >= 0x20 && b < 0x80)) {
          constexpr auto const hex = "0123456789abcdef";
          auto const c1 = char(hex[(b >> 4) & 0x0f]);
          auto const c0 = char(hex[(b >> 0) & 0x0f]);
          o << "\\u00" << c1 << c0;
        } else {
          o << c;
        }
      }
    }
  }
  o << "\"";
  return o;
}

} // namespace compiler
} // namespace thrift
} // namespace apache
