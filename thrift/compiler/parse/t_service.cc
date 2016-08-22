/*
 * Copyright 2016 Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <thrift/compiler/parse/t_service.h>

#include <unordered_map>
#include <unordered_set>
#include <stdexcept>
#include <folly/Format.h>
#include <folly/Range.h>

#include <thrift/compiler/common.h>

using SPH = folly::hasher<folly::StringPiece>;

void t_service::validate() const {
  // Enforces that there are no duplicate method names either within this
  // service or between this service and any of its ancestors.
  bool err = false;
  std::unordered_map<folly::StringPiece, t_service*, SPH> base_function_names;
  for (auto b = extends_; b != nullptr; b = b->get_extends()) {
    for (const auto& f : b->get_functions()) {
      base_function_names[f->get_name()] = b;
    }
  }
  for (auto f : functions_) {
    auto i = base_function_names.find(f->get_name());
    if (i != base_function_names.end()) {
      err = true;
      auto msg = folly::sformat(
          "[FAILURE:{}:{}] Function {}.{} redefines {}.{}\n",
            g_curpath.c_str(),
            f->lineno_,
            get_name(),
            f->get_name(),
            i->second->get_full_name(),
            f->get_name());
      fputs(msg.c_str(), stderr);
    }
  }
  std::unordered_set<folly::StringPiece, SPH> function_names;
  for (auto f : functions_) {
    if (function_names.count(f->get_name())) {
      err = true;
      auto msg = folly::sformat(
            "[FAILURE:{}:{}] Function {}.{} redefines {}.{}\n",
            g_curpath.c_str(),
            f->lineno_,
            get_name(),
            f->get_name(),
            get_name(),
            f->get_name());
      fputs(msg.c_str(), stderr);
    }
    function_names.insert(f->get_name());
  }
  if (err) {
    exit(1);
  }
}
