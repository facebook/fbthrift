/*
 * Copyright 2018-present Facebook, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <Python.h>

#include <folly/Executor.h>
#include <folly/Function.h>
#include <folly/ScopeGuard.h>
#include <folly/io/IOBuf.h>

#ifdef PY_VERSION_HEX < 0x03040000
#define PyGILState_Check() (true)
#endif

namespace thrift {
namespace py3 {

struct PyBufferData {
  folly::Executor* executor;
  PyObject* py_object;
};

std::unique_ptr<folly::IOBuf> iobuf_from_python(
    folly::Executor* executor,
    PyObject* py_object,
    void* buf,
    uint64_t length) {
  Py_INCREF(py_object);
  auto* userData = new PyBufferData();
  userData->executor = executor;
  userData->py_object = py_object;

  return folly::IOBuf::takeOwnership(
      buf,
      length,
      [](void* buf, void* userData) {
        auto* py_data = (PyBufferData*)userData;
        auto* py_object = py_data->py_object;
        if (PyGILState_Check()) {
          Py_DECREF(py_object);
        } else {
          py_data->executor->add(
              [py_object]() mutable { Py_DECREF(py_object); });
        }
        delete py_data;
      },
      userData);
}
} // namespace py3
} // namespace thrift
