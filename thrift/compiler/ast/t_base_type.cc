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

#include "thrift/compiler/ast/t_base_type.h"

namespace apache {
namespace thrift {
namespace compiler {

const t_base_type& t_base_type::t_void() {
  static t_base_type type{"void", TYPE_VOID};
  return type;
}

const t_base_type& t_base_type::t_string() {
  static t_base_type type{"string", TYPE_STRING};
  return type;
}

const t_base_type& t_base_type::t_binary() {
  // NOTE: thrift compiler used to treat both string and binary as string.
  static t_base_type type{"string", TYPE_BINARY};
  return type;
}

const t_base_type& t_base_type::t_bool() {
  static t_base_type type{"bool", TYPE_BOOL};
  return type;
}

const t_base_type& t_base_type::t_byte() {
  static t_base_type type{"byte", TYPE_BYTE};
  return type;
}

const t_base_type& t_base_type::t_i16() {
  static t_base_type type{"i16", TYPE_I16};
  return type;
}

const t_base_type& t_base_type::t_base_type::t_i32() {
  static t_base_type type{"i32", TYPE_I32};
  return type;
}

const t_base_type& t_base_type::t_i64() {
  static t_base_type type{"i64", TYPE_I64};
  return type;
}

const t_base_type& t_base_type::t_double() {
  static t_base_type type{"double", TYPE_DOUBLE};
  return type;
}

const t_base_type& t_base_type::t_float() {
  static t_base_type type{"float", TYPE_FLOAT};
  return type;
}

} // namespace compiler
} // namespace thrift
} // namespace apache
