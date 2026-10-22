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

#pragma once

#include <string>
#include <utility>

#include <Python.h>
#include <folly/futures/Future.h>
#include <thrift/lib/python/server/execution/ExecutionSystem.h>
#include <thrift/lib/python/server/execution/RequestDispatch.h>

namespace apache::thrift::python::test {

// Starts one request whose selected handler function is its own coroutine
// factory, so a test observes which function and loop each request received.
inline void startSelectedFunctionForTest(
    execution::ExecutionSystem& executionSystem, std::string method) {
  executionSystem.execute(
      std::move(method),
      execution::RequestDispatch(
          folly::Executor::KeepAlive<>{},
          [](PyObject* selectedFunction,
             const folly::Executor::KeepAlive<>&,
             execution::LaunchRequest launchRequest) {
            std::move(launchRequest)(selectedFunction);
            return folly::makeSemiFuture();
          }));
}

} // namespace apache::thrift::python::test
