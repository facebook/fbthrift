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

#include <thrift/compiler/generate/common.h>

std::vector<std::string> split_namespace(const std::string& s) {
  std::string token = ".";
  std::size_t last_match = 0;
  std::size_t next_match = s.find(token);

  std::vector<std::string> output;
  while (next_match != std::string::npos) {
    output.push_back(s.substr(last_match, next_match - last_match));
    last_match = next_match + 1;
    next_match = s.find(token, last_match);
  }
  output.push_back(s.substr(last_match));

  return output;
}
