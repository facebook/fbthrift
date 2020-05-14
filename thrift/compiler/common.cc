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
#include <thrift/compiler/parse/parsing_driver.h>

#include <cstdarg>

#include <boost/filesystem.hpp>

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
  const std::vector<t_typedef*>& typedefs = program->get_typedefs();
  std::vector<t_typedef*>::const_iterator t_iter;
  for (t_iter = typedefs.begin(); t_iter != typedefs.end(); ++t_iter) {
    t_typedef* td = *t_iter;
    if (td->has_doc()) {
      printf(
          "typedef %s:\n%s\n", td->get_name().c_str(), td->get_doc().c_str());
    }
  }
  const std::vector<t_enum*>& enums = program->get_enums();
  std::vector<t_enum*>::const_iterator e_iter;
  for (e_iter = enums.begin(); e_iter != enums.end(); ++e_iter) {
    t_enum* en = *e_iter;
    if (en->has_doc()) {
      printf("enum %s:\n%s\n", en->get_name().c_str(), en->get_doc().c_str());
    }
  }
  const std::vector<t_const*>& consts = program->get_consts();
  std::vector<t_const*>::const_iterator c_iter;
  for (c_iter = consts.begin(); c_iter != consts.end(); ++c_iter) {
    t_const* co = *c_iter;
    if (co->has_doc()) {
      printf("const %s:\n%s\n", co->get_name().c_str(), co->get_doc().c_str());
    }
  }
  const std::vector<t_struct*>& structs = program->get_structs();
  std::vector<t_struct*>::const_iterator s_iter;
  for (s_iter = structs.begin(); s_iter != structs.end(); ++s_iter) {
    t_struct* st = *s_iter;
    if (st->has_doc()) {
      printf("struct %s:\n%s\n", st->get_name().c_str(), st->get_doc().c_str());
    }
  }
  const std::vector<t_struct*>& xceptions = program->get_xceptions();
  std::vector<t_struct*>::const_iterator x_iter;
  for (x_iter = xceptions.begin(); x_iter != xceptions.end(); ++x_iter) {
    t_struct* xn = *x_iter;
    if (xn->has_doc()) {
      printf(
          "xception %s:\n%s\n", xn->get_name().c_str(), xn->get_doc().c_str());
    }
  }
  const std::vector<t_service*>& services = program->get_services();
  std::vector<t_service*>::const_iterator v_iter;
  for (v_iter = services.begin(); v_iter != services.end(); ++v_iter) {
    t_service* sv = *v_iter;
    if (sv->has_doc()) {
      printf(
          "service %s:\n%s\n", sv->get_name().c_str(), sv->get_doc().c_str());
    }
  }
}

std::unique_ptr<t_program_bundle> parse_and_dump_diagnostics(
    std::string path,
    parsing_params params) {
  parsing_driver driver{path, std::move(params)};

  std::vector<diagnostic_message> diagnostic_messages;
  auto program = driver.parse(diagnostic_messages);
  dump_diagnostics(diagnostic_messages);

  return program;
}

void dump_diagnostics(
    const std::vector<diagnostic_message>& diagnostic_messages) {
  for (auto const& message : diagnostic_messages) {
    switch (message.level) {
      case diagnostic_level::YY_ERROR:
        fprintf(
            stderr,
            "[ERROR:%s:%d] (last token was '%s')\n%s\n",
            message.filename.c_str(),
            message.lineno,
            message.last_token.c_str(),
            message.message.c_str());
        break;
      case diagnostic_level::WARNING:
        fprintf(
            stderr,
            "[WARNING:%s:%d] %s\n",
            message.filename.c_str(),
            message.lineno,
            message.message.c_str());
        break;
      case diagnostic_level::VERBOSE:
        fprintf(stderr, "%s", message.message.c_str());
        break;
      case diagnostic_level::DBG:
        fprintf(
            stderr, "[PARSE:%d] %s\n", message.lineno, message.message.c_str());
        break;
      case diagnostic_level::FAILURE:
        fprintf(
            stderr,
            "[FAILURE:%s:%d] %s\n",
            message.filename.c_str(),
            message.lineno,
            message.message.c_str());
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
