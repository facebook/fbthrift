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

/**
 * thrift - a lightweight cross-language rpc/serialization tool
 *
 * This file contains the main compiler engine for Thrift, which invokes the
 * scanner/parser to build the thrift object tree. The interface generation
 * code for each language lives in a file by the language name under the
 * generate/ folder, and all parse structures live in parse/
 *
 */

#include <thrift/compiler/compiler.h>

#ifdef _WIN32
#include <process.h> // @manual
#else
#include <unistd.h>
#endif
#include <ctime>

#include <boost/algorithm/string/join.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/filesystem.hpp>

#include <thrift/compiler/ast/diagnostic.h>
#include <thrift/compiler/ast/diagnostic_context.h>
#include <thrift/compiler/common.h>
#include <thrift/compiler/generate/t_generator.h>
#include <thrift/compiler/mutator/mutator.h>
#include <thrift/compiler/parse/parsing_driver.h>
#include <thrift/compiler/platform.h>
#include <thrift/compiler/sema/standard_mutator.h>
#include <thrift/compiler/sema/standard_validator.h>
#include <thrift/compiler/validator/validator.h>

namespace apache {
namespace thrift {
namespace compiler {
namespace {

/**
 * Flags to control code generation
 */
struct gen_params {
  bool gen_recurse = false;
  std::string genfile;
  std::vector<std::string> targets;
  t_generation_context context;
};

/**
 * Display the usage message.
 */
void usage() {
  fprintf(stderr, "Usage: thrift [options] file\n");
  fprintf(stderr, "Options:\n");
  fprintf(
      stderr,
      "  -o dir      Set the output directory for gen-* packages\n"
      "              (default: current directory)\n");
  fprintf(
      stderr,
      "  -out dir    Set the output location for generated files\n"
      "              (no gen-* folder will be created)\n");
  fprintf(
      stderr,
      "  -I dir      Add a directory to the list of directories\n"
      "              searched for include directives\n");
  fprintf(stderr, "  -nowarn     Suppress all compiler warnings (BAD!)\n");
  fprintf(stderr, "  -strict     Strict compiler warnings on\n");
  fprintf(stderr, "  -v[erbose]  Verbose mode\n");
  fprintf(stderr, "  -r[ecurse]  Also generate included files\n");
  fprintf(stderr, "  -debug      Parse debug trace to stdout\n");
  fprintf(
      stderr,
      "  --allow-neg-keys  Allow negative field keys (Used to preserve protocol\n"
      "                    compatibility with older .thrift files)\n");
  fprintf(stderr, "  --allow-neg-enum-vals Allow negative enum vals\n");
  fprintf(
      stderr,
      "  --allow-64bit-consts  Do not print warnings about using 64-bit constants\n");
  fprintf(
      stderr,
      "  --allow-experimental-features feature[,feature]\n"
      "              Enable experimental features. Use 'all' to enable all experimental features.\n");
  fprintf(
      stderr,
      "  --gen STR   Generate code with a dynamically-registered generator.\n"
      "              STR has the form language[:key1=val1[,key2,[key3=val3]]].\n"
      "              Keys and values are options passed to the generator.\n"
      "              Many options will not require values.\n");
  fprintf(
      stderr,
      "  --record-genfiles FILE\n"
      "              Save the list of generated files to FILE,\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Available generators (and options):\n");

  t_generator_registry::gen_map_t gen_map =
      t_generator_registry::get_generator_map();
  t_generator_registry::gen_map_t::iterator iter;
  for (iter = gen_map.begin(); iter != gen_map.end(); ++iter) {
    fprintf(
        stderr,
        "  %s (%s):\n",
        iter->second->get_short_name().c_str(),
        iter->second->get_long_name().c_str());
    fprintf(stderr, "%s", iter->second->get_documentation().c_str());
  }
}

bool isComma(const char& c) {
  return c == ',';
}

// Returns the input file name if successful, otherwise returns an empty
// string.
std::string parseArgs(
    const std::vector<std::string>& arguments,
    parsing_params& pparams,
    gen_params& gparams,
    diagnostic_params& dparams) {
  // Check for necessary arguments, you gotta have at least a filename and
  // an output language flag.
  if (arguments.size() < 2) {
    usage();
    return {};
  }

  // A helper that grabs the next argument, if possible.
  // Outputs an error and returns nullptr if not.
  size_t arg_i = 1; // Skip the binary name.
  auto consume_arg = [&](const char* arg_name) -> const std::string* {
    // Note: The input filename must be the last argument.
    if (arg_i + 2 >= arguments.size()) {
      fprintf(
          stderr,
          "!!! Missing %s between %s and '%s'\n",
          arg_name,
          arguments[arg_i].c_str(),
          arguments[arg_i + 1].c_str());
      usage();
      return nullptr;
    }
    return &arguments[++arg_i];
  };

  // Hacky parameter handling... I didn't feel like using a library sorry!
  bool nowarn = false; // Guard so --nowarn and --strict are order agnostic.
  for (; arg_i < arguments.size() - 1;
       ++arg_i) { // Last argument is the src file.
    // Parse flag.
    std::string flag;
    if (arguments[arg_i].size() < 2 || arguments[arg_i][0] != '-') {
      fprintf(stderr, "!!! Expected flag, got: %s\n", arguments[arg_i].c_str());
      usage();
      return {};
    } else if (arguments[arg_i][1] == '-') {
      flag = arguments[arg_i].substr(2);
    } else {
      flag = arguments[arg_i].substr(1);
    }

    // Interpret flag.
    if (flag == "allow-experimental-features") {
      auto* arg = consume_arg("feature");
      if (arg == nullptr) {
        return {};
      }
      boost::algorithm::split(
          pparams.allow_experimental_features, *arg, isComma);
    } else if (flag == "debug") {
      dparams.debug = true;
      g_debug = 1;
    } else if (flag == "nowarn") {
      dparams.warn_level = g_warn = 0;
      nowarn = true;
    } else if (flag == "strict") {
      pparams.strict = 255;
      if (!nowarn) { // Don't override nowarn.
        dparams.warn_level = g_warn = 2;
      }
    } else if (flag == "v" || flag == "verbose") {
      dparams.info = true;
      g_verbose = 1;
    } else if (flag == "r" || flag == "recurse") {
      gparams.gen_recurse = true;
    } else if (flag == "allow-neg-keys") {
      pparams.allow_neg_field_keys = true;
    } else if (flag == "allow-neg-enum-vals") {
      dparams.allow_neg_enum_vals = true;
    } else if (flag == "allow-64bit-consts") {
      pparams.allow_64bit_consts = true;
    } else if (flag == "record-genfiles") {
      auto* arg = consume_arg("genfile file specification");
      if (arg == nullptr) {
        return {};
      }
      gparams.genfile = *arg;
    } else if (flag == "gen") {
      auto* arg = consume_arg("generator specification");
      if (arg == nullptr) {
        return {};
      }
      gparams.targets.push_back(*arg);
    } else if (flag == "cpp_use_include_prefix") {
      g_cpp_use_include_prefix = true;
    } else if (flag == "I") {
      auto* arg = consume_arg("include directory");
      if (arg == nullptr) {
        return {};
      }
      // An argument of "-I\ asdf" is invalid and has unknown results
      pparams.incl_searchpath.push_back(*arg);
    } else if (flag == "o" || flag == "out") {
      auto* arg = consume_arg("output directory");
      if (arg == nullptr) {
        return {};
      }
      std::string out_path = *arg;
      bool out_path_is_absolute = (flag == "out");

      // Strip out trailing \ on a Windows path
      if (platform_is_windows()) {
        int last = out_path.length() - 1;
        if (out_path[last] == '\\') {
          out_path.erase(last);
        }
      }

      if (out_path_is_absolute) {
        // Invoker specified `-out blah`. We are supposed to output directly
        // into blah, e.g. `blah/Foo.java`. Make the directory if necessary,
        // just like how for `-o blah` we make `o/gen-java`
        boost::system::error_code errc;
        boost::filesystem::create_directory(out_path, errc);
        if (errc) {
          fprintf(
              stderr,
              "Output path %s is unusable or not a directory\n",
              out_path.c_str());
          return {};
        }
      }
      if (!boost::filesystem::is_directory(out_path)) {
        fprintf(
            stderr,
            "Output path %s is unusable or not a directory\n",
            out_path.c_str());
        return {};
      }
      gparams.context = {std::move(out_path), out_path_is_absolute};
    } else {
      fprintf(
          stderr, "!!! Unrecognized option: %s\n", arguments[arg_i].c_str());
      usage();
      return {};
    }
  }

  // You gotta generate something!
  if (gparams.targets.empty()) {
    fprintf(stderr, "!!! No output language(s) specified\n\n");
    usage();
    return {};
  }

  // Return the input file name.
  assert(arg_i == arguments.size() - 1);
  return arguments[arg_i];
}

/**
 * Generate code
 */
bool generate(
    const gen_params& params,
    t_program* program,
    std::set<std::string>& already_generated) {
  // Oooohh, recursive code generation, hot!!
  if (params.gen_recurse) {
    // Add the path we are about to generate.
    already_generated.emplace(program->path());
    for (const auto& include : program->get_included_programs()) {
      if (!already_generated.count(include->path()) &&
          !generate(params, include, already_generated)) {
        return false;
      }
    }
  }

  // Generate code!
  try {
    pverbose("Program: %s\n", program->path().c_str());

    if (dump_docs) {
      dump_docstrings(program);
    }

    bool success = true;
    std::ofstream genfile;
    if (!params.genfile.empty()) {
      genfile.open(params.genfile);
    }
    for (auto target : params.targets) {
      auto pos = target.find(':');
      std::string lang = target.substr(0, pos);
      auto generator = std::unique_ptr<t_generator>{
          t_generator_registry::get_generator(program, params.context, target)};
      if (generator == nullptr) {
        continue;
      }

      validator::diagnostics_t diagnostics;
      validator_list validators(diagnostics);
      generator->fill_validator_list(validators);
      validators.traverse(program);

      bool has_failure = false;
      for (const auto& d : diagnostics) {
        has_failure = has_failure || (d.level() == diagnostic_level::failure);
        std::cerr << d << std::endl;
      }
      if (has_failure) {
        success = false;
        continue;
      }

      pverbose("Generating \"%s\"\n", target.c_str());
      generator->generate_program();
      if (genfile.is_open()) {
        for (const std::string& s : generator->get_genfiles()) {
          genfile << s << "\n";
        }
      }
    }
    return success;
  } catch (const std::string& s) {
    printf("Error: %s\n", s.c_str());
    return false;
  } catch (const char* exc) {
    printf("Error: %s\n", exc);
    return false;
  }
}

bool generate(const gen_params& params, t_program* program) {
  std::set<std::string> already_generated;
  return generate(params, program, already_generated);
}

std::string get_include_path(
    const std::vector<std::string>& generator_targets,
    const std::string& input_filename) {
  std::string include_prefix;
  for (const auto& target : generator_targets) {
    auto const colon_pos = target.find(':');
    if (colon_pos == std::string::npos) {
      continue;
    }
    auto const lang_name = target.substr(0, colon_pos);
    if (lang_name != "cpp2" && lang_name != "mstch_cpp2") {
      continue;
    }

    auto const lang_args = target.substr(colon_pos + 1);
    parse_generator_options(lang_args, [&](std::string k, std::string v) {
      if (k.find("include_prefix") != std::string::npos) {
        include_prefix = std::move(v);
        return CallbackLoopControl::Break;
      }
      return CallbackLoopControl::Continue;
    });
  }

  // infer cpp include prefix from the filename passed in if none specified.
  if (include_prefix == "") {
    if (input_filename.rfind('/') != std::string::npos) {
      include_prefix = input_filename.substr(0, input_filename.rfind('/'));
    }
  }

  return include_prefix;
}

} // namespace

// TODO(urielrivas): Reuse somehow this function in compile(...).
std::unique_ptr<t_program_bundle> parse_and_get_program(
    const std::vector<std::string>& arguments) {
  // Parse arguments.
  parsing_params pparams{};
  gen_params gparams{};
  diagnostic_params dparams{};
  std::string input_filename = parseArgs(arguments, pparams, gparams, dparams);

  if (input_filename.empty()) {
    return {};
  }

  // Parse and mutate it!
  auto ctx = diagnostic_context::ignore_all();
  parsing_driver driver{ctx, input_filename, std::move(pparams)};
  auto program = driver.parse();
  if (program) {
    mutator::mutate(program->root_program());
  }

  return program;
}

compile_result compile(const std::vector<std::string>& arguments) {
  compile_result result;

  // Parse arguments.
  g_stage = "arguments";
  parsing_params pparams{};
  gen_params gparams{};
  diagnostic_params dparams{};
  std::string input_filename = parseArgs(arguments, pparams, gparams, dparams);
  if (input_filename.empty()) {
    return result;
  }
  diagnostic_context ctx{result.detail, std::move(dparams)};

  // Parse it!
  g_stage = "parse";
  parsing_driver driver{ctx, input_filename, std::move(pparams)};
  auto program = driver.parse();
  if (!program) {
    return result;
  }

  // Mutate it!
  try {
    mutator::mutate(program->root_program());
  } catch (MutatorException& e) {
    ctx.report(std::move(e.message));
    return result;
  }
  standard_mutator().mutate(ctx, *program->root_program());
  if (result.detail.has_failure()) {
    return result;
  }

  program->root_program()->set_include_prefix(
      get_include_path(gparams.targets, input_filename));

  // Validate it!
  ctx.report_all(validator::validate(program->root_program()));
  standard_validator()(ctx, *program->root_program());
  if (result.detail.has_failure()) {
    return result;
  }

  // Generate it!
  g_stage = "generation";
  try {
    if (generate(gparams, program->root_program())) {
      result.retcode = compile_retcode::success;
    }
  } catch (const std::exception& e) {
    ctx.failure(*program->root_program(), e.what());
  }
  return result;
}

} // namespace compiler
} // namespace thrift
} // namespace apache
