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
/**
 * thrift - a lightweight cross-language rpc/serialization tool
 *
 * This file contains the main compiler engine for Thrift, which invokes the
 * scanner/parser to build the thrift object tree. The interface generation
 * code for each language lives in a file by the language name under the
 * generate/ folder, and all parse structures live in parse/
 *
 */

#ifndef _WIN32
#include <unistd.h>
#endif
#include <ctime>

#include <thrift/compiler/generate/t_generator.h>
#include <thrift/compiler/mutator.h>
#include <thrift/compiler/platform.h>
#include <thrift/compiler/validator.h>

/**
 * Flags to control code generation
 */
bool gen_cpp = false;
bool gen_dense = false;
bool gen_java = false;
bool gen_javabean = false;
bool gen_rb = false;
bool gen_py = false;
bool gen_py_newstyle = false;
bool gen_php = false;
bool gen_phpi = false;
bool gen_phps = true;
bool gen_phpa = false;
bool gen_phpo = false;
bool gen_rest = false;
bool gen_perl = false;
bool gen_erl = false;
bool gen_ocaml = false;
bool gen_hs = false;
bool gen_cocoa = false;
bool gen_csharp = false;
bool gen_st = false;
bool gen_recurse = false;

ofstream genfile_file;
bool record_genfiles = false;

/**
 * Diplays the usage message and then exits with an error code.
 */
[[noreturn]] static void usage() {
  fprintf(stderr, "Usage: thrift [options] file\n");
  fprintf(stderr, "Options:\n");
  fprintf(
      stderr, "  -o dir      Set the output directory for gen-* packages\n");
  fprintf(stderr, "               (default: current directory)\n");
  fprintf(
      stderr, "  -out dir    Set the output location for generated files\n");
  fprintf(
      stderr,
      "  --templates dir    Set the directory containing mstch templates\n");
  fprintf(stderr, "               (no gen-* folder will be created)\n");
  fprintf(stderr, "  -I dir      Add a directory to the list of directories\n");
  fprintf(stderr, "                searched for include directives\n");
  fprintf(stderr, "  -nowarn     Suppress all compiler warnings (BAD!)\n");
  fprintf(stderr, "  -strict     Strict compiler warnings on\n");
  fprintf(stderr, "  -v[erbose]  Verbose mode\n");
  fprintf(stderr, "  -r[ecurse]  Also generate included files\n");
  fprintf(stderr, "  -debug      Parse debug trace to stdout\n");
  fprintf(
      stderr,
      "  --allow-neg-keys  Allow negative field keys (Used to "
      "preserve protocol\n");
  fprintf(stderr, "                compatibility with older .thrift files)\n");
  fprintf(stderr, "  --allow-neg-enum-vals Allow negative enum vals\n");
  fprintf(
      stderr,
      "  --allow-64bit-consts  Do not print warnings about using 64-bit constants\n");
  fprintf(
      stderr,
      "  --gen STR   Generate code with a dynamically-registered generator.\n");
  fprintf(
      stderr,
      "                STR has the form language[:key1=val1[,key2,[key3=val3]]].\n");
  fprintf(
      stderr,
      "                Keys and values are options passed to the generator.\n");
  fprintf(stderr, "                Many options will not require values.\n");
  fprintf(stderr, "  --record-genfiles FILE\n");
  fprintf(stderr, "              Save the list of generated files to FILE\n");
  fprintf(stderr, "  --python-compiler FILE\n");
  fprintf(
      stderr,
      "              Path to the python implementation of the thrift compiler\n");
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
  exit(1);
}

static void preprocess_cpp2_generator_strings(
    std::string& gen_string,
    bool& gen_py_cpp2_reflection) {
  // Use python generator for reflection
  if (gen_string.find("reflection") != std::string::npos ||
      gen_string.find("fatal") != std::string::npos) {
    gen_py_cpp2_reflection = true;
  }
}

