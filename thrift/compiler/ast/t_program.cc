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

#include <thrift/compiler/ast/t_program.h>

#include <cctype>
#include <map>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <boost/algorithm/string/split.hpp>

namespace apache {
namespace thrift {
namespace compiler {

namespace {

const char kDomainDelim[] = ".";
const char kPathDelim[] = "/";

std::string genPrefix(
    const std::vector<std::string>& domain,
    const std::vector<std::string>& path) {
  std::string result;
  const auto* delim = "";
  for (const auto& label : domain) {
    result += delim;
    result += label;
    delim = kDomainDelim;
  }
  delim = kPathDelim;
  for (const auto& segment : path) {
    result += delim;
    result += segment;
  }
  result += delim;
  return result;
}

void check(bool cond, const char* err) {
  if (!cond) {
    throw std::invalid_argument(err);
  }
}

bool isDomainChar(char c) {
  return std::isdigit(c) || std::islower(c) || c == '-';
}

bool isPathChar(char c) {
  return isDomainChar(c) || c == '_';
}

void checkDomainLabel(const std::string& label) {
  check(!label.empty(), "empty domain label");
  for (const auto& c : label) {
    check(isDomainChar(c), "invalid domain char");
  }
}

void checkDomain(const std::vector<std::string>& domain) {
  check(domain.size() >= 2, "not enough domain labels");
  for (const auto& label : domain) {
    checkDomainLabel(label);
  }
}

void checkPathSegment(const std::string& seg) {
  check(!seg.empty(), "empty path segment");
  for (const auto& c : seg) {
    check(isPathChar(c), "invalid path char");
  }
}

void checkPath(const std::vector<std::string>& path) {
  check(!path.empty(), "not enough path segments");
  for (const auto& seg : path) {
    checkPathSegment(seg);
  }
}

std::vector<std::string> parseDomain(const std::string& domain) {
  std::vector<std::string> labels;
  boost::algorithm::split(
      labels, domain, [](auto ch) { return ch == kDomainDelim[0]; });
  checkDomain(labels);
  return labels;
}

} // namespace

t_package::t_package(std::string name) : uriPrefix_(std::move(name)) {
  boost::algorithm::split(
      path_, uriPrefix_, [](auto ch) { return ch == kPathDelim[0]; });
  check(path_.size() >= 2, "invalid package name");
  domain_ = parseDomain(path_.front());
  path_.erase(path_.begin());
  checkPath(path_);
  uriPrefix_ += "/";
}

std::string t_package::get_uri(const std::string& name) const {
  if (empty()) {
    return {}; // Package is empty, so no URI.
  }
  return uriPrefix_ + name;
}

t_package::t_package(
    std::vector<std::string> domain, std::vector<std::string> path)
    : domain_(std::move(domain)), path_(std::move(path)) {
  checkDomain(domain_);
  checkPath(path_);
  uriPrefix_ = genPrefix(domain_, path_);
}

void t_program::add_definition(std::unique_ptr<t_named> definition) {
  assert(definition != nullptr);
  // Index the node.
  if (auto* node = dynamic_cast<t_exception*>(definition.get())) {
    objects_.push_back(node);
    exceptions_.push_back(node);
  } else if (auto* node = dynamic_cast<t_struct*>(definition.get())) {
    objects_.push_back(node);
    structs_.push_back(node);
  } else if (auto* node = dynamic_cast<t_interaction*>(definition.get())) {
    interactions_.push_back(node);
  } else if (auto* node = dynamic_cast<t_service*>(definition.get())) {
    services_.push_back(node);
  } else if (auto* node = dynamic_cast<t_enum*>(definition.get())) {
    enums_.push_back(node);
  } else if (auto* node = dynamic_cast<t_typedef*>(definition.get())) {
    typedefs_.push_back(node);
  } else if (auto* node = dynamic_cast<t_const*>(definition.get())) {
    consts_.push_back(node);
  }

  // Transfer ownership of the definition.
  definitions_.push_back(std::move(definition));
}

const std::string& t_program::get_namespace(const std::string& language) const {
  auto pos = namespaces_.find(language);
  static const auto& kEmpty = *new std::string();
  return (pos != namespaces_.end() ? pos->second : kEmpty);
}

std::unique_ptr<t_program> t_program::add_include(
    std::string path, std::string include_site, int lineno) {
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

size_t t_program::get_byte_offset(
    size_t line, size_t line_offset) const noexcept {
  if (line == 0) {
    return noffset; // Not specified.
  }
  if (line > line_to_offset_.size()) {
    return noffset; // No offset data provided.
  }
  return line_to_offset_[line - 1] + line_offset;
}

} // namespace compiler
} // namespace thrift
} // namespace apache
