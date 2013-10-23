/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements. See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership. The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License. You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied. See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
#include "thrift/tutorial/cpp/stateful/ServiceAuthState.h"

using namespace apache::thrift::concurrency;

ServiceAuthState::ServiceAuthState()
    : nextId_(1) {
}

int64_t ServiceAuthState::sessionOpened(AuthHandler* handler) {
  Guard g(mutex_);

  sessions_.insert(handler);
  return nextId_++;
}

void ServiceAuthState::sessionClosed(AuthHandler* handler) {
  Guard g(mutex_);

  sessions_.erase(handler);
}

void ServiceAuthState::forEachSession(
    const std::function<void(AuthHandler*)>& fn) {
  Guard g(mutex_);

  for (std::set<AuthHandler*>::const_iterator it = sessions_.begin();
       it != sessions_.end();
       ++it) {
    fn(*it);
  }
}
