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

#include "thrift/compiler/ast/base_types.h"

namespace apache {
namespace thrift {
namespace compiler {

t_type* void_type() {
  static t_base_type type{"void", t_base_type::TYPE_VOID};
  return &type;
}

t_type* string_type() {
  static t_base_type type{"string", t_base_type::TYPE_STRING};
  return &type;
}

t_type* binary_type() {
  // NOTE: thrift compiler used to treat both string and binary as string.
  static t_base_type type{"string", t_base_type::TYPE_BINARY};
  return &type;
}

t_type* bool_type() {
  static t_base_type type{"bool", t_base_type::TYPE_BOOL};
  return &type;
}

t_type* byte_type() {
  static t_base_type type{"byte", t_base_type::TYPE_BYTE};
  return &type;
}

t_type* i16_type() {
  static t_base_type type{"i16", t_base_type::TYPE_I16};
  return &type;
}

t_type* i32_type() {
  static t_base_type type{"i32", t_base_type::TYPE_I32};
  return &type;
}

t_type* i64_type() {
  static t_base_type type{"i64", t_base_type::TYPE_I64};
  return &type;
}

t_type* double_type() {
  static t_base_type type{"double", t_base_type::TYPE_DOUBLE};
  return &type;
}

t_type* float_type() {
  static t_base_type type{"float", t_base_type::TYPE_FLOAT};
  return &type;
}

} // namespace compiler
} // namespace thrift
} // namespace apache
