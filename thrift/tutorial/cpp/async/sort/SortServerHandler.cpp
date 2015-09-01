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

#include <thrift/tutorial/cpp/async/sort/SortServerHandler.h>

#include <folly/gen/Base.h>

using namespace std;
using namespace folly;

namespace apache { namespace thrift { namespace tutorial { namespace sort {

void SortServerHandler::sort(
    vector<int32_t>& _return,
    const vector<int32_t>& values) {
  //  This operation is pure CPU work. No blocking ops, no async ops.
  //  So we perform it in a CPU thread pool thread, which is the default.
  _return = gen::from(values) | gen::order | gen::as<vector>();
}

}}}}
