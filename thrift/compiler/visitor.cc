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

#include <thrift/compiler/visitor.h>

namespace apache { namespace thrift { namespace compiler {

void visitor::traverse(t_program const* const program) {
  visit_(program);
}

bool visitor::visit(t_program const* const program) {
  return true;
}

bool visitor::visit(t_service const* const service) {
  return true;
}

void visitor::visit_(t_program const* const program) {
  if (visit(program)) {
    for (const auto service : program->get_services()) {
      visit_(service);
    }
  }
}

void visitor::visit_(t_service const* const service) {
  if (visit(service)) {
    // partial implementation - that's the end of the line for now
  }
}

}}}
