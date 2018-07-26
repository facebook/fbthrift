/*
 * Copyright 2017-present Facebook, Inc.
 *
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
#ifndef THRIFT_COMPILER_COMMON_
#define THRIFT_COMPILER_COMMON_ 1

#include <sys/stat.h>
#include <sys/types.h>

#include <algorithm>
#include <cassert>
#include <cerrno>
#include <climits>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <set>
#include <string>

#include <thrift/compiler/parse/parsing_driver.h>

#include <thrift/compiler/ast/t_program.h>
#include <thrift/compiler/ast/t_scope.h>

namespace apache {
namespace thrift {
namespace compiler {

/**
 * Current compilation stage. One of: arguments, parse, generation
 */
extern std::string g_stage;

/**
 * Directory containing template files
 */
extern std::string g_template_dir;

/**
 * Should C++ include statements use path prefixes for other thrift-generated
 * header files
 */
extern bool g_cpp_use_include_prefix;

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

/**
 * Obtain the absolute path name given a path for any platform
 *
 * Remarks: This function only works with existing paths but can be
 *          expanded to work with paths that will exist
 */
std::string compute_absolute_path(const std::string& path);

/**
 * Prints a debug message from the parser.
 *
 * @param fmt C format string followed by additional arguments
 */
void pdebug(const char* fmt, ...);

/**
 * Prints a verbose output mode message
 *
 * @param fmt C format string followed by additional arguments
 */
void pverbose(const char* fmt, ...);

/**
 * Prints a warning message
 *
 * @param fmt C format string followed by additional arguments
 */
void pwarning(int level, const char* fmt, ...);

/**
 * Prints a failure message and exits
 *
 * @param fmt C format string followed by additional arguments
 */
[[noreturn]] void failure(const char* fmt, ...);

/** Set to true to debug docstring parsing */
static bool dump_docs = false;

/**
 * Dumps docstrings to stdout
 * Only works for top-level definitions and the whole program doc
 * (i.e., not enum constants, struct fields, or functions.
 */
void dump_docstrings(t_program* program);

/**
 * Check members of a throws block
 */
bool validate_throws(t_struct* throws);

/**
 * Parse with the given parameters, and dump all the diagnostic messages
 * returned.
 *
 * If the parsing fails, this function will exit(1).
 */
std::unique_ptr<t_program> parse_and_dump_diagnostics(
    std::string path,
    apache::thrift::parsing_params params);

} // namespace compiler
} // namespace thrift
} // namespace apache

#endif
