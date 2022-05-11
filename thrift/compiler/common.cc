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

#include <thrift/compiler/common.h>

#include <boost/filesystem.hpp>

namespace apache {
namespace thrift {
namespace compiler {

/**
 * Current compilation stage. One of: arguments, parse, generation
 */
std::string g_stage;

/**
 * Global debug state
 */
int g_debug = 0;

/**
 * Warning level
 */
int g_warn = 1;

/**
 * Verbose output
 */
int g_verbose = 0;

/**
 * The last parsed doctext comment.
 */
char* g_doctext;

/**
 * The location of the last parsed doctext comment.
 */
int g_doctext_lineno;

void mark_file_executable(std::string const& path) {
  namespace fs = boost::filesystem;
  fs::permissions(
      path, fs::add_perms | fs::owner_exe | fs::group_exe | fs::others_exe);
}

} // namespace compiler
} // namespace thrift
} // namespace apache
