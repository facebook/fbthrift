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
  explicit PluggableFunctionMetadata(
      folly::StringPiece name, std::type_index functionTag) noexcept
      : name_(std::string{name}), functionTag_(functionTag) {}
  void setDefault(intptr_t defaultImpl) {
    // idempotent with a unique value of defaultImpl, error with divergent value
    if (defaultImpl == defaultImpl_) {
      return;
    }
    CHECK(!locked_) << "Pluggable function '" << name_
                    << "' can't be updated once locked";
    CHECK(!std::exchange(defaultImpl_, defaultImpl))
        << "Pluggable function '" << name_ << "' can be registered only once";
  }
  intptr_t getDefault() const { return defaultImpl_; }
  void set(intptr_t impl) {
    CHECK(!locked_) << "Pluggable function '" << name_
                    << "' can't be updated once locked";
    CHECK(!std::exchange(impl_, impl))
        << "Pluggable function '" << name_ << "' can be registered only once";
  }
  intptr_t get() {
    locked_ = true;
    if (impl_) {
      return impl_;
    }
    CHECK(defaultImpl_);
    return defaultImpl_;
  }
  std::type_index getFunctionTag() const { return functionTag_; }

 private:
  std::atomic<bool> locked_{false};
  intptr_t defaultImpl_{};
  intptr_t impl_{};
  const std::string name_;
  const std::type_index functionTag_;
};

namespace {
PluggableFunctionMetadata& getPluggableFunctionMetadata(
    folly::StringPiece name, std::type_index tag, std::type_index functionTag) {
  using Map = std::unordered_map<std::type_index, PluggableFunctionMetadata>;
  static auto& map = *new folly::Synchronized<Map>();
  auto wMap = map.wlock();
  auto emplacement = wMap->emplace(
      std::piecewise_construct,
      std::forward_as_tuple(tag),
      std::forward_as_tuple(name, functionTag));
  auto& metadata = emplacement.first->second;
  CHECK(metadata.getFunctionTag() == functionTag)
      << "Type mismatch for pluggable function " << name << ". Types are "
      << folly::demangle(metadata.getFunctionTag().name()) << " and "
      << folly::demangle(functionTag.name());
  return metadata;
}
} // namespace

PluggableFunctionMetadata* registerPluggableFunction(
    folly::StringPiece name,
    std::type_index tag,
    std::type_index functionTag,
    intptr_t defaultImpl) {
  // idempotent with a unique value of defaultImpl, error with divergent value
  auto& metadata = getPluggableFunctionMetadata(name, tag, functionTag);
  metadata.setDefault(defaultImpl);
  return &metadata;
}

void setPluggableFunction(
    folly::StringPiece name,
    std::type_index tag,
    std::type_index functionTag,
    intptr_t defaultImpl,
    intptr_t impl) {
  // the linker may decide the override happens before true registration so
  // register here just in case; double-registration should not be a problem
  // since registration with a unique value of defaultImpl is idempotent
  auto& metadata =
      *registerPluggableFunction(name, tag, functionTag, defaultImpl);
  CHECK_EQ(metadata.getDefault(), defaultImpl);
  metadata.set(impl);
}

intptr_t getPluggableFunction(PluggableFunctionMetadata* metadata) {
  return metadata->get();
}
} // namespace detail
} // namespace thrift
} // namespace apache
