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

#pragma once

#include <algorithm>
#include <string_view>

namespace apache::thrift::compiler {

inline bool is_reserved_identifier(std::string_view name) {
  const std::string_view prefix = "fbthrift";

  auto pos = name.find_first_not_of('_');
  if (pos == std::string_view::npos) {
    return false;
  }

  auto after_underscores =
      std::string_view(name.data() + pos, name.size() - pos);

  if (after_underscores.size() < prefix.size()) {
    return false;
  }
  return std::equal(
      prefix.begin(),
      prefix.end(),
      after_underscores.begin(),
      [](char a, char b) {
        auto lower = [](char c) {
          return c >= 'A' && c <= 'Z' ? static_cast<char>(c - 'A' + 'a') : c;
        };
        return lower(a) == lower(b);
      });
}

} // namespace apache::thrift::compiler
