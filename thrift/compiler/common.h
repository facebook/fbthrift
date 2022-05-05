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
#include <utility>

#include <thrift/compiler/ast/t_program.h>
#include <thrift/compiler/ast/t_program_bundle.h>
#include <thrift/compiler/diagnostic.h>
#include <thrift/compiler/parse/parsing_driver.h>

namespace apache {
namespace thrift {
namespace compiler {

/**
 * Current compilation stage. One of: arguments, parse, generation
 */
extern std::string g_stage;

/**
 * Global debug state
 */
extern int g_debug;

/**
 * Warning level
 */
extern int g_warn;

/**
 * Verbose output
 */
extern int g_verbose;

/** Set to true to debug docstring parsing */
static bool dump_docs = false;

/**
 * Dumps docstrings to stdout
 * Only works for top-level definitions and the whole program doc
 * (i.e., not enum constants, struct fields, or functions.
 */
void dump_docstrings(t_program* program);

/**
 * Parse with the given parameters, and dump all the diagnostic messages
 * returned.
 *
 * If the parsing fails, nullptr is returned.
 */
std::unique_ptr<t_program_bundle> parse_and_dump_diagnostics(
    std::string path, parsing_params pparams, diagnostic_params dparams = {});

/**
 * Parse and mutate with the given parameters
 *
 * If the parsing fails, nullptr is returned for the program bundle.
 */
std::pair<std::unique_ptr<t_program_bundle>, diagnostic_results>
parse_and_mutate_program(
    source_manager& sm,
    const std::string& filename,
    parsing_params params,
    diagnostic_params dparams = {});
std::unique_ptr<t_program_bundle> parse_and_mutate_program(
    source_manager& sm,
    diagnostic_context& ctx,
    const std::string& filename,
    parsing_params params);

void mark_file_executable(std::string const& path);

} // namespace compiler
} // namespace thrift
} // namespace apache
