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
#define __STDC_FORMAT_MACROS

#include "thrift/tutorial/cpp/stateful/AuthHandler.h"

#include <iostream>

#include "thrift/tutorial/cpp/stateful/ServiceAuthState.h"
#include <thrift/lib/cpp/server/TConnectionContext.h>

using std::endl;
using std::vector;
using std::cout;
using std::string;
using namespace boost;
using namespace apache::thrift;
using namespace apache::thrift::server;

AuthHandler::AuthHandler(const std::shared_ptr<ServiceAuthState>& serviceState,
                         TConnectionContext* ctx) :
    serviceState_(serviceState),
    clientAddress_(*ctx->getPeerAddress()) {
  sessionInfo_.openTime = time(nullptr);
  sessionInfo_.clientInfo = computeClientInfoString();
  sessionInfo_.id = serviceState_->sessionOpened(this);
}

AuthHandler::~AuthHandler() {
  serviceState_->sessionClosed(this);
}

void
AuthHandler::authenticate(const string& username) {
  if (hasAuthenticated()) {
    throwLoginError("cannot authenticate twice on the same connection");
  }

  sessionInfo_.username = username;
  sessionInfo_.loginTime = time(nullptr);

  cout << "\"" << username << "\" logged in from " <<
    sessionInfo_.clientInfo << endl;
}

// GetInfo should ideally be a local class inside listSessions().
// Unfortunately, until C++1x is out, local classes can't be used as template
// arguments.
namespace {
class GetInfo {
 public:
  explicit GetInfo(vector<SessionInfo>* infoVector) :
      infoVector_(infoVector) {}

  void operator()(AuthHandler* handler) {
    infoVector_->push_back(*handler->getSessionInfo());
  }

 private:
  vector<SessionInfo>* infoVector_;
};
} // unnamed namespace

void
AuthHandler::listSessions(vector<SessionInfo> &_return) {
  GetInfo getInfo(&_return);
  serviceState_->forEachSession(std::ref(getInfo));
}

void
AuthHandler::throwLoginError(const string& message) const {
  LoginError err;
  err.message = message;
  throw err;
}

string
AuthHandler::computeClientInfoString() const {
  char portBuf[16];
  snprintf(portBuf, sizeof(portBuf), ":%" PRIu16, clientAddress_.getPort());
  return clientAddress_.getAddressStr() + portBuf;
}
