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

/**
 * thrift - a lightweight cross-language rpc/serialization tool
 *
 * This file contains the main compiler engine for Thrift, which invokes the
 * scanner/parser to build the thrift object tree. The interface generation
 * code for each language lives in a file by the language name under the
 * generate/ folder, and all parse structures live in parse/
 *
 */

#include "thrift/compiler/main.h"

#include "thrift/compiler/generate/t_generator.h"

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
bool gen_xsd = false;
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
void usage() {
  fprintf(stderr, "Usage: thrift [options] file\n");
  fprintf(stderr, "Options:\n");
  fprintf(stderr, "  -o dir      Set the output directory for gen-* packages\n");
  fprintf(stderr, "               (default: current directory)\n");
  fprintf(stderr, "  -I dir      Add a directory to the list of directories\n");
  fprintf(stderr, "                searched for include directives\n");
  fprintf(stderr, "  -nowarn     Suppress all compiler warnings (BAD!)\n");
  fprintf(stderr, "  -strict     Strict compiler warnings on\n");
  fprintf(stderr, "  -v[erbose]  Verbose mode\n");
  fprintf(stderr, "  -r[ecurse]  Also generate included files\n");
  fprintf(stderr, "  -debug      Parse debug trace to stdout\n");
  fprintf(stderr, "  --allow-neg-keys  Allow negative field keys (Used to "
          "preserve protocol\n");
  fprintf(stderr, "                compatibility with older .thrift files)\n");
  fprintf(stderr, "  --allow-neg-enum-vals Allow negative enum vals\n");
  fprintf(stderr, "  --allow-64bit-consts  Do not print warnings about using 64-bit constants\n");
  fprintf(stderr, "  --gen STR   Generate code with a dynamically-registered generator.\n");
  fprintf(stderr, "                STR has the form language[:key1=val1[,key2,[key3=val3]]].\n");
  fprintf(stderr, "                Keys and values are options passed to the generator.\n");
  fprintf(stderr, "                Many options will not require values.\n");
  fprintf(stderr, "  --record-genfiles FILE\n");
  fprintf(stderr, "              Save the list of generated files to FILE\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Available generators (and options):\n");

  t_generator_registry::gen_map_t gen_map = t_generator_registry::get_generator_map();
  t_generator_registry::gen_map_t::iterator iter;
  for (iter = gen_map.begin(); iter != gen_map.end(); ++iter) {
    fprintf(stderr, "  %s (%s):\n",
        iter->second->get_short_name().c_str(),
        iter->second->get_long_name().c_str());
    fprintf(stderr, "%s", iter->second->get_documentation().c_str());
  }
  exit(1);
}



/**
 * Generate code
 */
