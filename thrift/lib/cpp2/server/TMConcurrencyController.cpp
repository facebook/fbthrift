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

#include <thrift/lib/cpp2/async/AsyncProcessorHelper.h>
#include <thrift/lib/cpp2/server/TMConcurrencyController.h>

namespace apache::thrift {

void TMConcurrencyController::scheduleOnExecutor() {
  auto req = pile_.dequeue();
  auto es = req->requestContext()->getRequestExecutionScope();

  if (req) {
    apache::thrift::detail::ServerRequestHelper::setExecutor(
        req.value(),
        tm_.getKeepAlive(es, concurrency::ThreadManager::Source::INTERNAL));
  }

  tm_.getKeepAlive(es, concurrency::ThreadManager::Source::UPSTREAM)
      ->add([this, req = std::move(req)]() mutable {
        executeRequest(std::move(req));
      });
}

std::string TMConcurrencyController::describe() const {
  return fmt::format(
      "{{ParallelTMConcurrencyController executionLimit={}}}",
      executionLimit_.load());
}

serverdbginfo::ConcurrencyControllerDbgInfo
TMConcurrencyController::getDbgInfo() const {
  serverdbginfo::ConcurrencyControllerDbgInfo info;
  info.name() = folly::demangle(typeid(*this));
  info.qpsLimit() = getQpsLimit();
  info.concurrencyLimit() = getExecutionLimitRequests();
  return info;
}

} // namespace apache::thrift
