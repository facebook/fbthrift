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

#pragma once

#include "thrift/compiler/ast/t_base_type.h"

namespace apache {
namespace thrift {
namespace compiler {

/**
 * Global types for the parser to be able to reference
 *
 * shared_ptr<> would be nice, but...
 */
t_type* void_type();
t_type* string_type();
t_type* binary_type();
t_type* bool_type();
t_type* byte_type();
t_type* i16_type();
t_type* i32_type();
t_type* i64_type();
t_type* double_type();
t_type* float_type();

} // namespace compiler
} // namespace thrift
} // namespace apache
