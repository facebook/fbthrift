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

#include <thrift/lib/cpp/protocol/TType.h>

namespace apache {
namespace thrift {
namespace detail {

using VoidFuncPtr = void (*)(void*);

struct TypeInfo {
  protocol::TType type;

  // Returns the value of a Thrift object, dereferencing a smart pointer and
  // converting a user-defined (via cpp.type) to native Thrift type if
  // necessary:
  //   OptionalThriftValue get(const void* object);
  VoidFuncPtr get;

  // A function to set an object of a specific type, so deserialization logic
  // can modify or initialize the object accordingly.
  // This function helps us support cpp.type for primitive fields.
  // It should take a Thrift object pointer and optionally the value to set.
  // For container types, the function is the initialization function to clear
  // the container before deserializing into the container.
  VoidFuncPtr set;

  // A pointer to additional type information, e.g. `MapFieldExt` for a map.
  const void* typeExt;
};

template <typename TypeClass, typename T, typename Enable = void>
struct TypeToInfo;

} // namespace detail
} // namespace thrift
} // namespace apache
