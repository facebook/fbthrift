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

#include <map>
#include <memory>
#include <string>
#include <vector>

#include <thrift/compiler/ast/t_annotated.h>

namespace apache {
namespace thrift {
namespace compiler {

/**
 * class t_named
 *
 * Base class for any named AST node.
 * Anything that is named, can be annotated.
 */
class t_named : public t_annotated {
 public:
  /**
   * t_type setters
   */
  void set_name(const std::string& name) {
    name_ = name;
  }
  const std::string& get_name() const {
    return name_;
  }

 protected:
  // t_named is abstract.
  t_named() = default;

  explicit t_named(std::string name) : name_(std::move(name)) {}

  // TODO(afuller): make private.
  std::string name_;
};

} // namespace compiler
} // namespace thrift
} // namespace apache
