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

#include <memory>
#include <string>
#include <vector>

#include <thrift/compiler/ast/diagnostic.h>
#include <thrift/compiler/ast/t_program_bundle.h>

namespace apache {
namespace thrift {
namespace compiler {

enum class compile_retcode {
  success = 0,
  failure = 1,
};

struct compile_result {
  compile_retcode retcode = compile_retcode::failure;
  diagnostic_results detail;
};

/**
 * Runs the Thrift parser with the specified (command-line) arguments and
 * returns the program bundle.
 */
std::unique_ptr<t_program_bundle> parse_and_get_program(
    const std::vector<std::string>& arguments);

/**
 * Runs the Thrift compiler with the specified (command-line) arguments.
 */
compile_result compile(const std::vector<std::string>& arguments);

} // namespace compiler
} // namespace thrift
} // namespace apache
