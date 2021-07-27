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

#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include <thrift/compiler/ast/t_program.h>
#include <thrift/compiler/ast/t_type.h>
#include <thrift/compiler/gen/cpp/detail/gen.h>

namespace apache {
namespace thrift {
namespace compiler {
namespace gen {
namespace cpp {

// A class that resolves c++ namespace names for thrift programs and caches the
// results.
class namespace_resolver {
 public:
  namespace_resolver() = default;

  // Returns c++ namespace for the given program.
  const std::string& get_namespace(const t_program* node) {
    return detail::get_or_gen(
        namespace_cache_, node, [&]() { return gen_namespace(node); });
  }

  std::string gen_namespaced_name(const t_type* node);

  static std::string gen_namespace(const t_program* node);
  static std::vector<std::string> gen_namespace_components(
      const t_program* program);

 private:
  std::unordered_map<const t_program*, std::string> namespace_cache_;
};

} // namespace cpp
} // namespace gen
} // namespace compiler
} // namespace thrift
} // namespace apache
