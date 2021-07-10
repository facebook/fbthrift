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

#include <thrift/compiler/common.h>

#include <cstdarg>

#include <boost/filesystem.hpp>

#include <thrift/compiler/ast/diagnostic_context.h>

namespace apache {
namespace thrift {
namespace compiler {

/**
 * Current compilation stage. One of: arguments, parse, generation
 */
std::string g_stage;

/**
 * Should C++ include statements use path prefixes for other thrift-generated
 * header files
 */
bool g_cpp_use_include_prefix = false;

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

std::string compute_absolute_path(const std::string& path) {
  boost::filesystem::path abspath{path};
  try {
    abspath = boost::filesystem::absolute(abspath);
    return abspath.string();
  } catch (const boost::filesystem::filesystem_error& e) {
    failure("Could not find file: %s. Error: %s", path.c_str(), e.what());
  }
}

void pdebug(const char* fmt, ...) {
  if (g_debug == 0) {
    return;
  }
  va_list args;
  printf("[PARSE] ");
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
  printf("\n");
}

void pverbose(const char* fmt, ...) {
  if (g_verbose == 0) {
    return;
  }
  va_list args;
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
}

void pwarning(int level, const char* fmt, ...) {
  if (g_warn < level) {
    return;
  }
  va_list args;
  fprintf(stderr, "[WARNING:%s] ", g_stage.c_str());
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  fprintf(stderr, "\n");
}

[[noreturn]] void failure(const char* fmt, ...) {
  va_list args;
  fprintf(stderr, "[FAILURE:%s] ", g_stage.c_str());
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  fprintf(stderr, "\n");
  exit(1);
}

void dump_docstrings(t_program* program) {
  std::string progdoc = program->get_doc();
  if (!progdoc.empty()) {
    printf("Whole program doc:\n%s\n", progdoc.c_str());
  }
  for (auto* td : program->typedefs()) {
    if (td->has_doc()) {
      printf(
          "typedef %s:\n%s\n", td->get_name().c_str(), td->get_doc().c_str());
    }
  }
  for (auto* en : program->enums()) {
    if (en->has_doc()) {
      printf("enum %s:\n%s\n", en->get_name().c_str(), en->get_doc().c_str());
    }
  }
  for (auto* co : program->consts()) {
    if (co->has_doc()) {
      printf("const %s:\n%s\n", co->get_name().c_str(), co->get_doc().c_str());
    }
  }
  for (auto* st : program->structs()) {
    if (st->has_doc()) {
      printf("struct %s:\n%s\n", st->get_name().c_str(), st->get_doc().c_str());
    }
  }
  for (auto* xn : program->xceptions()) {
    if (xn->has_doc()) {
      printf(
          "xception %s:\n%s\n", xn->get_name().c_str(), xn->get_doc().c_str());
    }
  }
  for (auto* sv : program->services()) {
    if (sv->has_doc()) {
      printf(
          "service %s:\n%s\n", sv->get_name().c_str(), sv->get_doc().c_str());
    }
  }
}

std::unique_ptr<t_program_bundle> parse_and_dump_diagnostics(
    std::string path, parsing_params pparams, diagnostic_params dparams) {
  diagnostic_results results;
  diagnostic_context ctx{results, std::move(dparams)};
  parsing_driver driver{ctx, std::move(path), std::move(pparams)};
  auto program = driver.parse();
  dump_diagnostics(results.diagnostics());
  return program;
}

void dump_diagnostics(const std::vector<diagnostic>& diagnostic_messages) {
  char lineno[16];
  for (auto const& message : diagnostic_messages) {
    if (message.lineno() > 0) {
      sprintf(lineno, ":%d", message.lineno());
    } else {
      *lineno = '\0';
    }
    switch (message.level()) {
      case diagnostic_level::parse_error:
        fprintf(
            stderr,
            "[ERROR:%s%s] (last token was '%s')\n%s\n",
            message.file().c_str(),
            lineno,
            message.token().c_str(),
            message.message().c_str());
        break;
      case diagnostic_level::warning:
        fprintf(
            stderr,
            "[WARNING:%s%s] %s\n",
            message.file().c_str(),
            lineno,
            message.message().c_str());
        break;
      case diagnostic_level::info:
        fprintf(stderr, "%s", message.message().c_str());
        break;
      case diagnostic_level::debug:
        fprintf(stderr, "[PARSE%s] %s\n", lineno, message.message().c_str());
        break;
      case diagnostic_level::failure:
        fprintf(
            stderr,
            "[FAILURE:%s%s] %s\n",
            message.file().c_str(),
            lineno,
            message.message().c_str());
        break;
    }
  }
}

void mark_file_executable(std::string const& path) {
  namespace fs = boost::filesystem;
  fs::permissions(
      path, fs::add_perms | fs::owner_exe | fs::group_exe | fs::others_exe);
}

} // namespace compiler
} // namespace thrift
} // namespace apache
