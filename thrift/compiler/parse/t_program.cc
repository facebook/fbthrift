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

#include <thrift/compiler/parse/t_program.h>

#include <map>
#include <memory>
#include <utility>
#include <string>
#include <vector>

#include <folly/MapUtil.h>

const std::string& t_program::get_namespace(const std::string& language) const {
  static const auto& kEmpty = *new std::string();
  return folly::get_ref_default(namespaces_, language, kEmpty);
}

void t_program::set_out_path(std::string out_path, bool out_path_is_absolute) {
  out_path_ = std::move(out_path);
  out_path_is_absolute_ = out_path_is_absolute;
  if(!out_path_.empty()) {
    if (!(out_path_.back() == '/' || out_path_.back() == '\\')) {
      out_path_.push_back('/');
    }
  }
}

void t_program::add_include(std::string path, std::string include_site) {
  t_program* program = new t_program(path);

  std::string include_prefix;
  const auto last_slash = include_site.rfind("/");
  if (last_slash != std::string::npos) {
    include_prefix = include_site.substr(0, last_slash);
  }

  program->set_include_prefix(include_prefix);
  includes_.push_back(program);
}

void t_program::set_include_prefix(std::string include_prefix) {
  include_prefix_ = std::move(include_prefix);

  int len = include_prefix_.size();
  if (len > 0 && include_prefix_[len - 1] != '/') {
    include_prefix_ += '/';
  }
}

std::string t_program::compute_name_from_file_path(std::string path) {
  std::string::size_type slash = path.rfind("/");
  if (slash != std::string::npos) {
    path = path.substr(slash+1);
  }
  std::string::size_type dot = path.rfind(".");
  if (dot != std::string::npos) {
    path = path.substr(0, dot);
  }
  return path;
}
