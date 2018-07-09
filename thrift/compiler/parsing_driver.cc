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

namespace apache {
namespace thrift {

void parsing_driver::debug(const char* fmt, ...) const {
  if (!params_.debug) {
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
  if (!params_.verbose) {
    return;
  }
  va_list args;
  va_start(args, fmt);
  vprintf(fmt, args);
  va_end(args);
}

void parsing_driver::warning(int level, const char* fmt, ...) const {
  if (params_.warn < level) {
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

} // namespace thrift
} // namespace apache
