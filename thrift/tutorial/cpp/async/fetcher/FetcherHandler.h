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
#ifndef THRIFT_TUTORIAL_ASYNCSERVERHANDLER_H
#define THRIFT_TUTORIAL_ASYNCSERVERHANDLER_H

#include "thrift/tutorial/cpp/async/fetcher/gen-cpp/Fetcher.h"

#include <thrift/lib/cpp/async/TEventServer.h>

namespace tutorial { namespace async { namespace fetcher {

/**
 * Asynchronous handler implementation for the Fetcher service
 */
class FetcherHandler : public FetcherCobSvIf,
                       public boost::noncopyable {
 public:
  FetcherHandler() : server_(nullptr) { }

  void fetchHttp(std::function<void(std::string const&)> cob,
                 std::function<void(std::exception const&)>,
                 const std::string& host,
                 const std::string& path) override;

  /**
   * Set the TEventServer that will be used.
   *
   * This is necessary so that we can get the TEventBase for performing
   * asynchronous operations.
   */
  void setServer(apache::thrift::async::TEventServer* server) {
    server_ = server;
  }

 protected:
  /**
   * Get the TEventBase used by the current thread.
   */
  apache::thrift::async::TEventBase* getEventBase() const {
    return server_->getEventBase();
  }

 private:
  apache::thrift::async::TEventServer* server_;
};

}}} // tutorial::async::fetcher

#endif // THRIFT_TUTORIAL_ASYNCSERVERHANDLER_H
