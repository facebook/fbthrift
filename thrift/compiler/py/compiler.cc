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

#include <thrift/compiler/py/compiler.h>

#include <thrift/compiler/parse/parsing_driver.h>

namespace thrift {
namespace compiler {
namespace py {

/**
 * Parse it up.. then spit it back out, in pretty much every language. Alright
 * not that many languages, but the cool ones that we care about.
 */
void process(const dict& params, const object& generate_callback) {
  string out_path;

  // extract parameters
  dict to_generate = extract<dict>(params["to_generate"]);
  object opts = params["options"];

  stl_input_iterator<object> end;
  stl_input_iterator<object> it;

  // bind to c++ values.
  // we'll need these when calling c++ generators instead of the python ones
  g_debug = extract<bool>(opts.attr("debug"));
  g_warn = extract<int>(opts.attr("warn"));
  g_verbose = extract<bool>(opts.attr("verbose"));
  out_path = extract<string>(opts.attr("outputDir"));

  std::vector<std::string> incl_searchpath;

  auto e = extract<boost::python::list>(opts.attr("includeDirs"));
  if (e.check()) {
    // if it's set, push the include dirs into the global search path
    boost::python::list includes = e;
    for (it = stl_input_iterator<object>(includes); it != end; ++it) {
      incl_searchpath.push_back(extract<string>(*it));
    }
  }

  const string input_file = extract<string>(params["thrift_file"]);

  g_stage = "parse";

  // Parse it!
  apache::thrift::parsing_params parsing_params{};
  parsing_params.debug = (g_debug != 0);
  parsing_params.verbose = (g_verbose != 0);
  parsing_params.warn = g_warn;
  parsing_params.strict = extract<int>(opts.attr("strict"));
  parsing_params.allow_neg_field_keys =
      extract<bool>(opts.attr("allow_neg_keys"));
  parsing_params.allow_neg_enum_vals =
      extract<bool>(opts.attr("allow_neg_enum_vals"));
  parsing_params.allow_64bit_consts =
      extract<bool>(opts.attr("allow_64bit_consts"));
  parsing_params.incl_searchpath = incl_searchpath;
  unique_ptr<t_program> program{
      parse_and_dump_diagnostics(input_file, std::move(parsing_params))};

  // Generate it!
  g_stage = "generation";

  if (out_path.size()) {
    program->set_out_path(out_path, false);
  }

  // Compute the cpp include prefix.
  // infer this from the filename passed in
  const string& input_filename = input_file;
  string include_prefix;

  string::size_type last_slash = string::npos;
  if ((last_slash = input_filename.rfind("/")) != string::npos) {
    include_prefix = input_filename.substr(0, last_slash);
  }

  program->set_include_prefix(include_prefix);

  // Generate it in python!
  // boost doesn't have the necessary prowess to handle unique_ptr or
  // std::shared_ptr.
  // However our generate function will never delete the t_program object,
  // so it's safe to just give it the raw pointer
  call<void>(generate_callback.ptr(), ptr(program.get()), to_generate);
}

bool t_program_operatorEq(const t_program* self, const t_program* rhs) {
  return self == rhs;
}

bool t_program_operatorNe(const t_program* self, const t_program* rhs) {
  return !t_program_operatorEq(self, rhs);
}

bool t_const_operatorEq(const t_const* self, const t_const* rhs) {
  return self == rhs;
}

bool t_const_operatorNe(const t_const* self, const t_const* rhs) {
  return !t_const_operatorEq(self, rhs);
}

} // namespace py
} // namespace compiler
} // namespace thrift
