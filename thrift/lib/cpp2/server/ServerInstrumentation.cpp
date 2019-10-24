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

#include <thrift/lib/cpp2/server/ServerInstrumentation.h>

namespace apache {
namespace thrift {

ServerInstrumentation::ServerCollection&
ServerInstrumentation::ServerCollection::getInstance() {
  static ServerCollection* the_singleton = new ServerCollection();
  return *the_singleton;
}

folly::Synchronized<std::set<ThriftServer*>>::ConstLockedPtr
ServerInstrumentation::ServerCollection::getServers() const {
  return servers_.rlock();
}

void ServerInstrumentation::ServerCollection::addServer(ThriftServer& server) {
  servers_.wlock()->insert(&server);
}

void ServerInstrumentation::ServerCollection::removeServer(
    ThriftServer& server) {
  servers_.wlock()->erase(&server);
}

} // namespace thrift
} // namespace apache