static bool python_generator(
    std::string& options,
    const std::string& user_python_compiler,
    std::vector<std::string>& arguments) {
  // Attempt to call the new python compiler if we can find it
  string path = arguments[0];
  size_t last = path.find_last_of("/");
  if (last != string::npos) {
    ifstream ifile;
    auto dirname = path.substr(0, last + 1);
    std::string pycompiler;
    std::vector<std::string> pycompilers;
    if (!user_python_compiler.empty()) {
      pycompilers.push_back(user_python_compiler);
    }
    pycompilers.insert(
        pycompilers.end(),
        {
            dirname + "py/thrift.lpar",
            dirname + "../py/thrift.lpar",
            dirname + "py/thrift.par",
            dirname + "../py/thrift.par",
            dirname + "py/thrift.xar",
            dirname + "../py/thrift.xar",
            dirname + "py/thrift.pex",
            dirname + "../py/thrift.pex",
        });
    for (const auto& comp : pycompilers) {
      pycompiler = comp;
      ifile.open(pycompiler.c_str());
      if (ifile)
        break;
    }
    int ret = 0;
    if (ifile) {
      // Convert arguments to argv
      std::vector<char*> argv(arguments.size() + 1);
      for (size_t i = 0; i < arguments.size(); ++i) {
        if (arguments[i][0] == '-' && arguments[i] != "-I" &&
            arguments[i] != "-o") {
          arguments[i].insert(0, "-");
        }
        argv[i] = const_cast<char*>(arguments[i].c_str());
      }
      argv[arguments.size()] = nullptr;
      ret = execv(pycompiler.c_str(), argv.data());
    }
    if (!ifile || ret < 0) {
      pwarning(
          1,
          "Unable to get a generator for \"%s\" ret: %d.\n",
          options.c_str(),
          ret);
    }
  }
  return true;
}

static void search_and_replace_args(
    std::vector<std::string>& arguments,
    std::string search,
    std::string replace) {
  for (size_t i = 0; i < arguments.size(); ++i) {
    if (arguments[i] == "-gen") {
      auto pos = arguments[i + 1].find(search);
      if (pos != std::string::npos) {
        arguments[i + 1] =
            arguments[i + 1].replace(pos, search.size(), replace);
      }
      break;
    }
  }
}

static bool generate_cpp2(
    t_program* program,
    std::string& gen_string,
    const std::string& user_python_compiler,
    std::vector<std::string>& arguments) {
  bool gen_py_cpp2_reflection = false;
  preprocess_cpp2_generator_strings(gen_string, gen_py_cpp2_reflection);

  t_generator* generator =
      t_generator_registry::get_generator(program, gen_string);
  if (generator) {
    pverbose("Generating \"%s\"\n", gen_string.c_str());
    generator->generate_program();
    if (record_genfiles) {
      for (const std::string& s : generator->get_genfiles()) {
        genfile_file << s << "\n";
      }
    }
    delete generator;
  }

  if (gen_py_cpp2_reflection) {
    search_and_replace_args(arguments, "mstch_cpp2", "cpp2");

    // reflection -> only_reflection
    search_and_replace_args(arguments, "reflection", "only_reflection");

    python_generator(gen_string, user_python_compiler, arguments);
  }
  return true;
}

/**
 * Generate code
 */
static bool generate(
    t_program* program,
    vector<string>& generator_strings,
    std::set<std::string>& already_generated,
    const std::string& user_python_compiler,
    std::vector<std::string>& arguments) {
  // Oooohh, recursive code generation, hot!!
  if (gen_recurse) {
    const vector<t_program*>& includes = program->get_includes();
    for (const auto& include : includes) {
      if (already_generated.count(include->get_path())) {
        continue;
      }

      // Propogate output path from parent to child programs
      include->set_out_path(
          program->get_out_path(), program->is_out_path_absolute());

      if (!generate(
              include,
              generator_strings,
              already_generated,
              user_python_compiler,
              arguments)) {
        return false;
      } else {
        already_generated.insert(include->get_path());
      }
    }
  }

  // Generate code!
  try {
    pverbose("Program: %s\n", program->get_path().c_str());

    if (dump_docs) {
      dump_docstrings(program);
    }

    for (auto gen_string : generator_strings) {
      auto pos = gen_string.find(":");
      std::string lang = gen_string.substr(0, pos);
      if (lang.find("cpp2") != std::string::npos) {
        generate_cpp2(program, gen_string, user_python_compiler, arguments);
        continue;
      }

      t_generator* generator =
          t_generator_registry::get_generator(program, gen_string);
      if (generator) {
        pverbose("Generating \"%s\"\n", gen_string.c_str());
        generator->generate_program();
        if (record_genfiles) {
          for (const std::string& s : generator->get_genfiles()) {
            genfile_file << s << "\n";
          }
        }
        delete generator;
      }
    }

  } catch (const string& s) {
    printf("Error: %s\n", s.c_str());
    return false;
  } catch (const char* exc) {
    printf("Error: %s\n", exc);
    return false;
  }

  return true;
}

