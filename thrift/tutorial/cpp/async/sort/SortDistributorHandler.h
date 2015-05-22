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
#ifndef THRIFT_TUTORIAL_ASYNC_SORTDISTRIBUTORHANDLER_H
#define THRIFT_TUTORIAL_ASYNC_SORTDISTRIBUTORHANDLER_H

#include <thrift/lib/cpp/async/TEventServer.h>

#include "thrift/tutorial/cpp/async/sort/util.h"
#include "thrift/tutorial/cpp/async/sort/gen-cpp/Sorter.h"


namespace tutorial { namespace sort {

/**
 * Asynchronous, distributed handler implementation for Sorter
 */
class SortDistributorHandler : public SorterCobSvIf, public boost::noncopyable {
 public:
  typedef std::vector<int32_t> IntVector;
  typedef std::function<void(const IntVector& _return)> SortReturnCob;
  typedef std::function<void(const std::exception& ex)>
    SortErrorCob;

  SortDistributorHandler() : server_(nullptr) { }

  void sort(SortReturnCob cob,
            SortErrorCob errcob,
            const IntVector& values) override;

  /**
   * Set the TEventServer that will be used.
   *
   * This is necessary so that we can get the TEventBase for performing
   * asynchronous operations.
   */
  void setServer(apache::thrift::async::TEventServer* server) {
    server_ = server;
  }

  /**
   * Add a thrift sort server
   *
   * sort() requests will be distributed to all known backend sort servers.
   */
  void addSortServer(const std::string& ip, uint16_t port) {
    sortServers_.push_back(std::make_pair(ip, port));
  }

 protected:
  /**
   * Get the TEventBase used by the current thread.
   */
  apache::thrift::async::TEventBase* getEventBase() const {
    if (!server_) {
      throw apache::thrift::TLibraryException(
          "SortDistributorHandler.server_ is NULL");
    }
    return server_->getEventBase();
  }

 private:
  class Aggregator;
  typedef std::vector< std::pair<std::string, uint16_t> > ServerVector;

  apache::thrift::async::TEventServer* server_;
  ServerVector sortServers_;
};

}} // tutorial::sort

#endif // THRIFT_TUTORIAL_ASYNC_SORTDISTRIBUTORHANDLER_H
