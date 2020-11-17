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

#include <initializer_list>

#include <thrift/conformance/cpp2/UniversalType.h>
#include <thrift/conformance/if/gen-cpp2/thrift_type_info_types.h>

namespace apache::thrift::conformance {

inline constexpr type_hash_size_t kTypeHashBytesNotSpecified = -1;

// Creates an ThriftTypeInfo struct with the given names and configuration.
//
// The first name in names is set as the primary name, and all others are added
// as aliases.
template <typename C = std::initializer_list<const char*>>
ThriftTypeInfo createThriftTypeInfo(
    C&& names,
    type_hash_size_t typeHashBytes = kTypeHashBytesNotSpecified);

// Raises std::invalid_argument if invalid.
void validateThriftTypeInfo(const ThriftTypeInfo& type);

// Implementation

template <typename C>
ThriftTypeInfo createThriftTypeInfo(C&& uris, type_hash_size_t typeHashBytes) {
  ThriftTypeInfo type;
  if (typeHashBytes != kTypeHashBytesNotSpecified) {
    type.set_typeHashBytes(typeHashBytes);
  }
  auto itr = uris.begin();
  if (itr == uris.end()) {
    folly::throw_exception<std::invalid_argument>(
        "At least one name must be provided.");
  }
  // TODO(afuller): Fix folly::forward_like for containers that only expose
  // const access.
  if constexpr (std::is_const_v<std::remove_reference_t<decltype(*itr)>>) {
    type.set_uri(*itr++);
    for (; itr != uris.end(); ++itr) {
      type.altUris_ref()->emplace(*itr);
    }
  } else {
    type.set_uri(folly::forward_like<C>(*itr++));
    for (; itr != uris.end(); ++itr) {
      type.altUris_ref()->emplace(folly::forward_like<C>(*itr));
    }
  }
  return type;
}

} // namespace apache::thrift::conformance
