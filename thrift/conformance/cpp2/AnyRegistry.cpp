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

#include <thrift/conformance/cpp2/AnyRegistry.h>

#include <folly/CppAttributes.h>
#include <folly/io/Cursor.h>
#include <thrift/conformance/cpp2/Any.h>

namespace apache::thrift::conformance {

bool AnyRegistry::registerType(const std::type_info& typeInfo, AnyType type) {
  return registerTypeImpl(typeInfo, std::move(type)) != nullptr;
}

bool AnyRegistry::registerSerializer(
    const std::type_info& type,
    const AnySerializer* serializer) {
  return registerSerializerImpl(
      serializer, &registry_.at(std::type_index(type)));
}

bool AnyRegistry::registerSerializer(
    const std::type_info& type,
    std::unique_ptr<AnySerializer> serializer) {
  return registerSerializerImpl(
      std::move(serializer), &registry_.at(std::type_index(type)));
}

std::string_view AnyRegistry::getTypeName(const std::type_info& type) const {
  const auto* entry = getTypeEntry(type);
  if (entry == nullptr) {
    return {};
  }
  return entry->type.get_name();
}

const AnySerializer* AnyRegistry::getSerializer(
    const std::type_info& type,
    const Protocol& protocol) const {
  return getSerializer(getTypeEntry(type), protocol);
}

const AnySerializer* AnyRegistry::getSerializer(
    std::string_view name,
    const Protocol& protocol) const {
  return getSerializer(getTypeEntry(name), protocol);
}

Any AnyRegistry::store(any_ref value, const Protocol& protocol) const {
  if (value.type() == typeid(Any)) {
    // Use the Any specific overload.
    return store(any_cast<const Any&>(value), protocol);
  }

  const auto* entry = getTypeEntry(value.type());
  const auto* serializer = getSerializer(entry, protocol);
  if (serializer == nullptr) {
    folly::throw_exception<std::bad_any_cast>();
  }

  folly::IOBufQueue queue(folly::IOBufQueue::cacheChainLength());
  // Allocate 16KB at a time; leave some room for the IOBuf overhead
  constexpr size_t kDesiredGrowth = (1 << 14) - 64;
  serializer->encode(value, folly::io::QueueAppender(&queue, kDesiredGrowth));

  Any result;
  result.set_type(entry->type.get_name());
  if (protocol.isCustom()) {
    result.customProtocol_ref() = protocol.custom();
  } else {
    result.protocol_ref() = protocol.standard();
  }
  result.data_ref() = queue.moveAsValue();
  return result;
}

Any AnyRegistry::store(const Any& value, const Protocol& protocol) const {
  if (hasProtocol(value, protocol)) {
    return value;
  }
  return store(load(value), protocol);
}

void AnyRegistry::load(const Any& value, any_ref out) const {
  // TODO(afuller): Add support for type_id.
  if (!value.type_ref().has_value()) {
    folly::throw_exception<std::bad_any_cast>();
  }
  const auto* entry = getTypeEntry(*value.type_ref());
  const auto* serializer = getSerializer(entry, getProtocol(value));
  if (serializer == nullptr) {
    folly::throw_exception<std::bad_any_cast>();
  }
  folly::io::Cursor cursor(&*value.data_ref());
  serializer->decode(entry->typeInfo, cursor, out);
}

std::any AnyRegistry::load(const Any& value) const {
  std::any out;
  load(value, out);
  return out;
}

auto AnyRegistry::registerTypeImpl(const std::type_info& typeInfo, AnyType type)
    -> TypeEntry* {
  if (!checkNameAvailability(type)) {
    return nullptr;
  }

  auto result = registry_.emplace(
      std::type_index(typeInfo), TypeEntry(typeInfo, std::move(type)));
  if (!result.second) {
    return nullptr;
  }

  indexType(&result.first->second);
  return &result.first->second;
}

bool AnyRegistry::registerSerializerImpl(
    const AnySerializer* serializer,
    TypeEntry* entry) {
  if (serializer == nullptr) {
    return false;
  }
  return entry->serializers.emplace(serializer->getProtocol(), serializer)
      .second;
}

bool AnyRegistry::registerSerializerImpl(
    std::unique_ptr<AnySerializer> serializer,
    TypeEntry* entry) {
  if (!registerSerializerImpl(serializer.get(), entry)) {
    return false;
  }
  ownedSerializers_.emplace_front(std::move(serializer));
  return true;
}

bool AnyRegistry::checkNameAvailability(std::string_view name) const {
  return !name.empty() && !nameIndex_.contains(name);
}

bool AnyRegistry::checkNameAvailability(const AnyType& type) const {
  // Ensure name and all aliases are availabile.
  if (!checkNameAvailability(*type.name_ref())) {
    return false;
  }
  for (const auto& alias : *type.aliases_ref()) {
    if (!checkNameAvailability(alias)) {
      return false;
    }
  }
  return true;
}

void AnyRegistry::indexName(std::string_view name, TypeEntry* entry) {
  FOLLY_MAYBE_UNUSED auto res = nameIndex_.emplace(name, entry);
  assert(res.second);
  // TODO(afuller): Also index under typeId.
}

void AnyRegistry::indexType(TypeEntry* entry) {
  indexName(*entry->type.name_ref(), entry);
  for (const auto& alias : *entry->type.aliases_ref()) {
    indexName(alias, entry);
  }
}

auto AnyRegistry::getTypeEntry(std::string_view name) const
    -> const TypeEntry* {
  auto itr = nameIndex_.find(name);
  if (itr == nameIndex_.end()) {
    return nullptr;
  }
  return itr->second;
}

auto AnyRegistry::getTypeEntry(const std::type_index& index) const
    -> const TypeEntry* {
  auto itr = registry_.find(index);
  if (itr == registry_.end()) {
    return nullptr;
  }
  return &itr->second;
}

const AnySerializer* AnyRegistry::getSerializer(
    const TypeEntry* entry,
    const Protocol& protocol) const {
  if (entry == nullptr) {
    return nullptr;
  }

  auto itr = entry->serializers.find(protocol);
  if (itr == entry->serializers.end()) {
    return nullptr;
  }
  return itr->second;
}

} // namespace apache::thrift::conformance
