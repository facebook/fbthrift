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

#include <memory>
#include <vector>

#include <thrift/compiler/ast/t_program.h>

namespace apache {
namespace thrift {
namespace compiler {

/**
 * Used to encapsulate a root t_program along with all included dependencies.
 */
class t_program_bundle {
 public:
  t_program_bundle(std::unique_ptr<t_program> root_program)
      : root_program_(root_program.get()) {
    add_program(std::move(root_program));
  }

  void add_program(std::unique_ptr<t_program> program) {
    programs_raw_.push_back(program.get());
    programs_.push_back(std::move(program));
  }

  t_program* get_root_program() const {
    return root_program_;
  }

  const std::vector<t_program*>& get_programs() const {
    return programs_raw_;
  }

 private:
  std::vector<std::unique_ptr<t_program>> programs_;

  std::vector<t_program*> programs_raw_;

  t_program* root_program_;
};

} // namespace compiler
} // namespace thrift
} // namespace apache
