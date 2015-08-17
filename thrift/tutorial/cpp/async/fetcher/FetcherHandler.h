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
#pragma once

#include "thrift/tutorial/cpp/async/fetcher/gen-cpp2/Fetcher.h"

namespace apache { namespace thrift { namespace tutorial { namespace fetcher {

/**
 * Asynchronous handler implementation for the Fetcher service
 */
class FetcherHandler : public FetcherSvIf {
 public:
  FetcherHandler() = default;

  FetcherHandler(const FetcherHandler&) = delete;
  const FetcherHandler& operator=(const FetcherHandler&) = delete;

  folly::Future<std::string>
  future_fetchHttp(
      const FetchHttpRequest& request) override;
};

}}}}
