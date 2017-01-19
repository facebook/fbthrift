/*
 * Copyright 2017 Facebook, Inc.
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

#include <folly/ExceptionWrapper.h>
#include <folly/Try.h>
#include <folly/futures/Promise.h>
#include <folly/io/async/EventBase.h>
#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>

#include <Python.h>

#include <cstdint>
#include <exception>
#include <functional>

namespace apache {
namespace thrift {

template <class T, class U>
void make_py3_client(
    std::shared_ptr<folly::EventBase> event_base,
    const std::string& host,
    const uint16_t port,
    const uint32_t connect_timeout,
    std::function<void(PyObject*, folly::Try<std::shared_ptr<U>>)> callback,
    PyObject* py_future) {
  folly::via(
    event_base.get(),
    [=] {
      callback(
        py_future,
        folly::makeTryWith(
          [&] {
            auto client = std::make_shared<T>(
              HeaderClientChannel::newChannel(
                async::TAsyncSocket::newSocket(
                  event_base.get(),
                  host,
                  port,
                  connect_timeout)));
            return std::make_shared<U>(client, event_base);
          }));
    });
}

template <class T>
std::unique_ptr<T> py3_get_exception(
    const folly::exception_wrapper& exception) {
  try {
    exception.throwException();
  } catch (const T& typed_exception) {
    return std::make_unique<T>(typed_exception);
  }
}

}
} // namespace apache::thrift
