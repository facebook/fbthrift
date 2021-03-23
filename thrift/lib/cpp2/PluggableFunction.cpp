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

#include <thrift/lib/cpp2/PluggableFunction.h>

#include <unordered_map>

#include <folly/Demangle.h>
#include <folly/Synchronized.h>

namespace apache {
namespace thrift {
namespace detail {

class PluggableFunctionMetadata {
 public:
  void setDefault(intptr_t defaultImpl) {
    CHECK(!locked_) << "Pluggable function can't be updated once locked";
    CHECK(!std::exchange(defaultImpl_, defaultImpl))
        << "Pluggable function can be registered only once";
  }
  void set(intptr_t impl) {
    CHECK(!locked_) << "Pluggable function can't be updated once locked";
    CHECK(!std::exchange(impl_, impl))
        << "Pluggable function can be registered only once";
  }
  intptr_t get() {
    locked_ = true;
    if (impl_) {
      return impl_;
    }
    CHECK(defaultImpl_);
    return defaultImpl_;
  }

 private:
  std::atomic<bool> locked_{false};
  intptr_t defaultImpl_{};
  intptr_t impl_{};
};

namespace {
PluggableFunctionMetadata& getPluggableFunctionMetadata(
    folly::StringPiece name, std::type_index functionTag) {
  using Map = std::unordered_map<
      std::string,
      std::pair<std::type_index, std::unique_ptr<PluggableFunctionMetadata>>>;
  static auto& map = *new folly::Synchronized<Map>();
  auto wMap = map.wlock();
  auto entryAndInserted = wMap->emplace(std::make_pair(
      name.str(),
      std::make_pair(
          functionTag, std::make_unique<PluggableFunctionMetadata>())));
  auto& entry = entryAndInserted.first->second;
  CHECK(entry.first == functionTag)
      << "Type mismatch for pluggable function " << name << ". Types are "
      << folly::demangle(entry.first.name()) << " and "
      << folly::demangle(functionTag.name());
  return *entry.second;
}
} // namespace

PluggableFunctionMetadata* registerPluggableFunction(
    folly::StringPiece name,
    std::type_index functionTag,
    intptr_t defaultImpl) {
  auto& metadata = getPluggableFunctionMetadata(name, functionTag);
  metadata.setDefault(defaultImpl);
  return &metadata;
}

void setPluggableFunction(
    folly::StringPiece name, std::type_index functionTag, intptr_t impl) {
  auto& metadata = getPluggableFunctionMetadata(name, functionTag);
  metadata.set(impl);
}

intptr_t getPluggableFunction(PluggableFunctionMetadata* metadata) {
  return metadata->get();
}
} // namespace detail
} // namespace thrift
} // namespace apache
