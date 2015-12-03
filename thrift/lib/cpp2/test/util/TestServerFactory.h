/*
 * Copyright 2015 Facebook, Inc.
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

#include <thrift/lib/cpp2/server/BaseThriftServer.h>

struct TestServerFactory {
 public:
  TestServerFactory() {}
  virtual ~TestServerFactory() {}

  virtual std::shared_ptr<apache::thrift::BaseThriftServer> create() = 0;

  virtual TestServerFactory& useSimpleThreadManager(bool use) = 0;
  virtual TestServerFactory& useThreadManager(
      std::shared_ptr<apache::thrift::concurrency::ThreadManager> exe) = 0;

  TestServerFactory& setServerEventHandler(std::shared_ptr<
      apache::thrift::server::TServerEventHandler> serverEventHandler) {
    serverEventHandler_ = serverEventHandler;
    return *this;
  }

 protected:
  std::shared_ptr<apache::thrift::server::TServerEventHandler>
      serverEventHandler_{nullptr};
};
