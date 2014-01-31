/*
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

#ifndef THRIFT_LIB_CPP_REFLECTION_H_
#define THRIFT_LIB_CPP_REFLECTION_H_

#include <cstddef>
#include <cstdint>

#include "thrift/lib/thrift/gen-cpp/reflection_types.h"

namespace apache {
namespace thrift {
namespace reflection {

namespace detail {
const size_t kTypeBits = 5;
const uint64_t kTypeMask = (1ULL << kTypeBits) - 1;
}  // namespace detail

inline int64_t makeTypeId(Type type, uint64_t hash) {
  return static_cast<int64_t>((hash & ~detail::kTypeMask) | type);
}

inline Type getType(int64_t typeId) {
  return static_cast<Type>(typeId & detail::kTypeMask);
}

inline bool isBaseType(Type type) {
  return (type <= TYPE_DOUBLE) || (type == TYPE_FLOAT);
}

}  // namespace reflection
}  // namespace thrift
}  // namespace apache

#endif /* THRIFT_LIB_CPP_REFLECTION_H_ */
