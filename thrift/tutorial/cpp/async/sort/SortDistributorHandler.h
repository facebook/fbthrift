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

#include <vector>
#include <folly/SocketAddress.h>

#include "thrift/tutorial/cpp/async/sort/util.h"
#include "thrift/tutorial/cpp/async/sort/gen-cpp2/Sorter.h"


namespace apache { namespace thrift { namespace tutorial { namespace sort {

/**
 * Asynchronous, distributed handler implementation for Sorter
 */
class SortDistributorHandler : public SorterSvIf {
 public:

  /**
   * Construct with a list of backend host:port pairs.
   *
   * sort() requests will be distributed to all known backend sort servers.
   */
  explicit SortDistributorHandler(std::vector<folly::SocketAddress> backends) :
      backends_(std::move(backends)) {}

  SortDistributorHandler(const SortDistributorHandler&) = delete;
  const SortDistributorHandler& operator=(
      const SortDistributorHandler&) = delete;

  folly::Future<std::vector<int32_t>> future_sort(
      const std::vector<int32_t>& values) override;

 private:

  std::vector<folly::SocketAddress> backends_;
};

}}}}
