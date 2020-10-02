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

#include <thrift/conformance/cpp2/Protocol.h>
#include <thrift/conformance/if/gen-cpp2/any_constants.h>
#include <thrift/conformance/if/gen-cpp2/any_types.h>

namespace apache::thrift::conformance {

// Returns the protocol used by the given any.
Protocol getProtocol(const Any& any) noexcept;

// Returns true iff the any is encoded using the given protocol.
bool hasProtocol(const Any& any, const Protocol& protocol) noexcept;

// Optimizes the represenation for standard protocols and
// removes any unneeded customProtocol values.
void normalizeProtocol(Any& any) noexcept;

// Creates an AnyType struct with the given names and configuration.
//
// The first name in names is set as the primary name, and all others are added
// as aliases.
template <typename C = std::initializer_list<const char*>>
AnyType createAnyType(
    C&& names,
    int8_t typeIdBytes = any_constants::minTypeIdBytes());

// Raises std::invalid_argument if invalid.
void validateAny(const Any& any);
void validateAnyType(const AnyType& type);

// Implementation

template <typename C>
AnyType createAnyType(C&& names, int8_t typeIdBytes) {
  AnyType type;
  type.set_typeIdBytes(typeIdBytes);
  auto itr = names.begin();
  if (itr == names.end()) {
    folly::throw_exception<std::invalid_argument>(
        "At least one name must be provided.");
  }
  // TODO(afuller): Fix folly::forward_like for containers that only expose
  // const access.
  if constexpr (std::is_const_v<std::remove_reference_t<decltype(*itr)>>) {
    type.set_name(*itr++);
    for (; itr != names.end(); ++itr) {
      type.aliases_ref()->emplace(*itr);
    }
  } else {
    type.set_name(folly::forward_like<C>(*itr++));
    for (; itr != names.end(); ++itr) {
      type.aliases_ref()->emplace(folly::forward_like<C>(*itr));
    }
  }
  return type;
}

} // namespace apache::thrift::conformance
