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

// TODO(afuller): Delete this file.

#pragma once

#include <thrift/compiler/ast/t_base_type.h>

namespace apache {
namespace thrift {
namespace compiler {

/**
 * Global types for the parser to be able to reference
 */
inline const t_type* void_type() {
  return &t_base_type::t_void();
}
inline const t_type* string_type() {
  return &t_base_type::t_string();
}
inline const t_type* binary_type() {
  return &t_base_type::t_binary();
}
inline const t_type* bool_type() {
  return &t_base_type::t_bool();
}
inline const t_type* byte_type() {
  return &t_base_type::t_byte();
}
inline const t_type* i16_type() {
  return &t_base_type::t_i16();
}
inline const t_type* i32_type() {
  return &t_base_type::t_i32();
}
inline const t_type* i64_type() {
  return &t_base_type::t_i64();
}
inline const t_type* double_type() {
  return &t_base_type::t_double();
}
inline const t_type* float_type() {
  return &t_base_type::t_float();
}

} // namespace compiler
} // namespace thrift
} // namespace apache
