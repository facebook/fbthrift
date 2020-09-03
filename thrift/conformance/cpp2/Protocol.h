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

#include <string>
#include <string_view>
#include <unordered_map>

#include <thrift/conformance/if/gen-cpp2/protocol_types.h>
#include <thrift/conformance/if/gen-cpp2/protocol_types.tcc>

namespace apache::thrift::conformance {

// Create a protocol with from the given name.
Protocol createProtocol(std::string name) noexcept;
// Create a protocol with from the given standard protocol.
Protocol createProtocol(StandardProtocol protocol) noexcept;

// Converts custom protocols to standard ones, if they have the name of a
// standard protocol.
void normalizeProtocol(Protocol* protocol) noexcept;

std::string_view getProtocolName(const Protocol& protocol);

// Hands out standard ids for standard protocols, and assigns runtime ids for
// custom protocols.
//
// Different instances may return different ids for custom protocols.
class ProtocolIdManager {
 public:
  // TODO(afuller): Use a unique type instead.
  using id_type = size_t;
  static constexpr id_type kNoId = 0;

  // Gets a protocol identifier for the given protocol.
  //
  // Returns kNoId if protocol is empty, or is custom and hasn't been seen
  // before.
  id_type getId(const Protocol& protocol) const noexcept;

  // Gets or creates a protocol identifier for the given protocol.
  //
  // @throws std::invalid_argument if protocol is empty.
  id_type getOrCreateId(const Protocol& protocol);

 private:
  std::unordered_map<std::string, id_type> customProtocols_;

  // Returns the id for the given standard protocol.
  static id_type getStandardId(StandardProtocol protocol) noexcept;

  // Tries to get the standard id for the given name.
  //
  // Returns kNoId if name is not a recognized standard names;
  static id_type getStandardId(const std::string& name) noexcept;

  // Returns the custom id based on the ordinal at which it was added.
  static id_type getCustomId(size_t ordinal) noexcept;

  // Find (or allocate) an id for the given protocol.
  id_type findId(const std::string& name) const noexcept;
  id_type findOrAllocateId(const std::string& name) noexcept;
};

} // namespace apache::thrift::conformance
