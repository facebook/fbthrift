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

// Careful: must include globals first for extern definitions
#include <thrift/compiler/globals.h>

#include <thrift/compiler/parse/t_program.h>
#include <thrift/compiler/parse/t_scope.h>

// TODO: Move to parse/ when thrift-deglobalize is complete.
namespace apache {
namespace thrift {

struct parsing_params {
  // Default values are taken from the original global variables.

  /**
   * The master program parse tree. This is accessed from within the parser code
   * to build up the program elements.
   */
  t_program* program;

  /**
   * Global scope cache for faster compilations
   */
  t_scope* scope_cache;

  bool debug = false;
  bool verbose = false;
  int warn = 1;

  /**
   * Strictness level
   */
  int strict = 127;

  /**
   * Whether or not negative field keys are accepted.
   *
   * When a field does not have a user-specified key, thrift automatically
   * assigns a negative value.  However, this is fragile since changes to the
   * file may unintentionally change the key numbering, resulting in a new
   * protocol that is not backwards compatible.
   *
   * When allow_neg_field_keys is enabled, users can explicitly specify
   * negative keys.  This way they can write a .thrift file with explicitly
   * specified keys that is still backwards compatible with older .thrift files
   * that did not specify key values.
   */
  bool allow_neg_field_keys = false;

  /**
   * Whether or not negative enum values.
   */
  bool allow_neg_enum_vals = false;

  /**
   * Whether or not 64-bit constants will generate a warning.
   *
   * Some languages don't support 64-bit constants, but many do, so we can
   * suppress this warning for projects that don't use any non-64-bit-safe
   * languages.
   */
  bool allow_64bit_consts = false;

  /**
   * Search path for inclusions
   */
  std::vector<std::string> incl_searchpath;
};

} // namespace thrift
} // namespace apache

namespace {
using namespace std;
}

/**
 * Current file being parsed
 */
extern string g_curpath;

/**
 * Directory containing template files
 */
extern string g_template_dir;

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
 * Placeholder struct to return key and value of an annotation during parsing.
 */
struct t_annotation {
  std::string key;
  std::string val;
};

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

const t_type* get_true_type(const t_type* type);

/**
 * Check members of a throws block
 */
bool validate_throws(t_struct* throws);

/**
 * Parses a program. already_parsed_paths is deliberately passed by value
 * because it should be the set of files in the direct inclusion tree.
 */
void parse(
    apache::thrift::parsing_params params,
    std::set<std::string>& already_parsed_paths,
    std::set<std::string> circular_deps = std::set<std::string>());

#endif
