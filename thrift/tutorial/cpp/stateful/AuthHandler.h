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
#ifndef AUTHHANDLER_H
#define AUTHHANDLER_H

#include "thrift/lib/cpp/transport/TSocketAddress.h"
#include "thrift/tutorial/cpp/stateful/gen-cpp/AuthenticatedService.h"

class ServiceAuthState;

class AuthHandler : virtual public AuthenticatedServiceIf {
 public:
  AuthHandler(const std::shared_ptr<ServiceAuthState>& serviceState,
              apache::thrift::server::TConnectionContext* ctx);
  ~AuthHandler();

  virtual void authenticate(const std::string& username);
  virtual void listSessions(std::vector<SessionInfo> &_return);

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
  apache::thrift::transport::TSocketAddress clientAddress_;
};

#endif // AUTHHANDLER_H
