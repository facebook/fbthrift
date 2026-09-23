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

#include <cstdint>
#include <memory>
#include <string_view>
#include <vector>

#include <folly/Portability.h>
#include <folly/container/F14Map.h>

#include <thrift/lib/cpp2/fast_thrift/thrift/server/adapter/util/ThriftServerMethodDispatchTable.h>

namespace apache::thrift::fast_thrift::thrift {

/** Immutable method-to-child routing shared by all server connections. */
class ThriftServerCompositeRoutingTable final {
 public:
  struct Route {
    ThriftServerMethodDispatchTable::DispatchFn dispatch;
    uint16_t childIndex;
  };

  static std::shared_ptr<const ThriftServerCompositeRoutingTable> create(
      std::vector<std::shared_ptr<const ThriftServerMethodDispatchTable>>
          childTables);

  FOLLY_ALWAYS_INLINE const Route* find(
      std::string_view methodName) const noexcept {
    const auto it = routes_.find(methodName);
    return it == routes_.end() ? nullptr : &it->second;
  }

 private:
  explicit ThriftServerCompositeRoutingTable(
      std::vector<std::shared_ptr<const ThriftServerMethodDispatchTable>>
          childTables);

  // Keeps the strings referenced by routes_' string_view keys alive.
  std::vector<std::shared_ptr<const ThriftServerMethodDispatchTable>>
      childTables_;
  folly::F14FastMap<std::string_view, Route> routes_;
};

} // namespace apache::thrift::fast_thrift::thrift
