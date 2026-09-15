/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
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

#include <string_view>

#include <folly/container/F14Map.h>

namespace apache::thrift::fast_thrift::thrift {

struct ThriftServerMethodMetadata {
  std::string_view serviceName;
  std::string_view definingServiceName;
  std::string_view methodName;
  std::string_view qualifiedMethodName;
};

class ThriftServerMethodMetadataRegistry {
 public:
  // The registry borrows generated string literals. The first entry wins,
  // matching the composite app adapter's method-routing precedence.
  void add(ThriftServerMethodMetadata metadata) {
    methods_.try_emplace(metadata.methodName, metadata);
  }

  void clear() noexcept { methods_.clear(); }

  const ThriftServerMethodMetadata* find(
      std::string_view methodName) const noexcept {
    const auto it = methods_.find(methodName);
    return it == methods_.end() ? nullptr : &it->second;
  }

 private:
  folly::F14FastMap<std::string_view, ThriftServerMethodMetadata> methods_;
};

} // namespace apache::thrift::fast_thrift::thrift
