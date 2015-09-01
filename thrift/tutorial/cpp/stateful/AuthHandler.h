/*
 * Copyright 2014 Facebook, Inc.
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

#include <folly/SocketAddress.h>
#include <thrift/tutorial/cpp/stateful/gen-cpp2/AuthenticatedService.h>

namespace apache { namespace thrift { namespace tutorial { namespace stateful {

class ServiceAuthState;

class AuthHandler : virtual public AuthenticatedServiceSvIf {
 public:
  AuthHandler(std::shared_ptr<ServiceAuthState> serviceState,
              apache::thrift::server::TConnectionContext* ctx);
  ~AuthHandler() override;

  void authenticate(const std::string& username) override;
  void listSessions(std::vector<SessionInfo>& _return) override;

  const SessionInfo* getSessionInfo() const {
    return &sessionInfo_;
  }

  bool hasAuthenticated() const {
    return (sessionInfo_.loginTime != 0);
  }

 protected:
  void throwLoginError(const std::string& message) const;

  std::string computeClientInfoString() const;

  std::shared_ptr<ServiceAuthState> serviceState_;
  SessionInfo sessionInfo_;
  folly::SocketAddress clientAddress_;
};

}}}}
