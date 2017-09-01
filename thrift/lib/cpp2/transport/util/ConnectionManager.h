/*
 * Copyright 2017-present Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <folly/Synchronized.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include <stdint.h>
#include <thrift/lib/cpp2/transport/core/ClientConnectionIf.h>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace apache {
namespace thrift {

/**
 * Manages a pool of "ClientConnectionIf" objects, each one on its own
 * event base (thread) and offers them for use in a round-robin
 * fashion.
 */
class ConnectionManager {
 public:
  // Returns a singleton instance of this factory.
  static std::shared_ptr<ConnectionManager> getInstance();

  ConnectionManager();

  ~ConnectionManager() = default;

  // Returns a connection that may be used to talk to a server at
  // "addr:port".
  std::shared_ptr<ClientConnectionIf> getConnection(
      const std::string& addr,
      uint16_t port);

 private:
  struct Connections {
    // The connections for a specific server.  There will be
    // "numClientConnections_" entries in this vector.
    std::vector<std::shared_ptr<ClientConnectionIf>> clients;
    int32_t current;

    Connections();
  };

  // The threads on which the connections run.  There will be
  // "numClientConnections_" entries in this vector.
  std::vector<std::unique_ptr<folly::ScopedEventBaseThread>> threads_;
  // Map of clients per server - keyed by "addr:port".
  folly::Synchronized<std::unordered_map<std::string, Connections>> clients_;
};

} // namespace thrift
} // namespace apache
