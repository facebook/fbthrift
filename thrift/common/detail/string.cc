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

#include <thrift/common/detail/string.h>

#include <algorithm>

namespace apache::thrift::detail {
namespace {

constexpr char lowered(char ch) {
  return ch >= 'A' && ch <= 'Z' ? static_cast<char>(ch + ('a' - 'A')) : ch;
}

} // namespace

std::string escape(std::string_view str) {
  std::string result;
  for (const char ch : str) {
    switch (ch) {
      case '\\':
        result += "\\\\";
        break;
      case '\n':
        result += "\\n";
        break;
      case '\r':
        result += "\\r";
        break;
      case '\t':
        result += "\\t";
        break;
      case '\v':
        result += "\\v";
        break;
      case '\f':
        result += "\\f";
        break;
      case '\'':
        result += "\\'";
        break;
      default:
        result += ch;
    }
  }
  return result;
}

void replace_all(std::string& str, std::string_view from, std::string_view to) {
  if (from.empty()) {
    return;
  }
  std::size_t pos = str.find(from);
  if (pos == std::string::npos) {
    return;
  }
  // Build the result in one pass. Replacing in place would shift the tail of
  // the string on every match whenever `from` and `to` differ in length.
  std::string result;
  result.reserve(str.size());
  std::size_t copied = 0;
  do {
    result.append(str, copied, pos - copied);
    result.append(to);
    copied = pos + from.size();
    pos = str.find(from, copied);
  } while (pos != std::string::npos);
  result.append(str, copied);
  str = std::move(result);
}

std::string replace_all_copy(
    std::string_view str, std::string_view from, std::string_view to) {
  std::string result(str);
  replace_all(result, from, to);
  return result;
}

void to_lower_ascii(std::string& str) {
  for (char& ch : str) {
    ch = lowered(ch);
  }
}

void to_upper_ascii(std::string& str) {
  for (char& ch : str) {
    if (ch >= 'a' && ch <= 'z') {
      ch -= 'a' - 'A';
    }
  }
}

bool iequals(std::string_view a, std::string_view b) {
  return a.size() == b.size() &&
      std::equal(a.begin(), a.end(), b.begin(), [](char x, char y) {
           return lowered(x) == lowered(y);
         });
}

bool istarts_with(std::string_view str, std::string_view prefix) {
  return str.size() >= prefix.size() &&
      iequals(str.substr(0, prefix.size()), prefix);
}

} // namespace apache::thrift::detail
