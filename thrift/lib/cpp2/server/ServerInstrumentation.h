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

#include <folly/Synchronized.h>
#include <functional>
#include <mutex>
#include <set>
#include <vector>

namespace apache {
namespace thrift {

class ThriftServer;

/**
 * Instrument all the ThriftServer instances.
 * ServerInstrumentation serves for two purposes:
 * 1. Store a set of existing ThriftServer pointers in memory.
 * 2. As the entry point to access instrumentation data from servers.
 */
class ServerInstrumentation {
  friend class ThriftServer;

 public:
  static size_t getServerCount() {
    return ServerCollection::getInstance().getServers()->size();
  }

  template <typename F>
  static void forEachServer(F&& f) {
    auto servers = ServerCollection::getInstance().getServers();
    for (auto server : *servers) {
      f(*server);
    }
  }

 private:
  static void registerServer(ThriftServer& server) {
    ServerCollection::getInstance().addServer(server);
  }

  static void removeServer(ThriftServer& server) {
    ServerCollection::getInstance().removeServer(server);
  }

  class ServerCollection {
   private:
    folly::Synchronized<std::set<ThriftServer*>> servers_;
    ServerCollection() {}

   public:
    ServerCollection(ServerCollection const&) = delete;
    void operator=(ServerCollection const&) = delete;
    static ServerCollection& getInstance();

    folly::Synchronized<std::set<ThriftServer*>>::ConstLockedPtr getServers()
        const;
    void addServer(ThriftServer& server);
    void removeServer(ThriftServer& server);
  };
};
} // namespace thrift
} // namespace apache
