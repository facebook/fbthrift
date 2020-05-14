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
#include <vector>

namespace apache {
namespace thrift {
namespace compiler {

struct diagnostic_message;

enum class compile_retcode {
  SUCCESS = 0,
  FAILURE = 1,
};

struct compile_result {
  compile_retcode retcode;
  std::vector<diagnostic_message> diagnostics;
};

/**
 * Parse it up, then spit it back out, in pretty much every language. Alright
 * not that many languages, but the cool ones that we care about.
 */
compile_result compile(std::vector<std::string> arguments);
} // namespace compiler
} // namespace thrift
} // namespace apache
