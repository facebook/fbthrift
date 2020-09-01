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

#include <any>
#include <memory>
#include <string>
#include <string_view>
#include <typeindex>
#include <typeinfo>
#include <unordered_map>

#include <folly/container/F14Map.h>
#include <thrift/conformance/cpp2/AnyRef.h>
#include <thrift/conformance/cpp2/AnySerializer.h>
#include <thrift/conformance/if/gen-cpp2/any_types.h>

namespace apache::thrift::conformance {

// A registry for serializers for use with apache::thrift::conformance::Any.
//
// This registry can be used to store and load
// apache::thrift::conformance::Any values using the registered serializers.
class AnyRegistry {
 public:
  // Store a value in an Any using the registered serializers.
  //
  // Throws std::bad_any_cast if no matching serializer has been registered.
  Any store(any_ref value, const Protocol& protocol) const;
  Any store(const Any& value, const Protocol& protocol) const;

  // Load a value from an Any using the registered serializers.
  //
  // Throws std::bad_any_cast if no matching serializer has been registered.
  void load(const Any& value, any_ref out) const;
  std::any load(const Any& value) const;
  template <typename T>
  T load(const Any& value) const;

  // Register a type's unique name.
  bool registerType(const std::type_info& type, std::string name);

  // Register a serializer for a given type.
  //
  // Throws std::out_of_range error if the type has not already been
  // registered.
  bool registerSerializer(
      const std::type_info& type,
      const AnySerializer* serializer);
  bool registerSerializer(
      const std::type_info& type,
      std::unique_ptr<AnySerializer> serializer);

  // Returns the unique type name for the given type, or "" if the type has not
  // been registered.
  std::string_view getTypeName(const std::type_info& type) const;

  // Returns the serializer for the given type and protocol, or nullptr if
  // no matching serializer is found.
  const AnySerializer* getSerializer(
      const std::type_info& type,
      const Protocol& protocol) const;
  const AnySerializer* getSerializer(
      std::string_view name,
      const Protocol& protocol) const;

  // Compile-time Type overloads.
  template <typename T>
  bool registerType(std::string name) {
    return registerType(typeid(T), std::move(name));
  }
  template <typename T>
  bool registerSerializer(const AnySerializer* serializer) {
    return registerSerializer(typeid(T), serializer);
  }
  template <typename T>
  bool registerSerializer(std::unique_ptr<AnySerializer> serializer) {
    return registerSerializer(typeid(T), std::move(serializer));
  }

 private:
  struct TypeEntry {
    TypeEntry(const std::type_info& type, std::string name)
        : type(type), name(std::move(name)) {}

    const std::type_info& type;
    const std::string name; // Referenced by rev_index_.
    folly::F14FastMap<ProtocolIdManager::id_type, const AnySerializer*>
        protocols;
  };

  std::vector<std::unique_ptr<AnySerializer>> owned_serializers_;
  // rev_index_ references TypeEntry.name.
  folly::F14NodeMap<std::type_index, TypeEntry> registry_;
  folly::F14FastMap<std::string_view, std::type_index> rev_index_;
  ProtocolIdManager protocol_ids_;

  // Gets the TypeEntry for the given type, or null if the type has not been
  // registered.
  const TypeEntry* getTypeEntry(std::string_view name) const;
  const TypeEntry* getTypeEntry(const std::type_index& index) const;
  const TypeEntry* getTypeEntry(const std::type_info& type) const {
    return getTypeEntry(std::type_index(type));
  }

  const AnySerializer* getSerializer(
      const TypeEntry* entry,
      const Protocol& protocol) const;
};

// Implementation details.

template <typename T>
T AnyRegistry::load(const Any& value) const {
  T out;
  load(value, out);
  return out;
}

} // namespace apache::thrift::conformance
