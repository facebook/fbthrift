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

#include <glog/logging.h>

#include <folly/CppAttributes.h>
#include <folly/io/Cursor.h>
#include <thrift/conformance/cpp2/Any.h>
#include <thrift/conformance/cpp2/UniversalType.h>

namespace apache::thrift::conformance {

namespace {

folly::fbstring maybeGetTypeId(
    const AnyType& type,
    type_id_size_t defaultTypeIdBytes = kMinTypeIdBytes) {
  if (type.typeIdBytes_ref().has_value()) {
    // Use the custom size.
    defaultTypeIdBytes = type.typeIdBytes_ref().value_unchecked();
  }
  return conformance::maybeGetTypeId(type.get_name(), defaultTypeIdBytes);
}

} // namespace

AnyRegistry::TypeEntry::TypeEntry(const std::type_info& typeInfo, AnyType type)
    : typeInfo(typeInfo), typeId(maybeGetTypeId(type)), type(std::move(type)) {}

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
  if (entry->typeId.empty()) {
    result.set_type(entry->type.get_name());
  } else {
    result.set_typeId(entry->typeId);
  }
  if (protocol.isCustom()) {
    result.customProtocol_ref() = protocol.custom();
  } else {
    result.protocol_ref() = protocol.standard();
  }
  result.set_data(queue.moveAsValue());
  return result;
}

Any AnyRegistry::store(const Any& value, const Protocol& protocol) const {
  if (hasProtocol(value, protocol)) {
    return value;
  }
  return store(load(value), protocol);
}

void AnyRegistry::load(const Any& value, any_ref out) const {
  const auto* entry = getTypeEntryFor(value);
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
  validateAnyType(type);
  std::vector<folly::fbstring> typeIds;
  typeIds.reserve(type.aliases_ref()->size() + 1);
  if (!genTypeIdsAndCheckForConflicts(type, &typeIds)) {
    return nullptr;
  }

  auto result = registry_.emplace(
      std::type_index(typeInfo), TypeEntry(typeInfo, std::move(type)));
  if (!result.second) {
    return nullptr;
  }

  TypeEntry* entry = &result.first->second;

  // Add to secondary indexes.
  indexName(*entry->type.name_ref(), entry);
  for (const auto& alias : *entry->type.aliases_ref()) {
    indexName(alias, entry);
  }

  for (auto& id : typeIds) {
    indexId(std::move(id), entry);
  }
  return &result.first->second;
}

bool AnyRegistry::registerSerializerImpl(
    const AnySerializer* serializer,
    TypeEntry* entry) {
  if (serializer == nullptr) {
    return false;
  }
  validateProtocol(serializer->getProtocol());
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

bool AnyRegistry::genTypeIdsAndCheckForConflicts(
    std::string_view name,
    std::vector<folly::fbstring>* typeIds) const {
  if (name.empty() || nameIndex_.contains(name)) {
    return false; // Already exists.
  }

  auto typeId = getTypeId(name);
  // Find shortest valid partial type id.
  folly::fbstring minTypeId(getPartialTypeId(typeId, kMinTypeIdBytes));
  // Check if the minimum type id would be ambiguous.
  if (containsTypeId(idIndex_, minTypeId)) {
    return false; // Ambigous with another typeId.
  }
  typeIds->emplace_back(std::move(typeId));
  return true;
}

bool AnyRegistry::genTypeIdsAndCheckForConflicts(
    const AnyType& type,
    std::vector<folly::fbstring>* typeIds) const {
  // Ensure name and all aliases are availabile.
  if (!genTypeIdsAndCheckForConflicts(*type.name_ref(), typeIds)) {
    return false;
  }
  for (const auto& alias : *type.aliases_ref()) {
    if (!genTypeIdsAndCheckForConflicts(alias, typeIds)) {
      return false;
    }
  }
  return true;
}

void AnyRegistry::indexName(std::string_view name, TypeEntry* entry) {
  auto res = nameIndex_.emplace(name, entry);
  DCHECK(res.second);
}

void AnyRegistry::indexId(folly::fbstring&& id, TypeEntry* entry) {
  auto res = idIndex_.emplace(std::move(id), entry);
  DCHECK(res.second);
}

auto AnyRegistry::getTypeEntry(const std::type_index& index) const
    -> const TypeEntry* {
  auto itr = registry_.find(index);
  if (itr == registry_.end()) {
    return nullptr;
  }
  return &itr->second;
}

auto AnyRegistry::getTypeEntryById(const folly::fbstring& id) const
    -> const TypeEntry* {
  validateTypeId(id);
  auto itr = findByTypeId(idIndex_, id);
  if (itr == idIndex_.end()) {
    // No match.
    return nullptr;
  }
  return itr->second;
}

auto AnyRegistry::getTypeEntryByName(std::string_view name) const
    -> const TypeEntry* {
  auto itr = nameIndex_.find(name);
  if (itr == nameIndex_.end()) {
    return nullptr;
  }
  return itr->second;
}

auto AnyRegistry::getTypeEntryFor(const Any& value) const -> const TypeEntry* {
  if (value.type_ref().has_value() &&
      !value.type_ref().value_unchecked().empty()) {
    return getTypeEntryByName(value.type_ref().value_unchecked());
  }
  if (value.typeId_ref().has_value()) {
    return getTypeEntryById(value.typeId_ref().value_unchecked());
  }
  return nullptr;
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
