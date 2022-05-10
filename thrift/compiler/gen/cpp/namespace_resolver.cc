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

#include <thrift/compiler/gen/cpp/namespace_resolver.h>

#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/split.hpp>

namespace apache {
namespace thrift {
namespace compiler {
namespace gen {
namespace cpp {

std::vector<std::string>
namespace_resolver::gen_namespace_components_from_package(
    const t_package& package) {
  if (package.empty()) {
    return {};
  }
  std::vector<std::string> ret = {package.domain()[0]};
  auto&& path = package.path();
  ret.insert(ret.end(), path.begin(), path.end());
  return ret;
}

std::vector<std::string> namespace_resolver::gen_namespace_components(
    const t_program& program) {
  auto components = program.gen_namespace_or_default("cpp2", [&program] {
    return gen_namespace_components_from_package(program.package());
  });
  if (components.empty()) {
    components = program.gen_namespace_or_default(
        "cpp", [] { return std::vector<std::string>{}; });
    components.push_back("cpp2");
  }
  return components;
}

std::string namespace_resolver::gen_namespace(const t_program& program) {
  return "::" + gen_unprefixed_namespace(program);
}

std::string namespace_resolver::gen_unprefixed_namespace(
    const t_program& program) {
  auto const components = gen_namespace_components(program);
  return boost::algorithm::join(components, "::");
}

} // namespace cpp
} // namespace gen
} // namespace compiler
} // namespace thrift
} // namespace apache
