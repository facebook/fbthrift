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
#ifndef SHELLHANDLER_H
#define SHELLHANDLER_H

#include "thrift/tutorial/cpp/stateful/AuthHandler.h"
#include "thrift/tutorial/cpp/stateful/gen-cpp/ShellService.h"

class ShellHandler : virtual public ShellServiceIf, public AuthHandler {
 public:
  ShellHandler(const std::shared_ptr<ServiceAuthState>& serviceAuthState,
               apache::thrift::server::TConnectionContext* ctx);
  ~ShellHandler();

  void pwd(std::string& _return);
  void chdir(const std::string& dir);
  void listDirectory(std::vector<StatInfo> & _return, const std::string& dir);
  void cat(std::string& _return, const std::string& file);

 protected:
  void validateState();
  void throwErrno(const char* msg);

  int cwd_;
};

class ShellHandlerFactory : public ShellServiceIfFactory {
 public:
  ShellHandlerFactory(const std::shared_ptr<ServiceAuthState>& authState) :
      authState_(authState) {}

  ShellHandler* getHandler(apache::thrift::server::TConnectionContext* ctx) {
    return new ShellHandler(authState_, ctx);
  }

  void releaseHandler(AuthenticatedServiceIf* handler) {
    delete handler;
  }

 protected:
  std::shared_ptr<ServiceAuthState> authState_;
};

#endif // SHELLHANDLER_H
