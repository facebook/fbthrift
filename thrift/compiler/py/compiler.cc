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

#include <thrift/compiler/py/compiler.h>

unique_ptr<t_base_type> g_type_void_sptr;

// TODO reimplement validate_throws(t_struct*) in python?
// we don't want to import t_generator here
//
// we have to write its signature though because it's used somewhere
bool validate_throws(t_struct* /*throws*/) {
  return true;
}

namespace thrift { namespace compiler { namespace py {

/**
 * Parse it up.. then spit it back out, in pretty much every language. Alright
 * not that many languages, but the cool ones that we care about.
 */
void process(const dict& params, const object& generate_callback) {

  string out_path;

  // extract parameters
  dict to_generate = extract<dict> (params["to_generate"]);
  object opts = params["options"];

  stl_input_iterator<object> end;
  stl_input_iterator<object> it;

  // bind to c++ values.
  // we'll need these when calling c++ generators instead of the python ones
  g_debug = extract<bool> (opts.attr("debug"));
  g_warn = extract<int> (opts.attr("warn"));
  g_strict = extract<int> (opts.attr("strict"));
  g_verbose = extract<bool> (opts.attr("verbose"));
  out_path = extract<string> (opts.attr("outputDir"));
  g_allow_neg_field_keys = extract<bool> (opts.attr("allow_neg_keys"));
  g_allow_neg_enum_vals = extract<bool> (opts.attr("allow_neg_enum_vals"));
  g_allow_64bit_consts = extract<bool> (opts.attr("allow_64bit_consts"));

  auto e = extract<list> (opts.attr("includeDirs"));
  if (e.check()) {
    // if it's set, push the include dirs into the global search path
    list includes = e;
    for (it = stl_input_iterator<object> (includes); it != end; ++it) {
      g_incl_searchpath.push_back(extract<string> (*it));
    }
  }

  const string input_file = extract<string> (params["thrift_file"]);

  // Instance of the global parse tree
  unique_ptr<t_program> program(new t_program(input_file));
  if (out_path.size()) {
    program->set_out_path(out_path, false);
  }

  // Compute the cpp include prefix.
  // infer this from the filename passed in
  const string &input_filename = input_file;
  string include_prefix;

  string::size_type last_slash = string::npos;
  if ((last_slash = input_filename.rfind("/")) != string::npos) {
    include_prefix = input_filename.substr(0, last_slash);
  }

  program->set_include_prefix(include_prefix);

  // Initialize global types
  // Moved this one inside BOOST_PYTHON_MODULE as we need to expose it
  //unique_ptr<t_base_type> type_void(
  //    new t_base_type("void",   t_base_type::TYPE_VOID));
  unique_ptr<t_base_type> type_string(
      new t_base_type("string", t_base_type::TYPE_STRING));
  unique_ptr<t_base_type> type_binary(
      new t_base_type("string", t_base_type::TYPE_STRING));
  type_binary->set_binary(true);
  unique_ptr<t_base_type> type_slist(
      new t_base_type("string", t_base_type::TYPE_STRING));
  type_slist->set_string_list(true);
  unique_ptr<t_base_type> type_bool(
      new t_base_type("bool",   t_base_type::TYPE_BOOL));
  unique_ptr<t_base_type> type_byte(
      new t_base_type("byte",   t_base_type::TYPE_BYTE));
  unique_ptr<t_base_type> type_i16(
      new t_base_type("i16",    t_base_type::TYPE_I16));
  unique_ptr<t_base_type> type_i32(
      new t_base_type("i32",    t_base_type::TYPE_I32));
  unique_ptr<t_base_type> type_i64(
      new t_base_type("i64",    t_base_type::TYPE_I64));
  unique_ptr<t_base_type> type_double(
      new t_base_type("double", t_base_type::TYPE_DOUBLE));
  unique_ptr<t_base_type> type_float(
      new t_base_type("float",  t_base_type::TYPE_FLOAT));
  // Assign pointers to actual global variables
  //g_type_void = type_void.get();
  g_type_string = type_string.get();
  g_type_binary = type_binary.get();
  g_type_slist = type_slist.get();
  g_type_bool = type_bool.get();
  g_type_byte = type_byte.get();
  g_type_i16 = type_i16.get();
  g_type_i32 = type_i32.get();
  g_type_i64 = type_i64.get();
  g_type_double = type_double.get();
  g_type_float = type_float.get();

  // Parse it!
  parse(program.get(), nullptr);

  // The current path is not really relevant when we are doing generation.
  // Reset the variable to make warning messages clearer.
  g_curpath = "generation";
  // Reset yylineno for the heck of it.  Use 1 instead of 0 because
  // That is what shows up during argument parsing.
  yylineno = 1;

  // Generate it in python!
  // boost doesn't have the necessary prowess to handle unique_ptr or
  // std::shared_ptr.
  // However our generate function will never delete the t_program object,
  // so it's safe to just give it the raw pointer
  call<void> (generate_callback.ptr(), ptr(program.get()), to_generate);
}

bool t_program_operatorEq(const t_program* self, const t_program* rhs) {
  return self == rhs;
}

bool t_program_operatorNe(const t_program* self, const t_program* rhs) {
  return !t_program_operatorEq(self, rhs);
}

}}} // thrift::compiler::py
