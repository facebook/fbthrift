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

#include <thrift/lib/python/capi/iobuf.h>

#include <folly/python/import.h>
#include <thrift/lib/python/exceptions_api.h>

namespace apache::thrift::python::capi::detail {

namespace {

bool ensure_module_imported() {
  static ::folly::python::import_cache_nocapture import(
      ::import_thrift__python__exceptions);
  return import();
}

} // namespace

void handle_protocol_error(const apache::thrift::TProtocolException& e) {
  if (!ensure_module_imported()) {
    PyErr_SetString(PyExc_ValueError, e.what());
    return;
  }
  PyObject* exc =
      create_ProtocolError_from_cpp(static_cast<int>(e.getType()), e.what());
  if (exc) {
    PyErr_SetObject(reinterpret_cast<PyObject*>(Py_TYPE(exc)), exc);
    Py_DECREF(exc);
  }
  CHECK(PyErr_Occurred()) << "Unknown error while creating ProtocolError";
}

} // namespace apache::thrift::python::capi::detail
