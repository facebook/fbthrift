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

#include <Python.h>
#include <folly/Function.h>
#include <folly/futures/Future.h>

namespace apache::thrift::python::execution {

// Launches the request's Python-side processing where placement put it. The
// coroutine factory is borrowed only for the call.
using LaunchRequest = folly::Function<int(_object*)>;

using RequestDispatch = folly::Function<folly::SemiFuture<folly::Unit>(
    PyObject* selectedHandlerFunction, LaunchRequest launchRequest)>;

} // namespace apache::thrift::python::execution
