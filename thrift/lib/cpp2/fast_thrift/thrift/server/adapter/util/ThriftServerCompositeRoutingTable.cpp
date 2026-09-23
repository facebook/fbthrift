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

#include <thrift/lib/cpp2/fast_thrift/thrift/server/adapter/util/ThriftServerCompositeRoutingTable.h>

#include <limits>
#include <utility>

#include <folly/logging/xlog.h>

namespace apache::thrift::fast_thrift::thrift {

std::shared_ptr<const ThriftServerCompositeRoutingTable>
ThriftServerCompositeRoutingTable::create(
    std::vector<std::shared_ptr<const ThriftServerMethodDispatchTable>>
        childTables) {
  return std::shared_ptr<const ThriftServerCompositeRoutingTable>(
      new ThriftServerCompositeRoutingTable(std::move(childTables)));
}

ThriftServerCompositeRoutingTable::ThriftServerCompositeRoutingTable(
    std::vector<std::shared_ptr<const ThriftServerMethodDispatchTable>>
        childTables)
    : childTables_(std::move(childTables)) {
  CHECK_LE(childTables_.size(), std::numeric_limits<uint16_t>::max());
  std::size_t methodCount = 0;
  for (const auto& table : childTables_) {
    CHECK(table);
    methodCount += table->size();
  }
  routes_.reserve(methodCount);

  for (std::size_t childIndex = 0; childIndex < childTables_.size();
       ++childIndex) {
    childTables_[childIndex]->forEach(
        [&](std::string_view name,
            ThriftServerMethodDispatchTable::DispatchFn dispatch) {
          auto [_, inserted] = routes_.try_emplace(
              name, Route{dispatch, static_cast<uint16_t>(childIndex)});
          if (!inserted) {
            XLOG(WARN)
                << "ThriftServerCompositeRoutingTable: method '" << name
                << "' already claimed by an earlier child; dropping from "
                   "child index "
                << childIndex;
          }
        });
  }
}

} // namespace apache::thrift::fast_thrift::thrift
