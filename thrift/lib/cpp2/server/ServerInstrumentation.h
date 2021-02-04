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

#include <cstddef>
#include <set>
#include <string>
#include <string_view>

#include <folly/Function.h>

namespace apache {
namespace thrift {

class ThriftServer;

namespace instrumentation {

constexpr std::string_view kThriftServerTrackerKey = "thrift_server";

class ServerTracker {
 public:
  ServerTracker(std::string_view key, ThriftServer& server);
  ~ServerTracker();

  ThriftServer& getServer() const {
    return server_;
  }

  const std::string& getKey() const {
    return key_;
  }

 private:
  std::string key_;
  ThriftServer& server_;
};

void forAllTrackers(folly::FunctionRef<
                    void(std::string_view, const std::set<ServerTracker*>&)>);

size_t getServerCount(std::string_view key);

void forEachServer(
    std::string_view key,
    folly::FunctionRef<void(ThriftServer&)>);

} // namespace instrumentation
} // namespace thrift
} // namespace apache
