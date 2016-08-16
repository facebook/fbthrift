/*
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

#include <cassert>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string>
#include <algorithm>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <limits.h>

// Careful: must include globals first for extern definitions
#include <thrift/compiler/globals.h>

#include <thrift/compiler/parse/t_program.h>
#include <thrift/compiler/parse/t_scope.h>
#ifdef MINGW
# include <windows.h> /* for GetFullPathName */
#endif

namespace {
using namespace std;
}

/**
 * Current directory of file being parsed
 */
extern string g_curdir;

/**
 * Current file being parsed
 */
extern string g_curpath;

/**
 * Directory containing template files
 */
extern string g_template_dir;

/**
 * Search path for inclusions
 */
extern vector<string> g_incl_searchpath;

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
 * MinGW doesn't have realpath, so use fallback implementation in that case,
 * otherwise this just calls through to realpath
 */
char *saferealpath(const char *path, char *resolved_path);

/**
 * Report an error to the user. This is called yyerror for historical
 * reasons (lex and yacc expect the error reporting routine to be called
 * this). Call this function to report any errors to the user.
 * yyerror takes printf style arguments.
 *
 * @param fmt C format string followed by additional arguments
 */
void yyerror(const char* fmt, ...);

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
[[noreturn]]
void failure(const char* fmt, ...);

/**
 * Converts a string filename into a thrift program name
 */
string program_name(string filename);

/**
 * Gets the directory path of a filename
 */
string directory_name(string filename);

/**
 * Finds the appropriate file path for the given filename
 */
string include_file(string filename);

/**
 * Clears any previously stored doctext string.
 * Also prints a warning if we are discarding information.
 */
void clear_doctext();

/**
 * Cleans up text commonly found in doxygen-like comments
 *
 * Warning: if you mix tabs and spaces in a non-uniform way,
 * you will get what you deserve.
 */
char* clean_up_doctext(char* doctext);

/** Set to true to debug docstring parsing */
static bool dump_docs = false;

/**
 * Dumps docstrings to stdout
 * Only works for top-level definitions and the whole program doc
 * (i.e., not enum constants, struct fields, or functions.
 */
void dump_docstrings(t_program* program);

/**
 * You know, when I started working on Thrift I really thought it wasn't going
 * to become a programming language because it was just a generator and it
 * wouldn't need runtime type information and all that jazz. But then we
 * decided to add constants, and all of a sudden that means runtime type
 * validation and inference, except the "runtime" is the code generator
 * runtime. Shit. I've been had.
 */
void validate_const_rec(std::string name, t_type* type, t_const_value* value);

/**
 * Check the type of the parsed const information against its declared type
 */
void validate_const_type(t_const* c);

/**
 * Check the type of a default value assigned to a field.
 */
void validate_field_value(t_field* field, t_const_value* cv);

/**
 * Parses a program. already_parsed_paths is deliberately passed by value
 * because it should be the set of files in the direct inclusion tree.
 */
void parse(t_program* program,
           t_program* parent_program,
           set<string> already_parsed_paths = set<string>());

#endif
