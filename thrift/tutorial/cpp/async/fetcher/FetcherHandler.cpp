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

using namespace std;
using namespace folly;

namespace apache { namespace thrift { namespace tutorial { namespace fetcher {

/**
 * fetchHttp() implementation
 */
Future<string>
FetcherHandler::future_fetchHttp(
    const FetchHttpRequest& request) {
  auto eb = getEventBase();
  auto p = make_shared<Promise<string>>();
  auto ret_cob = [=](string value) { p->setValue(move(value)); };
  auto exn_cob = [=](exception_wrapper exn) { p->setException(move(exn)); };
  auto fetcher = new HttpFetcher(
      eb, ret_cob, exn_cob, request.addr, request.port, request.path);
  via(eb, [=] { fetcher->fetch(); });
  return p->getFuture().ensure([=] { delete fetcher; });
}

}}}}
