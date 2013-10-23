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
#include "thrift/tutorial/cpp/async/fetcher/FetcherHandler.h"

#include "thrift/tutorial/cpp/async/fetcher/HttpFetcher.h"

namespace tutorial { namespace async { namespace fetcher {

/**
 * fetchHttp() implementation
 */
void FetcherHandler::fetchHttp(
    std::function<void(std::string const& _return)> cob,
    std::function<void(std::exception const&)> exn_cob,
    const std::string& host, const std::string& path) {
  // Create a new HttpFetcher to get the HTTP URL in an asynchronous manner
  HttpFetcher* fetcher = new HttpFetcher(getEventBase(), cob, exn_cob,
                                         host, path);

  // fetch() schedules the work to be done.
  // Eventually, cob or exn_cob will be called when it is finished.
  fetcher->fetch();
}

}}} // tutorial::async::fetcher
