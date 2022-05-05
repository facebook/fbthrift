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

#include <cstdarg>
#include <iostream>

#include <boost/filesystem.hpp>

#include <thrift/compiler/ast/diagnostic_context.h>
#include <thrift/compiler/sema/standard_mutator.h>

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

std::unique_ptr<t_program_bundle> parse_and_mutate_program(
    source_manager& sm,
    diagnostic_context& ctx,
    const std::string& filename,
    parsing_params params) {
  parsing_driver driver(sm, ctx, filename, std::move(params));
  auto program = driver.parse();
  if (program != nullptr) {
    standard_mutators()(ctx, *program);
  }
  return program;
}

std::pair<std::unique_ptr<t_program_bundle>, diagnostic_results>
parse_and_mutate_program(
    source_manager& sm,
    const std::string& filename,
    parsing_params params,
    diagnostic_params dparams) {
  diagnostic_results results;
  diagnostic_context ctx(sm, results, std::move(dparams));
  return {
      parse_and_mutate_program(sm, ctx, filename, std::move(params)), results};
}

std::unique_ptr<t_program_bundle> parse_and_dump_diagnostics(
    std::string path, parsing_params pparams, diagnostic_params dparams) {
  source_manager source_mgr;
  diagnostic_results results;
  diagnostic_context ctx(source_mgr, results, std::move(dparams));
  auto program =
      parse_and_mutate_program(source_mgr, ctx, path, std::move(pparams));
  for (const auto& diag : results.diagnostics()) {
    std::cerr << diag << "\n";
  }
  return program;
}

void mark_file_executable(std::string const& path) {
  namespace fs = boost::filesystem;
  fs::permissions(
      path, fs::add_perms | fs::owner_exe | fs::group_exe | fs::others_exe);
}

} // namespace compiler
} // namespace thrift
} // namespace apache
