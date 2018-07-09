/*
 * Copyright 2018-present Facebook, Inc.
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
#include <thrift/compiler/parsing_driver.h>

#include <cstdarg>

#include <boost/filesystem.hpp>

namespace apache {
namespace thrift {

void parsing_driver::debug(const char* fmt, ...) const {
  if (!params.debug) {
    return;
  }
  va_list args;
  printf("[PARSE:%d] ", yylineno);
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
  printf("\n");
}

void parsing_driver::verbose(const char* fmt, ...) const {
  if (!params.verbose) {
    return;
  }
  va_list args;
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
}

void parsing_driver::warning(int level, const char* fmt, ...) const {
  if (params.warn < level) {
    return;
  }
  va_list args;
  fprintf(stderr, "[WARNING:%s:%d] ", g_curpath.c_str(), yylineno);
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  fprintf(stderr, "\n");
}

[[noreturn]] void parsing_driver::failure(const char* fmt, ...) const {
  va_list args;
  fprintf(stderr, "[FAILURE:%s:%d] ", g_curpath.c_str(), yylineno);
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  fprintf(stderr, "\n");
  exit(1);
}

// TODO: This doesn't really need to be a member function. Move it somewhere
// else (e.g. `util.{h|cc}`) once everything gets consolidated into `parse/`.
/* static */ std::string
    parsing_driver::directory_name(const std::string& filename) {
  std::string::size_type slash = filename.rfind("/");
  // No slash, just use the current directory
  if (slash == std::string::npos) {
    return ".";
  }
  return filename.substr(0, slash);
}

std::string parsing_driver::include_file(const std::string& filename) {
  // Absolute path? Just try that
  if (filename[0] == '/') {
    boost::filesystem::path abspath{filename};
    try {
      abspath = boost::filesystem::canonical(abspath);
      return abspath.string();
    } catch (const boost::filesystem::filesystem_error& e) {
      failure("Could not find file: %s. Error: %s", filename.c_str(), e.what());
    }
  } else { // relative path, start searching
    // new search path with current dir global
    vector<std::string> sp = params.incl_searchpath;
    sp.insert(sp.begin(), curdir_);

    // iterate through paths
    vector<std::string>::iterator it;
    for (it = sp.begin(); it != sp.end(); it++) {
      std::string sfilename = *(it) + "/" + filename;
      if (boost::filesystem::exists(sfilename)) {
        return sfilename;
      } else {
        debug("Could not find: %s.", sfilename.c_str());
      }
    }

    // File was not found
    failure("Could not find include file %s", filename.c_str());
  }
}

} // namespace thrift
} // namespace apache
