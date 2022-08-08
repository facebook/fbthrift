/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <thrift/conformance/rpcserver/RPCServerConformanceHandler.h>

namespace apache::thrift::conformance {

void RPCServerConformanceHandler::requestResponseBasic(
    Response& res, std::unique_ptr<Request> req) {
  result_.requestResponseBasic_ref().emplace().request() = *req;
  res = *testCase_->serverInstruction_ref()
             ->requestResponseBasic_ref()
             ->response();
}

} // namespace apache::thrift::conformance
