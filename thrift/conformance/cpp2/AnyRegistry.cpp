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

#include <folly/io/Cursor.h>

namespace apache::thrift::conformance {

bool AnyRegistry::registerType(const std::type_info& type, std::string name) {
  if (name.empty() || rev_index_.count(name) > 0) {
    return false;
  }
  auto result = registry_.emplace(
      std::type_index(type), TypeEntry(type, std::move(name)));
  if (!result.second) {
    return false;
  }
  rev_index_.emplace(result.first->second.name, std::type_index(type));
  return true;
}

bool AnyRegistry::registerSerializer(
    const std::type_info& type,
    const AnySerializer* serializer) {
  if (serializer == nullptr) {
    return false;
  }
  return registry_.at(std::type_index(type))
      .protocols
      .emplace(
          protocol_ids_.getOrCreateId(serializer->getProtocol()), serializer)
      .second;
}

bool AnyRegistry::registerSerializer(
    const std::type_info& type,
    std::unique_ptr<AnySerializer> serializer) {
  if (!registerSerializer(type, serializer.get())) {
    return false;
  }
  owned_serializers_.emplace_back(std::move(serializer));
  return true;
}

std::string_view AnyRegistry::getTypeName(const std::type_info& type) const {
  const auto* entry = getTypeEntry(type);
  if (entry == nullptr) {
    return {};
  }
  return entry->name;
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
  result.type_ref() = entry->name;
  result.protocol_ref() = protocol;
  result.data_ref() = queue.moveAsValue();
  return result;
}

Any AnyRegistry::store(const Any& value, const Protocol& protocol) const {
  if (value.protocol_ref()->getType() == Protocol::__EMPTY__ ||
      value.protocol_ref() == protocol) {
    return value;
  }
  return store(load(value), protocol);
}

void AnyRegistry::load(const Any& value, any_ref out) const {
  const auto* entry = getTypeEntry(*value.type_ref());
  const auto* serializer = getSerializer(entry, *value.protocol_ref());
  if (serializer == nullptr) {
    folly::throw_exception<std::bad_any_cast>();
  }
  folly::io::Cursor cursor(&*value.data_ref());
  serializer->decode(entry->type, cursor, out);
}

std::any AnyRegistry::load(const Any& value) const {
  std::any out;
  load(value, out);
  return out;
}

auto AnyRegistry::getTypeEntry(std::string_view name) const
    -> const TypeEntry* {
  auto itr = rev_index_.find(name);
  if (itr == rev_index_.end()) {
    return nullptr;
  }
  return getTypeEntry(itr->second);
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
  auto id = protocol_ids_.getId(protocol);
  if (id == ProtocolIdManager::kNoId) {
    return nullptr;
  }

  auto itr = entry->protocols.find(id);
  if (itr == entry->protocols.end()) {
    return nullptr;
  }
  return itr->second;
}

} // namespace apache::thrift::conformance
