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

#include <thrift/compiler/ast/t_typedef.h>

#include <thrift/compiler/ast/t_program.h>

namespace apache {
namespace thrift {
namespace compiler {

bool t_typedef::resolve_placeholder() const {
  assert(!type_);
  assert(!defined_);

  type_ = scope_->get_type(get_program()->get_name() + "." + symbolic_);

  return !!type_;
}

t_type* t_typedef::get_type() const {
  assert(!!type_);
  return type_;
}

} // namespace compiler
} // namespace thrift
} // namespace apache
