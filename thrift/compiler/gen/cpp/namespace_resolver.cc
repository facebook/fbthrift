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

#include <thrift/compiler/gen/cpp/namespace_resolver.h>

#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/split.hpp>

namespace apache {
namespace thrift {
namespace compiler {
namespace gen {
namespace cpp {

namespace {

static bool is_dot(char const c) {
  return c == '.';
}

} // namespace

std::vector<std::string> namespace_resolver::gen_namespace_components(
    const t_program* program) {
  auto const& cpp2 = program->get_namespace("cpp2");
  auto const& cpp = program->get_namespace("cpp");

  std::vector<std::string> components;
  if (!cpp2.empty()) {
    boost::algorithm::split(components, cpp2, is_dot);
  } else if (!cpp.empty()) {
    boost::algorithm::split(components, cpp, is_dot);
    components.push_back("cpp2");
  } else {
    components.push_back("cpp2");
  }

  return components;
}

std::string namespace_resolver::gen_namespace(const t_program* program) {
  auto const components = gen_namespace_components(program);
  return "::" + boost::algorithm::join(components, "::");
}

std::string namespace_resolver::gen_namespaced_name(const t_type* node) {
  const std::string& name = node->get_annotation("cpp.name", &node->get_name());
  if (node->program() == nullptr) {
    // No namespace.
    return name;
  }
  return get_namespace(node->program()) + "::" + name;
}

} // namespace cpp
} // namespace gen
} // namespace compiler
} // namespace thrift
} // namespace apache
