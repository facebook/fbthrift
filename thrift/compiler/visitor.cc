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
  visit_and_recurse(program);
}

bool visitor::visit(t_program const* const program) {
  return true;
}

bool visitor::visit(t_service const* const service) {
  return true;
}

bool visitor::visit(t_enum const* const tenum) {
  return true;
}

void visitor::visit_and_recurse(t_program const* const program) {
  if (visit(program)) {
    recurse(program);
  }
}

void visitor::visit_and_recurse(t_service const* const service) {
  if (visit(service)) {
    recurse(service);
  }
}

void visitor::visit_and_recurse(t_enum const* const tenum) {
  if (visit(tenum)) {
    recurse(tenum);
  }
}

void visitor::recurse(t_program const* const program) {
  for (auto const* const service : program->get_services()) {
    visit_and_recurse(service);
  }
  for (auto const* const tenum : program->get_enums()) {
    visit_and_recurse(tenum);
  }
}

void visitor::recurse(t_service const* const service) {
  // partial implementation - that's the end of the line for now
}

void visitor::recurse(t_enum const* const tenum) {
  // partial implementation - that's the end of the line for now
}

interleaved_visitor::interleaved_visitor(std::vector<visitor*> visitors)
    : visitor(), visitors_(std::move(visitors)) {}

void interleaved_visitor::visit_and_recurse(t_program const* const program) {
  visit_and_recurse_gen(program);
}

void interleaved_visitor::visit_and_recurse(t_service const* const service) {
  visit_and_recurse_gen(service);
}

void interleaved_visitor::visit_and_recurse(t_enum const* const tenum) {
  visit_and_recurse_gen(tenum);
}

template <typename Visitee>
void interleaved_visitor::visit_and_recurse_gen(Visitee const* const visitee) {
  // track the set of visitors which return true from visit()
  auto rec_mask = std::vector<bool>(visitors_.size());
  auto any = false;
  for (size_t i = 0; i < visitors_.size(); ++i) {
    auto const rec = rec_mask_[i] && visitors_[i]->visit(visitee);
    rec_mask[i] = rec;
    any = any || rec;
  }
  // only recurse with the set of visitors which return true from visit()
  if (any) {
    std::swap(rec_mask_, rec_mask);
    recurse(visitee);
    std::swap(rec_mask_, rec_mask);
  }
}

}}}
