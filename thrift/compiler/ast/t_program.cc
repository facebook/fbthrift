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

#include <thrift/compiler/ast/t_program.h>

#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace apache {
namespace thrift {
namespace compiler {

const std::string& t_program::get_namespace(const std::string& language) const {
  auto pos = namespaces_.find(language);
  static const auto& kEmpty = *new std::string();
  return (pos != namespaces_.end() ? pos->second : kEmpty);
}

std::unique_ptr<t_program>
t_program::add_include(std::string path, std::string include_site, int lineno) {
  auto program = std::make_unique<t_program>(path);

  std::string include_prefix;
  const auto last_slash = include_site.find_last_of("/\\");
  if (last_slash != std::string::npos) {
    include_prefix = include_site.substr(0, last_slash);
  }

  program->set_include_prefix(include_prefix);

  auto include = std::make_unique<t_include>(program.get());
  include->set_lineno(lineno);

  add_include(std::move(include));

  return program;
}

void t_program::set_include_prefix(std::string include_prefix) {
  include_prefix_ = std::move(include_prefix);

  int len = include_prefix_.size();
  if (len > 0 && include_prefix_[len - 1] != '/') {
    include_prefix_ += '/';
  }
}

std::string t_program::compute_name_from_file_path(std::string path) {
  std::string::size_type slash = path.find_last_of("/\\");
  if (slash != std::string::npos) {
    path = path.substr(slash + 1);
  }
  std::string::size_type dot = path.rfind('.');
  if (dot != std::string::npos) {
    path = path.substr(0, dot);
  }
  return path;
}

} // namespace compiler
} // namespace thrift
} // namespace apache
