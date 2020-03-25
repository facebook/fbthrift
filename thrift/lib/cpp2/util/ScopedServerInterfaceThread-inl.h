/*
 * Copyright (c) Facebook, Inc. and its affiliates.
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

#include <folly/executors/GlobalExecutor.h>
#include <folly/io/async/AsyncSocket.h>
#include <thrift/lib/cpp2/async/PooledRequestChannel.h>

namespace apache {
namespace thrift {

template <class AsyncClientT>
std::unique_ptr<AsyncClientT> ScopedServerInterfaceThread::newClient(
    folly::Executor* callbackExecutor,
    ScopedServerInterfaceThread::MakeChannelFunc makeChannel) const {
  return std::make_unique<AsyncClientT>(PooledRequestChannel::newChannel(
      callbackExecutor,
      folly::getIOExecutor(),
      [makeChannel = std::move(makeChannel),
       address = getAddress()](folly::EventBase& eb) mutable {
        return makeChannel(folly::AsyncSocket::UniquePtr(
            new folly::AsyncSocket(&eb, address)));
      }));
}

template <class AsyncClientT>
std::unique_ptr<AsyncClientT> ScopedServerInterfaceThread::newClient(
    folly::EventBase* eb) const {
  return std::make_unique<AsyncClientT>(HeaderClientChannel::newChannel(
      folly::AsyncSocket::newSocket(eb, getAddress())));
}

template <class AsyncClientT>
std::unique_ptr<AsyncClientT> ScopedServerInterfaceThread::newClient(
    folly::EventBase& eb) const {
  return newClient<AsyncClientT>(&eb);
}

} // namespace thrift
} // namespace apache
