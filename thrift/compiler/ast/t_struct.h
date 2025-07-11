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

#include <string>
#include <utility>

#include <thrift/compiler/ast/t_structured.h>
#include <thrift/compiler/ast/t_type.h>

namespace apache::thrift::compiler {

// Forward declare that puppy
class t_program;

/**
 * Represents a struct definition.
 */
class t_struct final : public t_structured {
 public:
  t_struct(const t_program* program, std::string name)
      : t_structured(program, std::move(name)) {}
};

} // namespace apache::thrift::compiler