bool generate(t_program* program, const vector<string>& generator_strings,
              char** argv) {
  // Oooohh, recursive code generation, hot!!
  if (gen_recurse) {
    const vector<t_program*>& includes = program->get_includes();
    for (size_t i = 0; i < includes.size(); ++i) {
      // Propogate output path from parent to child programs
      includes[i]->set_out_path(program->get_out_path());

      if (!generate(includes[i], generator_strings, argv)) {
        return false;
      }
    }
  }

  // Generate code!
  try {
    pverbose("Program: %s\n", program->get_path().c_str());

    if (dump_docs) {
      dump_docstrings(program);
    }

    vector<string>::const_iterator iter;
    for (iter = generator_strings.begin(); iter != generator_strings.end(); ++iter) {
      t_generator* generator = t_generator_registry::get_generator(program, *iter);

      if (generator == NULL) {
        // Attempt to call the new python compiler if we can find it
        string path = getenv("_");
        size_t last = path.find_last_of("/");
        if (last != string::npos) {
          string pycompiler = path.substr(0, last + 1) + "thrift-py.par";
          ifstream ifile(pycompiler);
          int ret = 0;
          if (ifile) {
            ret = execv(pycompiler.c_str(), argv);
          }
          if (!ifile || ret < 0) {
            pwarning(1, "Unable to get a generator for \"%s\".\n",
                     iter->c_str());
          }
        }
      } else {
        pverbose("Generating \"%s\"\n", iter->c_str());
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
 * Check that all the elements of a throws block are actually exceptions.
 */
bool validate_throws(t_struct* throws) {
  const vector<t_field*>& members = throws->get_members();
  vector<t_field*>::const_iterator m_iter;
  for (m_iter = members.begin(); m_iter != members.end(); ++m_iter) {
    if (!t_generator::get_true_type((*m_iter)->get_type())->is_xception()) {
      return false;
    }
  }
  return true;
}

/**
 * Parse it up.. then spit it back out, in pretty much every language. Alright
 * not that many languages, but the cool ones that we care about.
 */
int main(int argc, char** argv) {
  int i;
  std::string out_path;

  // Setup time string
  time_t now = time(NULL);
  g_time_str = ctime(&now);

  // Check for necessary arguments, you gotta have at least a filename and
  // an output language flag
  if (argc < 2) {
    usage();
  }

  vector<string> generator_strings;

  // Set the current path to a dummy value to make warning messages clearer.
  g_curpath = "arguments";

  // Hacky parameter handling... I didn't feel like using a library sorry!
  for (i = 1; i < argc-1; i++) {
    char* arg;

    arg = strtok(argv[i], " ");
    while (arg != NULL) {
      // Treat double dashes as single dashes
      if (arg[0] == '-' && arg[1] == '-') {
        ++arg;
      }

      if (strcmp(arg, "-debug") == 0) {
        g_debug = 1;
      } else if (strcmp(arg, "-nowarn") == 0) {
        g_warn = 0;
      } else if (strcmp(arg, "-strict") == 0) {
        g_strict = 255;
        g_warn = 2;
      } else if (strcmp(arg, "-v") == 0 || strcmp(arg, "-verbose") == 0 ) {
        g_verbose = 1;
      } else if (strcmp(arg, "-r") == 0 || strcmp(arg, "-recurse") == 0 ) {
        gen_recurse = true;
      } else if (strcmp(arg, "-allow-neg-keys") == 0) {
        g_allow_neg_field_keys = true;
      } else if (strcmp(arg, "-allow-neg-enum-vals") == 0) {
        g_allow_neg_enum_vals = true;
      } else if (strcmp(arg, "-allow-64bit-consts") == 0) {
        g_allow_64bit_consts = true;
      } else if (strcmp(arg, "-record-genfiles") == 0) {
        record_genfiles = true;
        arg = argv[++i];
        if (arg == NULL) {
          fprintf(stderr, "!!! Missing genfile file specification\n");
          usage();
        }
        genfile_file.open(arg);
      } else if (strcmp(arg, "-gen") == 0) {
        arg = argv[++i];
        if (arg == NULL) {
          fprintf(stderr, "!!! Missing generator specification\n");
          usage();
        }
        generator_strings.push_back(arg);
      } else if (strcmp(arg, "-dense") == 0) {
        gen_dense = true;
      } else if (strcmp(arg, "-cpp") == 0) {
        gen_cpp = true;
      } else if (strcmp(arg, "-javabean") == 0) {
        gen_javabean = true;
      } else if (strcmp(arg, "-java") == 0) {
        gen_java = true;
      } else if (strcmp(arg, "-php") == 0) {
        gen_php = true;
      } else if (strcmp(arg, "-phpi") == 0) {
        gen_phpi = true;
      } else if (strcmp(arg, "-phps") == 0) {
        gen_php = true;
        gen_phps = true;
      } else if (strcmp(arg, "-phpl") == 0) {
        gen_php = true;
        gen_phps = false;
      } else if (strcmp(arg, "-phpa") == 0) {
        gen_php = true;
        gen_phps = false;
        gen_phpa = true;
      } else if (strcmp(arg, "-phpo") == 0) {
        gen_php = true;
        gen_phpo = true;
      } else if (strcmp(arg, "-rest") == 0) {
        gen_rest = true;
      } else if (strcmp(arg, "-py") == 0) {
        gen_py = true;
      } else if (strcmp(arg, "-pyns") == 0) {
        gen_py = true;
        gen_py_newstyle = true;
      } else if (strcmp(arg, "-rb") == 0) {
        gen_rb = true;
      } else if (strcmp(arg, "-xsd") == 0) {
        gen_xsd = true;
      } else if (strcmp(arg, "-perl") == 0) {
        gen_perl = true;
      } else if (strcmp(arg, "-erl") == 0) {
        gen_erl = true;
      } else if (strcmp(arg, "-ocaml") == 0) {
        gen_ocaml = true;
      } else if (strcmp(arg, "-hs") == 0) {
        gen_hs = true;
      } else if (strcmp(arg, "-cocoa") == 0) {
        gen_cocoa = true;
      } else if (strcmp(arg, "-st") == 0) {
        gen_st = true;
      } else if (strcmp(arg, "-csharp") == 0) {
        gen_csharp = true;
      } else if (strcmp(arg, "-cpp_use_include_prefix") == 0) {
        g_cpp_use_include_prefix = true;
      } else if (strcmp(arg, "-I") == 0) {
        // An argument of "-I\ asdf" is invalid and has unknown results
        arg = argv[++i];

        if (arg == NULL) {
          fprintf(stderr, "!!! Missing Include directory\n");
          usage();
        }
        g_incl_searchpath.push_back(arg);
      } else if (strcmp(arg, "-o") == 0) {
        arg = argv[++i];
        if (arg == NULL) {
          fprintf(stderr, "-o: missing output directory\n");
          usage();
        }
        out_path = arg;

#ifdef MINGW
        //strip out trailing \ on Windows
        int last = out_path.length()-1;
        if (out_path[last] == '\\')
        {
          out_path.erase(last);
        }
#endif

        struct stat sb;
        if (stat(out_path.c_str(), &sb) < 0) {
          fprintf(stderr, "Output directory %s is unusable: %s\n", out_path.c_str(), strerror(errno));
          return -1;
        }
        if (! S_ISDIR(sb.st_mode)) {
          fprintf(stderr, "Output directory %s exists but is not a directory\n", out_path.c_str());
          return -1;
        }
      } else {
        fprintf(stderr, "!!! Unrecognized option: %s\n", arg);
        usage();
      }

      // Tokenize more
      arg = strtok(NULL, " ");
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
    } else if(gen_phps) {
      gen_string.append("server,");
    } else if(gen_phpa) {
      gen_string.append("autoload,");
    } else if(gen_phpo) {
      gen_string.append("oop,");
    } else if(gen_rest) {
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
  if (gen_xsd) {
    pwarning(1, "-xsd is deprecated.  Use --gen xsd");
    generator_strings.push_back("xsd");
  }

  // You gotta generate something!
  if (generator_strings.empty()) {
    fprintf(stderr, "!!! No output language(s) specified\n\n");
    usage();
  }

  // Real-pathify it
  char rp[PATH_MAX];
  if (argv[i] == NULL) {
    fprintf(stderr, "!!! Missing file name\n");
    usage();
  }
  if (saferealpath(argv[i], rp) == NULL) {
    failure("Could not open input file with realpath: %s", argv[i]);
  }
  string input_file(rp);

  // Instance of the global parse tree
  t_program* program = new t_program(input_file);
  if (out_path.size()) {
    program->set_out_path(out_path);
  }

  // Compute the cpp include prefix.
  // infer this from the filename passed in
  string input_filename = argv[i];
  string include_prefix;

  string::size_type last_slash = string::npos;
  if ((last_slash = input_filename.rfind("/")) != string::npos) {
    include_prefix = input_filename.substr(0, last_slash);
  }

  program->set_include_prefix(include_prefix);

  // Initialize global types
  g_type_void   = new t_base_type("void",   t_base_type::TYPE_VOID);
  g_type_string = new t_base_type("string", t_base_type::TYPE_STRING);
  g_type_binary = new t_base_type("string", t_base_type::TYPE_STRING);
  ((t_base_type*)g_type_binary)->set_binary(true);
  g_type_slist  = new t_base_type("string", t_base_type::TYPE_STRING);
  ((t_base_type*)g_type_slist)->set_string_list(true);
  g_type_bool   = new t_base_type("bool",   t_base_type::TYPE_BOOL);
  g_type_byte   = new t_base_type("byte",   t_base_type::TYPE_BYTE);
  g_type_i16    = new t_base_type("i16",    t_base_type::TYPE_I16);
  g_type_i32    = new t_base_type("i32",    t_base_type::TYPE_I32);
  g_type_i64    = new t_base_type("i64",    t_base_type::TYPE_I64);
  g_type_double = new t_base_type("double", t_base_type::TYPE_DOUBLE);
  g_type_float  = new t_base_type("float",  t_base_type::TYPE_FLOAT);

  // Parse it!
  parse(program, NULL);

  // The current path is not really relevant when we are doing generation.
  // Reset the variable to make warning messages clearer.
  g_curpath = "generation";
  // Reset yylineno for the heck of it.  Use 1 instead of 0 because
  // That is what shows up during argument parsing.
  yylineno = 1;

  // Generate it!
  bool success = generate(program, generator_strings, argv);

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
