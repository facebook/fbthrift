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

#include <thrift/lib/python/server/execution/ExecutionSystem.h>

namespace apache::thrift::python::execution {

void ExecutionSystem::shutdown() noexcept {
  controlExecutor_.reset();
}

void ExecutionSystem::execute(
    std::string method, RequestDispatch requestDispatch) const {
  // The map holds every method the server accepts. A request reaches this
  // point only after the server accepted its method, so the lookup always
  // succeeds.
  const auto destination = executeOn_.at(method);
  (this->*destination)(std::move(method), std::move(requestDispatch));
}

void ExecutionSystem::executeOnControl(
    std::string method, RequestDispatch requestDispatch) const {
  folly::makeSemiFuture()
      .deferValue([method = std::move(method),
                   controlMethods = controlMethods_,
                   requestDispatch = std::move(requestDispatch),
                   launchRequest = LaunchRequest(
                       [controlExecutor = controlExecutor_,
                        launchControlRequest =
                            launchControlRequest_](_object* coroutineFactory) {
                         // Retain the selected destination until Cython starts
                         // the request.
                         (void)controlExecutor;
                         return launchControlRequest(coroutineFactory);
                       })](folly::Unit) mutable {
        // Both handler maps come from the same composition-root catalogue,
        // and the name already passed that catalogue lookup upstream in
        // executeRequest, so absence here is unreachable on valid paths.
        // Loop-local handlers are a future requirement. With loop-local
        // handlers, the handler function depends on the loop that executes
        // it, which is available only after the execution system schedules
        // a worker to take up the work. To prepare for that future, look
        // up the function by method name in today's single shared context.
        auto* selectedHandlerFunction = controlMethods->at(method);
        return std::move(requestDispatch)(
            selectedHandlerFunction, std::move(launchRequest));
      })
      .via(controlExecutor_);
}

} // namespace apache::thrift::python::execution