/**
 * Parse it up.. then spit it back out, in pretty much every language. Alright
 * not that many languages, but the cool ones that we care about.
 */
int main(int argc, char** argv) {
  std::string out_path;
  bool out_path_is_absolute = false;
  std::vector<std::string> arguments(argv, argv + argc);

  // Setup time string
  time_t now = time(nullptr);
  g_time_str = ctime(&now);

  // Check for necessary arguments, you gotta have at least a filename and
  // an output language flag
  if (argc < 2) {
    usage();
  }

  std::string user_python_compiler;
  vector<std::string> generator_strings;

  // Set the current path to a dummy value to make warning messages clearer.
  g_curpath = "arguments";

  // Hacky parameter handling... I didn't feel like using a library sorry!
  size_t i;
  for (i = 1; i < arguments.size() - 1; ++i) {
    // Treat double dashes as single dashes
    if (arguments[i][0] == '-' && arguments[i][1] == '-') {
      arguments[i] = arguments[i].replace(0, 1, "");
    }

    if (arguments[i] == "-debug") {
      g_debug = 1;
    } else if (arguments[i] == "-nowarn") {
      g_warn = 0;
    } else if (arguments[i] == "-strict") {
      g_strict = 255;
      g_warn = 2;
    } else if (arguments[i] == "-v" || arguments[i] == "-verbose") {
      g_verbose = 1;
    } else if (arguments[i] == "-r" || arguments[i] == "-recurse") {
      gen_recurse = true;
    } else if (arguments[i] == "-allow-neg-keys") {
      g_allow_neg_field_keys = true;
    } else if (arguments[i] == "-allow-neg-enum-vals") {
      g_allow_neg_enum_vals = true;
    } else if (arguments[i] == "-allow-64bit-consts") {
      g_allow_64bit_consts = true;
    } else if (arguments[i] == "-record-genfiles") {
      record_genfiles = true;
      if (i + 1 == arguments.size() - 1) {
        fprintf(
            stderr,
            "!!! Missing genfile file specification between %s and '%s'\n",
            arguments[i].c_str(),
            arguments[i + 1].c_str());
        usage();
      }
      genfile_file.open(arguments[++i]);
    } else if (arguments[i] == "-gen") {
      if (i + 1 == arguments.size() - 1) {
        fprintf(
            stderr,
            "!!! Missing generator specification between %s and '%s'\n",
            arguments[i].c_str(),
            arguments[i + 1].c_str());
        usage();
      }
      generator_strings.push_back(arguments[++i]);
    } else if (arguments[i] == "-dense") {
      gen_dense = true;
    } else if (arguments[i] == "-cpp") {
      gen_cpp = true;
    } else if (arguments[i] == "-javabean") {
      gen_javabean = true;
    } else if (arguments[i] == "-java") {
      gen_java = true;
    } else if (arguments[i] == "-php") {
      gen_php = true;
    } else if (arguments[i] == "-phpi") {
      gen_phpi = true;
    } else if (arguments[i] == "-phps") {
      gen_php = true;
      gen_phps = true;
    } else if (arguments[i] == "-phpl") {
      gen_php = true;
      gen_phps = false;
    } else if (arguments[i] == "-phpa") {
      gen_php = true;
      gen_phps = false;
      gen_phpa = true;
    } else if (arguments[i] == "-phpo") {
      gen_php = true;
      gen_phpo = true;
    } else if (arguments[i] == "-rest") {
      gen_rest = true;
    } else if (arguments[i] == "-py") {
      gen_py = true;
    } else if (arguments[i] == "-pyns") {
      gen_py = true;
      gen_py_newstyle = true;
    } else if (arguments[i] == "-rb") {
      gen_rb = true;
    } else if (arguments[i] == "-perl") {
      gen_perl = true;
    } else if (arguments[i] == "-erl") {
      gen_erl = true;
    } else if (arguments[i] == "-ocaml") {
      gen_ocaml = true;
    } else if (arguments[i] == "-hs") {
      gen_hs = true;
    } else if (arguments[i] == "-cocoa") {
      gen_cocoa = true;
    } else if (arguments[i] == "-st") {
      gen_st = true;
    } else if (arguments[i] == "-csharp") {
      gen_csharp = true;
    } else if (arguments[i] == "-cpp_use_include_prefix") {
      g_cpp_use_include_prefix = true;
    } else if (arguments[i] == "-I") {
      if (i + 1 == arguments.size() - 1) {
        fprintf(
            stderr,
            "!!! Missing Include directory between %s and '%s'\n",
            arguments[i].c_str(),
            arguments[i + 1].c_str());
        usage();
      }
      // An argument of "-I\ asdf" is invalid and has unknown results
      g_incl_searchpath.push_back(arguments[++i]);
      continue;
    } else if (arguments[i] == "-templates") {
      if (i + 1 == arguments.size() - 1) {
        fprintf(stderr, "-templates: missing template directory");
        usage();
      }
      g_template_dir = arguments[++i];
      continue;
    } else if (arguments[i] == "-o" || (arguments[i] == "-out")) {
      out_path_is_absolute = (arguments[i] == "-out") ? true : false;
      if (i + 1 == arguments.size() - 1) {
        fprintf(
            stderr,
            "-o: missing output directory between %s and '%s'\n",
            arguments[i].c_str(),
            arguments[i + 1].c_str());
        usage();
      }
      out_path = arguments[++i];

      // Strip out trailing \ on a Windows path
      if (apache::thrift::compiler::isWindows()) {
        int last = out_path.length() - 1;
        if (out_path[last] == '\\') {
          out_path.erase(last);
        }
      }

      struct stat sb;
      if (out_path_is_absolute) {
        // Invoker specified `-out blah`. We are supposed to output directly
        // into blah, e.g. `blah/Foo.java`. Make the directory if necessary,
        // just like how for `-o blah` we make `o/gen-java`
        if (stat(out_path.c_str(), &sb) < 0 && errno == ENOENT &&
            make_dir(out_path.c_str()) < 0) {
          fprintf(
              stderr,
              "Output directory %s is unusable: mkdir: %s\n",
              out_path.c_str(),
              strerror(errno));
          return -1;
        }
      }
      if (stat(out_path.c_str(), &sb) < 0) {
        fprintf(
            stderr,
            "Output directory %s is unusable: %s\n",
            out_path.c_str(),
            strerror(errno));
        return -1;
      }
#ifndef _WIN32
      if (!S_ISDIR(sb.st_mode)) {
        fprintf(
            stderr,
            "Output directory %s exists but is not a directory\n",
            out_path.c_str());
        return -1;
      }
#endif
      continue;
    } else if (arguments[i] == "-python-compiler") {
      if (i + 1 == arguments.size() - 1) {
        fprintf(
            stderr,
            "No path was given for the python compiler between "
            "%s and '%s'\n",
            arguments[i].c_str(),
            arguments[i + 1].c_str());
        usage();
      }
      user_python_compiler = arguments[++i];
      continue;
    } else {
      fprintf(stderr, "!!! Unrecognized option: %s\n", arguments[i].c_str());
      usage();
    }
  }

  // TODO(dreiss): Delete these when everyone is using the new hotness.
  if (gen_cpp) {
    pwarning(1, "-cpp is deprecated.  Use --gen cpp");
    string gen_string = "cpp:";
    if (gen_dense) {
      gen_string.append("dense,");
    }
    if (g_cpp_use_include_prefix) {
      gen_string.append("include_prefix,");
    }
    generator_strings.push_back(gen_string);
  }
  if (gen_java) {
    pwarning(1, "-java is deprecated.  Use --gen java");
    generator_strings.push_back("java");
  }
  if (gen_javabean) {
    pwarning(1, "-javabean is deprecated.  Use --gen java:beans");
    generator_strings.push_back("java:beans");
  }
  if (gen_csharp) {
    pwarning(1, "-csharp is deprecated.  Use --gen csharp");
    generator_strings.push_back("csharp");
  }
  if (gen_py) {
    pwarning(1, "-py is deprecated.  Use --gen py");
    generator_strings.push_back("py");
  }
  if (gen_rb) {
    pwarning(1, "-rb is deprecated.  Use --gen rb");
    generator_strings.push_back("rb");
  }
  if (gen_perl) {
    pwarning(1, "-perl is deprecated.  Use --gen perl");
    generator_strings.push_back("perl");
  }
  if (gen_php || gen_phpi) {
    pwarning(1, "-php is deprecated.  Use --gen php");
    string gen_string = "php:";
    if (gen_phpi) {
      gen_string.append("inlined,");
    } else if (gen_phps) {
      gen_string.append("server,");
    } else if (gen_phpa) {
      gen_string.append("autoload,");
    } else if (gen_phpo) {
      gen_string.append("oop,");
    } else if (gen_rest) {
      gen_string.append("rest,");
    }
    generator_strings.push_back(gen_string);
  }
  if (gen_cocoa) {
    pwarning(1, "-cocoa is deprecated.  Use --gen cocoa");
    generator_strings.push_back("cocoa");
  }
  if (gen_erl) {
    pwarning(1, "-erl is deprecated.  Use --gen erl");
    generator_strings.push_back("erl");
  }
  if (gen_st) {
    pwarning(1, "-st is deprecated.  Use --gen st");
    generator_strings.push_back("st");
  }
  if (gen_ocaml) {
    pwarning(1, "-ocaml is deprecated.  Use --gen ocaml");
    generator_strings.push_back("ocaml");
  }
  if (gen_hs) {
    pwarning(1, "-hs is deprecated.  Use --gen hs");
    generator_strings.push_back("hs");
  }

  // You gotta generate something!
  if (generator_strings.empty()) {
    fprintf(stderr, "!!! No output language(s) specified\n\n");
    usage();
  }

  // Real-pathify it
  if (arguments.size() < i) {
    fprintf(stderr, "!!! Missing file name\n");
    usage();
  }

  std::string input_file = compute_absolute_path(arguments[i]);

  // Instance of the global parse tree
  t_program* program = new t_program(input_file);
  if (out_path.size()) {
    program->set_out_path(out_path, out_path_is_absolute);
  }

  // Compute the cpp include prefix.
  // infer this from the filename passed in
  string input_filename = arguments[i];
  string include_prefix;

  string::size_type last_slash = string::npos;
  if ((last_slash = input_filename.rfind("/")) != string::npos) {
    include_prefix = input_filename.substr(0, last_slash);
  }

  program->set_include_prefix(include_prefix);

  // Initialize global types
  g_type_void = new t_base_type("void", t_base_type::TYPE_VOID);
  g_type_string = new t_base_type("string", t_base_type::TYPE_STRING);
  g_type_binary = new t_base_type("string", t_base_type::TYPE_STRING);
  ((t_base_type*)g_type_binary)->set_binary(true);
  g_type_slist = new t_base_type("string", t_base_type::TYPE_STRING);
  ((t_base_type*)g_type_slist)->set_string_list(true);
  g_type_bool = new t_base_type("bool", t_base_type::TYPE_BOOL);
  g_type_byte = new t_base_type("byte", t_base_type::TYPE_BYTE);
  g_type_i16 = new t_base_type("i16", t_base_type::TYPE_I16);
  g_type_i32 = new t_base_type("i32", t_base_type::TYPE_I32);
  g_type_i64 = new t_base_type("i64", t_base_type::TYPE_I64);
  g_type_double = new t_base_type("double", t_base_type::TYPE_DOUBLE);
  g_type_float = new t_base_type("float", t_base_type::TYPE_FLOAT);

  // Parse it!
  g_scope_cache = program->scope();
  std::set<std::string> already_parsed_paths;
  parse(program, already_parsed_paths);

  // Mutate it!
  apache::thrift::compiler::mutator::mutate(program);

  // Validate it!
  auto errors = apache::thrift::compiler::validator::validate(program);
  if (!errors.empty()) {
    for (const auto& error : errors) {
      std::cerr << error << std::endl;
    }
    return 1;
  }

  // The current path is not really relevant when we are doing generation.
  // Reset the variable to make warning messages clearer.
  g_curpath = "generation";
  // Reset yylineno for the heck of it.  Use 1 instead of 0 because
  // That is what shows up during argument parsing.
  yylineno = 1;

  // Generate it!
  bool success;
  try {
    std::set<std::string> already_generated{program->get_path()};
    success = generate(
        program,
        generator_strings,
        already_generated,
        user_python_compiler,
        arguments);
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
    return 1;
  }

  // Clean up. Who am I kidding... this program probably orphans heap memory
  // all over the place, but who cares because it is about to exit and it is
  // all referenced and used by this wacky parse tree up until now anyways.

  delete program;
  delete g_type_void;
  delete g_type_string;
  delete g_type_bool;
  delete g_type_byte;
  delete g_type_i16;
  delete g_type_i32;
  delete g_type_i64;
  delete g_type_double;
  delete g_type_float;

  // Finished
  if (success) {
    return 0;
  } else {
    return 1;
  }
}
